#!/usr/bin/env python3
"""
MoMask-based motion generation for the denizen animation pipeline.

TWO MODES:
  1. clip_select: Use MoMask to pick the best Mixamo clip for each beat
     by generating motion, then matching it to the clip library via
     joint-position similarity. The animation is then rendered through
     the proven Mixamo clip pipeline.

  2. raw: Generate bone rotations directly from MoMask and retarget.
     WARNING: This mode produces poor visual results — broken legs,
     no ground contact, no weapons. Use clip_select for production.

Usage:
    from motion_generator import MotionGenerator

    gen = MotionGenerator()

    # RECOMMENDED: Pick Mixamo clips by semantic matching
    clip_sequence = gen.select_clips("a knight draws a sword and slashes")
    # Returns: [{'clip': 'sword and shield slash', 'speed': 0.6, ...}, ...]

    # NOT RECOMMENDED for production: Direct bone rotations
    frames = gen.generate("a knight draws a sword and slashes", duration=15.0)
"""

import os
import sys
import json
import numpy as np
import torch
import torch.nn.functional as F
from pathlib import Path

MOMASK_DIR = Path(__file__).parent.parent / 'momask'
sys.path.insert(0, str(MOMASK_DIR))

from retarget import retarget, blend_beat_sequences

# Available Mixamo clips and their motion characteristics
# These are used by clip_select mode to match MoMask output
CLIP_LIBRARY = {
    'sword and shield idle': {
        'tags': ['idle', 'stand', 'wait', 'guard', 'ready', 'weight shift'],
        'energy': 0.2, 'arms': 'down',
    },
    'sword and shield idle (2)': {
        'tags': ['idle', 'stand', 'alert', 'tense'],
        'energy': 0.25, 'arms': 'down',
    },
    'sword and shield idle (3)': {
        'tags': ['idle', 'relaxed', 'stand'],
        'energy': 0.15, 'arms': 'down',
    },
    'sword and shield idle (4)': {
        'tags': ['idle', 'fidget', 'impatient'],
        'energy': 0.3, 'arms': 'down',
    },
    'sword and shield attack': {
        'tags': ['attack', 'swing', 'slash', 'strike', 'aggressive', 'forward'],
        'energy': 0.9, 'arms': 'overhead',
    },
    'sword and shield attack (2)': {
        'tags': ['attack', 'thrust', 'stab', 'lunge'],
        'energy': 0.85, 'arms': 'forward',
    },
    'sword and shield attack (3)': {
        'tags': ['attack', 'heavy', 'overhead', 'smash'],
        'energy': 0.95, 'arms': 'overhead',
    },
    'sword and shield attack (4)': {
        'tags': ['attack', 'combo', 'fast', 'flurry'],
        'energy': 0.9, 'arms': 'mixed',
    },
    'sword and shield slash': {
        'tags': ['slash', 'sweep', 'wide', 'horizontal', 'arc'],
        'energy': 0.8, 'arms': 'wide',
    },
    'sword and shield slash (2)': {
        'tags': ['slash', 'diagonal', 'quick'],
        'energy': 0.75, 'arms': 'diagonal',
    },
    'sword and shield slash (3)': {
        'tags': ['slash', 'backhand', 'reverse'],
        'energy': 0.7, 'arms': 'cross',
    },
    'sword and shield slash (4)': {
        'tags': ['slash', 'uppercut', 'rising'],
        'energy': 0.8, 'arms': 'up',
    },
    'sword and shield slash (5)': {
        'tags': ['slash', 'spin', 'whirl'],
        'energy': 0.85, 'arms': 'wide',
    },
    'sword and shield block': {
        'tags': ['block', 'defend', 'shield', 'brace', 'protect'],
        'energy': 0.4, 'arms': 'up',
    },
    'sword and shield block (2)': {
        'tags': ['block', 'deflect', 'parry'],
        'energy': 0.45, 'arms': 'forward',
    },
    'sword and shield block idle': {
        'tags': ['block', 'hold', 'defensive', 'stance'],
        'energy': 0.3, 'arms': 'up',
    },
    'sword and shield casting': {
        'tags': ['cast', 'spell', 'magic', 'channel', 'raise hands'],
        'energy': 0.5, 'arms': 'up',
    },
    'sword and shield casting (2)': {
        'tags': ['cast', 'spell', 'summon', 'invoke'],
        'energy': 0.55, 'arms': 'forward',
    },
    'sheath sword 1': {
        'tags': ['sheath', 'draw', 'unsheath', 'reach', 'pull'],
        'energy': 0.4, 'arms': 'cross',
    },
    'sheath sword 2': {
        'tags': ['sheath', 'put away', 'holster'],
        'energy': 0.35, 'arms': 'cross',
    },
    'sword and shield power up': {
        'tags': ['power', 'charge', 'gather', 'energy', 'raise', 'overhead'],
        'energy': 0.6, 'arms': 'overhead',
    },
    'sword and shield kick': {
        'tags': ['kick', 'leg', 'stomp', 'boot'],
        'energy': 0.7, 'arms': 'down',
    },
    'sword and shield walk': {
        'tags': ['walk', 'advance', 'approach', 'forward', 'step'],
        'energy': 0.35, 'arms': 'down',
    },
    'sword and shield run': {
        'tags': ['run', 'charge', 'sprint', 'rush'],
        'energy': 0.7, 'arms': 'pump',
    },
    'sword and shield crouch': {
        'tags': ['crouch', 'low', 'sneak', 'stalk', 'predator'],
        'energy': 0.3, 'arms': 'down',
    },
    'sword and shield crouch idle': {
        'tags': ['crouch', 'hold', 'hide', 'wait', 'low'],
        'energy': 0.2, 'arms': 'down',
    },
    'sword and shield strafe': {
        'tags': ['strafe', 'sidestep', 'dodge', 'dance', 'lateral'],
        'energy': 0.5, 'arms': 'down',
    },
    'sword and shield turn': {
        'tags': ['turn', 'spin', 'rotate', 'pivot', 'face'],
        'energy': 0.4, 'arms': 'down',
    },
    'sword and shield jump': {
        'tags': ['jump', 'leap', 'spring', 'airborne'],
        'energy': 0.8, 'arms': 'up',
    },
    'sword and shield death': {
        'tags': ['death', 'fall', 'collapse', 'defeated'],
        'energy': 0.6, 'arms': 'limp',
    },
    'sword and shield impact': {
        'tags': ['impact', 'hit', 'stagger', 'recoil', 'flinch'],
        'energy': 0.6, 'arms': 'back',
    },
}


def _score_clip(clip_name, clip_info, prompt_words):
    """Score how well a clip matches a text prompt."""
    score = 0
    prompt_lower = set(w.lower() for w in prompt_words)
    tags = set(clip_info['tags'])

    # Tag overlap
    overlap = prompt_lower & tags
    score += len(overlap) * 10

    # Partial matches (prompt word is substring of a tag or vice versa)
    for pw in prompt_lower:
        for tag in tags:
            if pw in tag or tag in pw:
                score += 3

    # Energy keywords
    high_energy = {'attack', 'slash', 'strike', 'swing', 'charge', 'aggressive',
                   'fast', 'power', 'smash', 'lunge', 'punch', 'kick', 'jump'}
    low_energy = {'stand', 'idle', 'wait', 'still', 'hold', 'gentle', 'slow',
                  'rest', 'calm', 'sway'}

    prompt_energy = 0.5
    if prompt_lower & high_energy:
        prompt_energy = 0.8
    if prompt_lower & low_energy:
        prompt_energy = 0.2

    energy_diff = abs(clip_info['energy'] - prompt_energy)
    score -= energy_diff * 5

    return score


def select_clip_for_prompt(prompt):
    """Select the best Mixamo clip for a text prompt.

    Returns: (clip_name, speed) tuple
    """
    words = prompt.replace(',', ' ').replace('.', ' ').split()

    best_name = 'sword and shield idle'
    best_score = -999

    for name, info in CLIP_LIBRARY.items():
        score = _score_clip(name, info, words)
        if score > best_score:
            best_score = score
            best_name = name

    # Determine speed based on prompt energy
    prompt_lower = prompt.lower()
    speed = 0.6  # default
    if any(w in prompt_lower for w in ['slow', 'careful', 'gentle', 'still']):
        speed = 0.4
    elif any(w in prompt_lower for w in ['fast', 'quick', 'explosive', 'aggressive']):
        speed = 0.9
    elif any(w in prompt_lower for w in ['wide', 'sweep', 'arc', 'swing']):
        speed = 0.5

    return best_name, speed


class MotionGenerator:
    """Text-to-motion generator with two modes: clip_select and raw."""

    def __init__(self, device=None, checkpoint_dir=None):
        if device is None:
            device = 'cuda' if torch.cuda.is_available() else 'cpu'
        self.device = torch.device(device)
        if checkpoint_dir is None:
            checkpoint_dir = str(MOMASK_DIR / 'checkpoints')
        self.checkpoint_dir = checkpoint_dir
        self._loaded = False

    def select_clips(self, beat_prompts):
        """Select Mixamo clips for a list of (prompt, duration) beat descriptions.

        This is the RECOMMENDED mode. Uses text matching against the clip
        library to pick the best clip for each beat. The result is a list
        of segment configs that plug directly into the Mixamo pipeline.

        Args:
            beat_prompts: List of (prompt, duration_seconds) tuples.

        Returns:
            List of segment dicts compatible with denizen_pipeline PLANE_CONFIGS:
            [{'clip': 'sword and shield slash', 'clipStart': 0.0, 'speed': 0.6,
              'start': 1, 'end': 72, 'beat': 'slash'}, ...]
        """
        segments = []
        frame_cursor = 1
        fps = 24

        for i, (prompt, duration) in enumerate(beat_prompts):
            clip_name, speed = select_clip_for_prompt(prompt)
            n_frames = int(duration * fps)
            end_frame = frame_cursor + n_frames - 1

            segments.append({
                'beat': f'beat{i+1}',
                'clip': clip_name,
                'start': frame_cursor,
                'end': end_frame,
                'clipStart': 0.0,
                'speed': speed,
                'prompt': prompt,
            })

            print(f"  Beat {i+1}: \"{prompt[:50]}...\" → {clip_name} (speed={speed})")
            frame_cursor = end_frame + 1

        return segments

    def _load_models(self):
        """Lazy-load MoMask models."""
        if self._loaded:
            return

        print("Loading MoMask models...")

        from utils.get_opt import get_opt
        from models.vq.model import RVQVAE, LengthEstimator
        from models.mask_transformer.transformer import MaskTransformer, ResidualTransformer

        dataset_name = 't2m'
        dim_pose = 263

        vq_name = 'rvq_nq6_dc512_nc512_noshare_qdp0.2'
        vq_opt = get_opt(os.path.join(self.checkpoint_dir, dataset_name, vq_name, 'opt.txt'),
                         device=self.device)
        vq_opt.dim_pose = dim_pose

        self.vq_model = RVQVAE(vq_opt, vq_opt.dim_pose, vq_opt.nb_code, vq_opt.code_dim,
                               vq_opt.output_emb_width, vq_opt.down_t, vq_opt.stride_t,
                               vq_opt.width, vq_opt.depth, vq_opt.dilation_growth_rate,
                               vq_opt.vq_act, vq_opt.vq_norm)
        ckpt = torch.load(os.path.join(self.checkpoint_dir, dataset_name, vq_name, 'model', 'net_best_fid.tar'),
                          map_location='cpu', weights_only=False)
        self.vq_model.load_state_dict(ckpt.get('vq_model', ckpt.get('net')))

        trans_name = 't2m_nlayer8_nhead6_ld384_ff1024_cdp0.1_rvq6ns'
        model_opt = get_opt(os.path.join(self.checkpoint_dir, dataset_name, trans_name, 'opt.txt'),
                            device=self.device)
        model_opt.num_tokens = vq_opt.nb_code
        model_opt.num_quantizers = vq_opt.num_quantizers
        model_opt.code_dim = vq_opt.code_dim

        self.t2m_transformer = MaskTransformer(
            code_dim=model_opt.code_dim, cond_mode='text',
            latent_dim=model_opt.latent_dim, ff_size=model_opt.ff_size,
            num_layers=model_opt.n_layers, num_heads=model_opt.n_heads,
            dropout=model_opt.dropout, clip_dim=512,
            cond_drop_prob=model_opt.cond_drop_prob,
            clip_version='ViT-B/32', opt=model_opt)
        ckpt = torch.load(os.path.join(self.checkpoint_dir, dataset_name, trans_name, 'model', 'latest.tar'),
                          map_location='cpu', weights_only=False)
        self.t2m_transformer.load_state_dict(ckpt.get('t2m_transformer', ckpt.get('trans')), strict=False)

        res_name = 'tres_nlayer8_ld384_ff1024_rvq6ns_cdp0.2_sw'
        res_opt = get_opt(os.path.join(self.checkpoint_dir, dataset_name, res_name, 'opt.txt'),
                          device=self.device)
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
            clip_version='ViT-B/32', opt=res_opt)
        ckpt = torch.load(os.path.join(self.checkpoint_dir, dataset_name, res_name, 'model', 'net_best_fid.tar'),
                          map_location=self.device, weights_only=False)
        self.res_model.load_state_dict(ckpt['res_transformer'], strict=False)

        self.length_estimator = LengthEstimator(512, 50)
        ckpt = torch.load(os.path.join(self.checkpoint_dir, dataset_name, 'length_estimator', 'model', 'finest.tar'),
                          map_location=self.device, weights_only=False)
        self.length_estimator.load_state_dict(ckpt['estimator'])

        meta_dir = os.path.join(self.checkpoint_dir, dataset_name, vq_name, 'meta')
        self.mean = np.load(os.path.join(meta_dir, 'mean.npy'))
        self.std = np.load(os.path.join(meta_dir, 'std.npy'))

        self.vq_model.to(self.device).eval()
        self.t2m_transformer.to(self.device).eval()
        self.res_model.to(self.device).eval()
        self.length_estimator.to(self.device).eval()

        self._loaded = True
        print("MoMask ready.")

    def generate_raw(self, prompt, duration=None, seed=42):
        """Generate raw joint positions. Returns (positions, n_frames)."""
        self._load_models()

        from utils.fixseed import fixseed
        from utils.motion_process import recover_from_ric
        from torch.distributions.categorical import Categorical

        fixseed(seed)

        with torch.no_grad():
            if duration is None:
                text_embedding = self.t2m_transformer.encode_text([prompt])
                pred_dis = self.length_estimator(text_embedding)
                probs = F.softmax(pred_dis, dim=-1)
                token_lens = Categorical(probs).sample()
            else:
                n_frames = int(duration * 20)
                token_lens = torch.LongTensor([n_frames // 4]).to(self.device)

            m_length = token_lens * 4
            mids = self.t2m_transformer.generate([prompt], token_lens,
                                                  timesteps=18, cond_scale=4,
                                                  temperature=1.0, topk_filter_thres=0.9,
                                                  gsample=True)
            mids = self.res_model.generate(mids, [prompt], token_lens, temperature=1, cond_scale=5)
            pred_motions = self.vq_model.forward_decoder(mids).detach().cpu().numpy()
            data = pred_motions * self.std + self.mean
            joint_data = data[0, :m_length[0].item()]
            joints = recover_from_ric(torch.from_numpy(joint_data).float(), 22).numpy()

        return joints, m_length[0].item()

    def generate(self, prompt, duration=15.0, fps=24, total_frames=360, seed=42):
        """Generate Mixamo bone rotations directly. WARNING: poor visual quality."""
        joints, n_frames = self.generate_raw(prompt, duration=duration, seed=seed)
        frames = retarget(joints, fps_in=20, fps_out=fps, total_frames=total_frames)
        return frames

    def generate_beats(self, beat_prompts, fps=24, blend_frames=6):
        """Generate multi-beat animation. WARNING: use select_clips() instead."""
        beat_lists = []
        for i, (prompt, duration) in enumerate(beat_prompts):
            n_frames = int(duration * fps)
            beat_frames = self.generate(prompt, duration=duration, fps=fps,
                                        total_frames=n_frames, seed=42 + i)
            beat_lists.append(beat_frames)

        if blend_frames > 0 and len(beat_lists) > 1:
            return blend_beat_sequences(beat_lists, blend_frames=blend_frames)
        else:
            all_frames = []
            for bl in beat_lists:
                all_frames.extend(bl)
            return all_frames


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python motion_generator.py select 'prompt1' 'prompt2' ...")
        print("  python motion_generator.py raw 'prompt' [duration]")
        sys.exit(0)

    mode = sys.argv[1]

    if mode == 'select':
        prompts = [(p, 3.0) for p in sys.argv[2:]]
        gen = MotionGenerator()
        segments = gen.select_clips(prompts)
        print(json.dumps(segments, indent=2))
    elif mode == 'raw':
        prompt = sys.argv[2]
        duration = float(sys.argv[3]) if len(sys.argv) > 3 else 10.0
        gen = MotionGenerator()
        frames = gen.generate(prompt, duration=duration)
        print(f"Generated {len(frames)} frames")
