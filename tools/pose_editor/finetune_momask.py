#!/usr/bin/env python3
"""
Fine-tune MoMask on the Aurelith knight animation dataset.

Two-stage fine-tuning:
  1. VQ-VAE: Learn to tokenize our combat motion style
  2. Transformer: Learn to generate combat tokens from text

Uses low learning rate and few epochs to specialize without
catastrophic forgetting of the base HumanML3D capabilities.

Usage:
    python3 finetune_momask.py            # Both stages
    python3 finetune_momask.py --vq-only  # VQ-VAE only
    python3 finetune_momask.py --test     # Test generation after fine-tuning
"""

import os
import sys
import glob
import numpy as np
import torch
import torch.nn.functional as F
from torch.utils.data import Dataset, DataLoader
from pathlib import Path

MOMASK_DIR = Path(__file__).parent.parent / 'momask'
sys.path.insert(0, str(MOMASK_DIR))

DATA_DIR = MOMASK_DIR / 'finetune_data' / 'aurelith'
CHECKPOINT_DIR = MOMASK_DIR / 'checkpoints' / 't2m'
FINETUNE_DIR = MOMASK_DIR / 'checkpoints' / 't2m_finetuned'


class KnightMotionDataset(Dataset):
    """Simple dataset loader for our fine-tuning data."""

    def __init__(self, data_dir, split='train', max_len=200):
        self.data_dir = Path(data_dir)
        self.max_len = max_len

        split_file = self.data_dir / f'{split}.txt'
        with open(split_file) as f:
            self.indices = [line.strip() for line in f if line.strip()]

        self.motions = []
        self.captions = []
        self.lengths = []

        for idx in self.indices:
            motion_path = self.data_dir / 'new_joints' / f'{idx}.npy'
            text_path = self.data_dir / 'texts' / f'{idx}.txt'

            motion = np.load(str(motion_path))
            with open(str(text_path)) as f:
                line = f.readline().strip()
                caption = line.split('#')[0]

            # Pad or truncate to max_len
            n = motion.shape[0]
            if n > max_len:
                motion = motion[:max_len]
                n = max_len
            elif n < max_len:
                pad = np.zeros((max_len - n, motion.shape[1]))
                motion = np.concatenate([motion, pad], axis=0)

            self.motions.append(torch.FloatTensor(motion))
            self.captions.append(caption)
            self.lengths.append(n)

    def __len__(self):
        return len(self.motions)

    def __getitem__(self, idx):
        return self.motions[idx], self.captions[idx], self.lengths[idx]


def finetune_vq(epochs=50, lr=1e-5, batch_size=4):
    """Fine-tune the VQ-VAE on our knight motion data.

    Low learning rate to avoid catastrophic forgetting.
    The VQ-VAE learns to tokenize our combat-style motions.
    """
    from utils.get_opt import get_opt
    from models.vq.model import RVQVAE

    print("\n=== STAGE 1: Fine-tuning VQ-VAE ===")

    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

    # Load pretrained VQ-VAE
    vq_name = 'rvq_nq6_dc512_nc512_noshare_qdp0.2'
    vq_opt_path = str(CHECKPOINT_DIR / vq_name / 'opt.txt')
    vq_opt = get_opt(vq_opt_path, device=device)
    vq_opt.dim_pose = 263

    vq_model = RVQVAE(
        vq_opt, vq_opt.dim_pose, vq_opt.nb_code, vq_opt.code_dim,
        vq_opt.output_emb_width, vq_opt.down_t, vq_opt.stride_t,
        vq_opt.width, vq_opt.depth, vq_opt.dilation_growth_rate,
        vq_opt.vq_act, vq_opt.vq_norm
    )

    ckpt = torch.load(
        str(CHECKPOINT_DIR / vq_name / 'model' / 'net_best_fid.tar'),
        map_location='cpu', weights_only=False
    )
    key = 'vq_model' if 'vq_model' in ckpt else 'net'
    vq_model.load_state_dict(ckpt[key])
    vq_model.to(device)
    vq_model.train()

    print(f"  Loaded VQ-VAE: {vq_name}")

    # Dataset
    dataset = KnightMotionDataset(DATA_DIR, split='train')
    loader = DataLoader(dataset, batch_size=batch_size, shuffle=True)
    print(f"  Dataset: {len(dataset)} samples")

    # Optimizer — low LR for fine-tuning
    optimizer = torch.optim.AdamW(vq_model.parameters(), lr=lr, weight_decay=0.01)

    # Training loop
    best_loss = float('inf')
    for epoch in range(epochs):
        total_loss = 0
        for batch_motion, batch_caption, batch_len in loader:
            batch_motion = batch_motion.to(device)

            # VQ-VAE forward: encode → quantize → decode
            pred, loss_commit, perplexity = vq_model(batch_motion)

            # Reconstruction loss
            loss_recon = F.mse_loss(pred, batch_motion)
            loss = loss_recon + loss_commit.mean() * 0.02

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(vq_model.parameters(), 1.0)
            optimizer.step()

            total_loss += loss.item()

        avg_loss = total_loss / max(len(loader), 1)
        if (epoch + 1) % 10 == 0 or epoch == 0:
            print(f"  Epoch {epoch+1}/{epochs}: loss={avg_loss:.4f}")

        if avg_loss < best_loss:
            best_loss = avg_loss

    # Save fine-tuned VQ-VAE
    os.makedirs(str(FINETUNE_DIR / vq_name / 'model'), exist_ok=True)
    os.makedirs(str(FINETUNE_DIR / vq_name / 'meta'), exist_ok=True)

    save_dict = {'vq_model': vq_model.state_dict(), 'ep': epochs}
    torch.save(save_dict, str(FINETUNE_DIR / vq_name / 'model' / 'net_best_fid.tar'))

    # Copy meta files (mean/std)
    import shutil
    for f in ['mean.npy', 'std.npy']:
        src = CHECKPOINT_DIR / vq_name / 'meta' / f
        dst = FINETUNE_DIR / vq_name / 'meta' / f
        if src.exists():
            shutil.copy(str(src), str(dst))

    # Copy opt.txt
    shutil.copy(str(CHECKPOINT_DIR / vq_name / 'opt.txt'),
                str(FINETUNE_DIR / vq_name / 'opt.txt'))

    print(f"  Saved fine-tuned VQ-VAE to {FINETUNE_DIR / vq_name}")
    print(f"  Best loss: {best_loss:.4f}")

    return vq_model


def finetune_transformer(epochs=30, lr=5e-6, batch_size=4):
    """Fine-tune the Mask Transformer on our knight data.

    Even lower LR than VQ since the transformer is more sensitive.
    """
    from utils.get_opt import get_opt
    from models.vq.model import RVQVAE
    from models.mask_transformer.transformer import MaskTransformer

    print("\n=== STAGE 2: Fine-tuning Mask Transformer ===")

    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

    # Load VQ-VAE (use fine-tuned if available)
    vq_name = 'rvq_nq6_dc512_nc512_noshare_qdp0.2'
    vq_dir = FINETUNE_DIR / vq_name if (FINETUNE_DIR / vq_name).exists() else CHECKPOINT_DIR / vq_name

    vq_opt = get_opt(str(vq_dir / 'opt.txt'), device=device)
    vq_opt.dim_pose = 263

    vq_model = RVQVAE(
        vq_opt, vq_opt.dim_pose, vq_opt.nb_code, vq_opt.code_dim,
        vq_opt.output_emb_width, vq_opt.down_t, vq_opt.stride_t,
        vq_opt.width, vq_opt.depth, vq_opt.dilation_growth_rate,
        vq_opt.vq_act, vq_opt.vq_norm
    )
    ckpt = torch.load(str(vq_dir / 'model' / 'net_best_fid.tar'),
                      map_location='cpu', weights_only=False)
    key = 'vq_model' if 'vq_model' in ckpt else 'net'
    vq_model.load_state_dict(ckpt[key])
    vq_model.to(device).eval()
    print(f"  VQ-VAE loaded from {vq_dir}")

    # Load pretrained Transformer
    trans_name = 't2m_nlayer8_nhead6_ld384_ff1024_cdp0.1_rvq6ns'
    model_opt = get_opt(str(CHECKPOINT_DIR / trans_name / 'opt.txt'), device=device)
    model_opt.num_tokens = vq_opt.nb_code
    model_opt.num_quantizers = vq_opt.num_quantizers
    model_opt.code_dim = vq_opt.code_dim

    t2m_transformer = MaskTransformer(
        code_dim=model_opt.code_dim, cond_mode='text',
        latent_dim=model_opt.latent_dim, ff_size=model_opt.ff_size,
        num_layers=model_opt.n_layers, num_heads=model_opt.n_heads,
        dropout=model_opt.dropout, clip_dim=512,
        cond_drop_prob=model_opt.cond_drop_prob,
        clip_version='ViT-B/32', opt=model_opt
    )
    ckpt = torch.load(str(CHECKPOINT_DIR / trans_name / 'model' / 'latest.tar'),
                      map_location='cpu', weights_only=False)
    key = 't2m_transformer' if 't2m_transformer' in ckpt else 'trans'
    t2m_transformer.load_state_dict(ckpt[key], strict=False)
    t2m_transformer.to(device)
    t2m_transformer.train()
    print(f"  Transformer loaded: {trans_name}")

    # Dataset
    dataset = KnightMotionDataset(DATA_DIR, split='train')
    loader = DataLoader(dataset, batch_size=batch_size, shuffle=True)
    print(f"  Dataset: {len(dataset)} samples")

    # Only fine-tune the transformer layers, freeze CLIP
    trainable_params = []
    for name, param in t2m_transformer.named_parameters():
        if 'clip_model' not in name:
            trainable_params.append(param)
        else:
            param.requires_grad = False

    optimizer = torch.optim.AdamW(trainable_params, lr=lr, weight_decay=0.01)

    # Training
    best_loss = float('inf')
    for epoch in range(epochs):
        total_loss = 0
        for batch_motion, batch_caption, batch_len in loader:
            batch_motion = batch_motion.to(device)
            batch_len_tensor = torch.LongTensor(batch_len).to(device)

            # Tokenize motion with VQ-VAE
            with torch.no_grad():
                code_idx, all_codes = vq_model.encode(batch_motion)
                # code_idx: (B, T, num_quantizers)

            # Use first quantizer tokens for the mask transformer
            m_tokens = code_idx[:, :, 0]  # (B, T)
            token_lens = batch_len_tensor // 4  # VQ downsampling factor

            # MoMask's forward() does its own BERT-style masking and returns
            # (ce_loss, pred_ids, accuracy). Pass raw text + ground truth tokens.
            captions_list = list(batch_caption)
            ce_loss, pred_id, acc = t2m_transformer.forward(
                m_tokens, captions_list, token_lens
            )
            loss = ce_loss

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(trainable_params, 1.0)
            optimizer.step()

            total_loss += loss.item()

        avg_loss = total_loss / max(len(loader), 1)
        if (epoch + 1) % 10 == 0 or epoch == 0:
            print(f"  Epoch {epoch+1}/{epochs}: loss={avg_loss:.4f}")

        if avg_loss < best_loss:
            best_loss = avg_loss

    # Save fine-tuned transformer
    os.makedirs(str(FINETUNE_DIR / trans_name / 'model'), exist_ok=True)
    save_dict = {'t2m_transformer': t2m_transformer.state_dict(), 'ep': epochs}
    torch.save(save_dict, str(FINETUNE_DIR / trans_name / 'model' / 'latest.tar'))

    import shutil
    shutil.copy(str(CHECKPOINT_DIR / trans_name / 'opt.txt'),
                str(FINETUNE_DIR / trans_name / 'opt.txt'))

    # Copy residual transformer and length estimator unchanged
    for subdir in ['tres_nlayer8_ld384_ff1024_rvq6ns_cdp0.2_sw', 'length_estimator']:
        src = CHECKPOINT_DIR / subdir
        dst = FINETUNE_DIR / subdir
        if src.exists() and not dst.exists():
            shutil.copytree(str(src), str(dst))

    print(f"  Saved fine-tuned Transformer to {FINETUNE_DIR / trans_name}")
    print(f"  Best loss: {best_loss:.4f}")


def test_generation():
    """Test the fine-tuned model with combat-specific prompts."""
    sys.path.insert(0, str(Path(__file__).parent))
    from motion_generator import MotionGenerator

    print("\n=== Testing fine-tuned model ===")

    # Use fine-tuned checkpoints — structured as finetuned/t2m/...
    finetuned_root = MOMASK_DIR / 'checkpoints' / 'finetuned'
    checkpoint_dir = str(finetuned_root) if finetuned_root.exists() else str(CHECKPOINT_DIR.parent)
    gen = MotionGenerator(checkpoint_dir=checkpoint_dir)

    test_prompts = [
        "a person stands in a fighting stance and shifts weight between feet",
        "a person swings their right arm in a wide horizontal slash",
        "a person raises their right arm straight above their head in a salute",
    ]

    for prompt in test_prompts:
        frames = gen.generate(prompt, duration=3.0, total_frames=72, seed=42)
        # Check for motion variety
        first = frames[0].get('mixamorigRightArm', (0, 0, 0))
        mid = frames[36].get('mixamorigRightArm', (0, 0, 0))
        last = frames[71].get('mixamorigRightArm', (0, 0, 0))
        print(f"\n  Prompt: \"{prompt[:60]}...\"")
        print(f"  RightArm: start={first[0]:.0f},{first[1]:.0f},{first[2]:.0f}"
              f"  mid={mid[0]:.0f},{mid[1]:.0f},{mid[2]:.0f}"
              f"  end={last[0]:.0f},{last[1]:.0f},{last[2]:.0f}")


if __name__ == '__main__':
    if '--test' in sys.argv:
        test_generation()
    elif '--vq-only' in sys.argv:
        finetune_vq()
    else:
        finetune_vq()
        finetune_transformer()
        test_generation()
