#!/usr/bin/env python3
"""
MoMask-based motion generation for the denizen animation pipeline.

Wraps MoMask inference to take a text prompt and return Mixamo-compatible
bone rotations ready for the pose editor.

Usage:
    from motion_generator import MotionGenerator

    gen = MotionGenerator()  # loads model once (~5s on GPU)
    frames = gen.generate("a knight draws a sword and slashes forward", duration=15.0)
    # frames = [{ 'mixamorigRightArm': (rx, ry, rz), ... }, ...]  360 entries
"""

import os
import sys
import numpy as np
import torch
import torch.nn.functional as F
from pathlib import Path

MOMASK_DIR = Path(__file__).parent.parent / 'momask'
sys.path.insert(0, str(MOMASK_DIR))

from retarget import retarget, blend_beat_sequences


class MotionGenerator:
    """Text-to-motion generator using MoMask with Mixamo retargeting."""

    def __init__(self, device=None, checkpoint_dir=None):
        """Load MoMask models.

        Args:
            device: 'cuda', 'cpu', or None (auto-detect)
            checkpoint_dir: Path to MoMask checkpoints (default: tools/momask/checkpoints)
        """
        if device is None:
            device = 'cuda' if torch.cuda.is_available() else 'cpu'
        self.device = torch.device(device)

        if checkpoint_dir is None:
            checkpoint_dir = str(MOMASK_DIR / 'checkpoints')
        self.checkpoint_dir = checkpoint_dir

        self._loaded = False
        self.vq_model = None
        self.t2m_transformer = None
        self.res_model = None
        self.length_estimator = None
        self.mean = None
        self.std = None

    def _load_models(self):
        """Lazy-load all MoMask models on first use."""
        if self._loaded:
            return

        print("Loading MoMask models...")

        from utils.get_opt import get_opt
        from models.vq.model import RVQVAE, LengthEstimator
        from models.mask_transformer.transformer import MaskTransformer, ResidualTransformer

        dataset_name = 't2m'
        dim_pose = 263  # HumanML3D

        # VQ-VAE
        vq_name = 'rvq_nq6_dc512_nc512_noshare_qdp0.2'
        vq_opt_path = os.path.join(self.checkpoint_dir, dataset_name, vq_name, 'opt.txt')
        vq_opt = get_opt(vq_opt_path, device=self.device)
        vq_opt.dim_pose = dim_pose

        self.vq_model = RVQVAE(
            vq_opt, vq_opt.dim_pose, vq_opt.nb_code, vq_opt.code_dim,
            vq_opt.output_emb_width, vq_opt.down_t, vq_opt.stride_t,
            vq_opt.width, vq_opt.depth, vq_opt.dilation_growth_rate,
            vq_opt.vq_act, vq_opt.vq_norm
        )
        ckpt = torch.load(
            os.path.join(self.checkpoint_dir, dataset_name, vq_name, 'model', 'net_best_fid.tar'),
            map_location='cpu'
        )
        key = 'vq_model' if 'vq_model' in ckpt else 'net'
        self.vq_model.load_state_dict(ckpt[key])
        print(f"  VQ-VAE loaded ({vq_name})")

        # Mask Transformer
        trans_name = 't2m_nlayer8_nhead6_ld384_ff1024_cdp0.1_rvq6ns'
        model_opt_path = os.path.join(self.checkpoint_dir, dataset_name, trans_name, 'opt.txt')
        model_opt = get_opt(model_opt_path, device=self.device)
        model_opt.num_tokens = vq_opt.nb_code
        model_opt.num_quantizers = vq_opt.num_quantizers
        model_opt.code_dim = vq_opt.code_dim

        self.t2m_transformer = MaskTransformer(
            code_dim=model_opt.code_dim, cond_mode='text',
            latent_dim=model_opt.latent_dim, ff_size=model_opt.ff_size,
            num_layers=model_opt.n_layers, num_heads=model_opt.n_heads,
            dropout=model_opt.dropout, clip_dim=512,
            cond_drop_prob=model_opt.cond_drop_prob,
            clip_version='ViT-B/32', opt=model_opt
        )
        ckpt = torch.load(
            os.path.join(self.checkpoint_dir, dataset_name, trans_name, 'model', 'latest.tar'),
            map_location='cpu'
        )
        key = 't2m_transformer' if 't2m_transformer' in ckpt else 'trans'
        self.t2m_transformer.load_state_dict(ckpt[key], strict=False)
        print(f"  Mask Transformer loaded ({trans_name})")

        # Residual Transformer
        res_name = 'tres_nlayer8_ld384_ff1024_rvq6ns_cdp0.2_sw'
        res_opt_path = os.path.join(self.checkpoint_dir, dataset_name, res_name, 'opt.txt')
        res_opt = get_opt(res_opt_path, device=self.device)
        res_opt.num_quantizers = vq_opt.num_quantizers
        res_opt.num_tokens = vq_opt.nb_code

        self.res_model = ResidualTransformer(
            code_dim=vq_opt.code_dim, cond_mode='text',
            latent_dim=res_opt.latent_dim, ff_size=res_opt.ff_size,
            num_layers=res_opt.n_layers, num_heads=res_opt.n_heads,
            dropout=res_opt.dropout, clip_dim=512,
            shared_codebook=vq_opt.shared_codebook,
            cond_drop_prob=res_opt.cond_drop_prob,
            share_weight=res_opt.share_weight,
            clip_version='ViT-B/32', opt=res_opt
        )
        ckpt = torch.load(
            os.path.join(self.checkpoint_dir, dataset_name, res_name, 'model', 'net_best_fid.tar'),
            map_location=self.device
        )
        self.res_model.load_state_dict(ckpt['res_transformer'], strict=False)
        print(f"  Residual Transformer loaded ({res_name})")

        # Length estimator
        self.length_estimator = LengthEstimator(512, 50)
        ckpt = torch.load(
            os.path.join(self.checkpoint_dir, dataset_name, 'length_estimator', 'model', 'finest.tar'),
            map_location=self.device
        )
        self.length_estimator.load_state_dict(ckpt['estimator'])
        print("  Length Estimator loaded")

        # Normalization stats
        meta_dir = os.path.join(self.checkpoint_dir, dataset_name, vq_name, 'meta')
        self.mean = np.load(os.path.join(meta_dir, 'mean.npy'))
        self.std = np.load(os.path.join(meta_dir, 'std.npy'))

        # Move to device and eval mode
        self.vq_model.to(self.device).eval()
        self.t2m_transformer.to(self.device).eval()
        self.res_model.to(self.device).eval()
        self.length_estimator.to(self.device).eval()

        self._loaded = True
        print("MoMask ready.")

    def generate_raw(self, prompt, duration=None, seed=42):
        """Generate raw HumanML3D motion data from a text prompt.

        Args:
            prompt: Text description of the motion
            duration: Duration in seconds (None = auto-estimate)
            seed: Random seed for reproducibility

        Returns:
            (joint_positions, n_frames): numpy array (N, 22, 3) and frame count
        """
        self._load_models()

        from utils.fixseed import fixseed
        from utils.motion_process import recover_from_ric
        from torch.distributions.categorical import Categorical

        fixseed(seed)

        with torch.no_grad():
            # Estimate or set motion length
            if duration is None:
                text_embedding = self.t2m_transformer.encode_text([prompt])
                pred_dis = self.length_estimator(text_embedding)
                probs = F.softmax(pred_dis, dim=-1)
                token_lens = Categorical(probs).sample()
            else:
                # Duration in seconds → frames at 20fps → tokens (÷4)
                n_frames = int(duration * 20)
                token_lens = torch.LongTensor([n_frames // 4]).to(self.device)

            m_length = token_lens * 4

            # Generate motion tokens
            mids = self.t2m_transformer.generate(
                [prompt], token_lens,
                timesteps=18,
                cond_scale=4,
                temperature=1.0,
                topk_filter_thres=0.9,
                gsample=True,
            )

            # Refine with residual transformer
            mids = self.res_model.generate(
                mids, [prompt], token_lens,
                temperature=1, cond_scale=5
            )

            # Decode to motion
            pred_motions = self.vq_model.forward_decoder(mids)
            pred_motions = pred_motions.detach().cpu().numpy()

            # Denormalize
            data = pred_motions * self.std + self.mean

            # Recover joint positions from ric (rotation-invariant coordinates)
            joint_data = data[0, :m_length[0].item()]
            joints = recover_from_ric(
                torch.from_numpy(joint_data).float(), 22
            ).numpy()

        return joints, m_length[0].item()

    def generate(self, prompt, duration=15.0, fps=24, total_frames=360, seed=42):
        """Generate Mixamo-compatible bone rotations from a text prompt.

        Args:
            prompt: Text description (e.g. "a knight draws a sword and attacks")
            duration: Desired duration in seconds
            fps: Output fps (24 for the pose editor)
            total_frames: Total output frames (360 = 15s at 24fps)
            seed: Random seed

        Returns:
            List of dicts, one per frame: [{ bone_name: (rx, ry, rz), ... }, ...]
        """
        joints, n_frames = self.generate_raw(prompt, duration=duration, seed=seed)
        print(f"  Generated {n_frames} frames at 20fps from: \"{prompt}\"")

        # Retarget to Mixamo skeleton and resample to target fps
        frames = retarget(joints, fps_in=20, fps_out=fps, total_frames=total_frames)
        print(f"  Retargeted to {len(frames)} frames at {fps}fps, "
              f"{len(frames[0])} Mixamo bones")

        return frames

    def generate_beats(self, beat_prompts, fps=24, blend_frames=6):
        """Generate a multi-beat animation from separate prompts per beat.

        Args:
            beat_prompts: List of (prompt, duration_seconds) tuples.
                e.g. [("standing idle", 3), ("draws sword", 3), ...]
            fps: Output fps
            blend_frames: Number of frames to crossfade at beat boundaries

        Returns:
            List of frame dicts for the entire sequence, with smooth
            crossfade transitions between beats.
        """
        beat_lists = []
        for i, (prompt, duration) in enumerate(beat_prompts):
            n_frames = int(duration * fps)
            beat_frames = self.generate(
                prompt, duration=duration, fps=fps,
                total_frames=n_frames, seed=42 + i
            )
            beat_lists.append(beat_frames)
            print(f"  Beat {i+1}/{len(beat_prompts)}: {len(beat_frames)} frames")

        # Crossfade blend at beat boundaries
        if blend_frames > 0 and len(beat_lists) > 1:
            all_frames = blend_beat_sequences(beat_lists, blend_frames=blend_frames)
            blended_count = sum(len(b) for b in beat_lists) - len(all_frames)
            print(f"  Blended {blended_count} overlap frames across "
                  f"{len(beat_lists)-1} transitions")
        else:
            all_frames = []
            for bl in beat_lists:
                all_frames.extend(bl)

        return all_frames


# ============================================================
# CLI
# ============================================================

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python motion_generator.py 'text prompt' [duration_secs]")
        print("Example: python motion_generator.py 'a person draws a sword and slashes' 10")
        sys.exit(0)

    prompt = sys.argv[1]
    duration = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0

    gen = MotionGenerator()
    frames = gen.generate(prompt, duration=duration)

    print(f"\nGenerated {len(frames)} frames")
    print(f"Bones: {list(frames[0].keys())[:5]}...")
    print(f"Frame 0 RightArm: {frames[0].get('mixamorigRightArm', 'N/A')}")
    print(f"Frame {len(frames)//2} RightArm: {frames[len(frames)//2].get('mixamorigRightArm', 'N/A')}")
