/**
 * materials.js — Knight steel plate + gold inlay materials
 *
 * Repaint the Knight D Pelegrini texture atlas at load time:
 * - Red tabard/cloak → polished steel plate
 * - Brown leather → dark steel
 * - Skin tones → steel (helmet/gauntlets)
 * - Gold trim → enhanced gold
 * - Chain mail → plate segments with gold bands
 * - Adds gold inlay filigree lines
 */
import { THREE, envMap } from './core.js';

const steelMat = new THREE.MeshStandardMaterial({
  color: 0x8899aa, metalness: 0.95, roughness: 0.08,
  envMap, envMapIntensity: 2.5,
});

const goldMat = new THREE.MeshStandardMaterial({
  color: 0xdaa520, metalness: 1.0, roughness: 0.1,
  emissive: 0x332200, emissiveIntensity: 0.15,
  envMap, envMapIntensity: 1.5,
});

const darkSteelMat = new THREE.MeshStandardMaterial({
  color: 0x2a2a3a, metalness: 0.9, roughness: 0.25, envMap,
});

const eyeMat = new THREE.MeshStandardMaterial({
  color: 0x60c0ff, emissive: 0x2080ff, emissiveIntensity: 0.8,
});

/**
 * Convert a brightness value to a steel plate color.
 * @param {number} brightness - Input brightness (0-255)
 * @param {number} warmth - 0 = cold steel, 1 = warm gold-tinted steel
 */
function toSteel(brightness, warmth = 0) {
  const base = Math.max(40, Math.min(220, brightness));
  return [
    Math.min(255, base * (0.82 + warmth * 0.15)),
    Math.min(255, base * (0.85 + warmth * 0.08)),
    Math.min(255, base * (0.95 - warmth * 0.05)),
  ];
}

/**
 * Draw a line on ImageData with the given color.
 */
function drawLine(d, W, H, x1, y1, x2, y2, w, r, g, b) {
  const dx = x2 - x1, dy = y2 - y1;
  const len = Math.sqrt(dx * dx + dy * dy);
  for (let s = 0; s <= len; s++) {
    const t = s / len;
    const cx = Math.round(x1 + dx * t), cy = Math.round(y1 + dy * t);
    for (let j = -w; j <= w; j++) {
      const px = cx + (Math.abs(dy) > Math.abs(dx) ? j : 0);
      const py = cy + (Math.abs(dx) >= Math.abs(dy) ? j : 0);
      if (px >= 0 && px < W && py >= 0 && py < H) {
        const idx = (py * W + px) * 4;
        d[idx] = r + Math.floor(Math.random() * 15);
        d[idx + 1] = g + Math.floor(Math.random() * 10);
        d[idx + 2] = b + Math.floor(Math.random() * 8);
      }
    }
  }
}

/**
 * Repaint the texture atlas: replace fabric/skin with steel plate + gold inlays.
 */
function repaintTextureAtlas(tex) {
  if (!tex?.image) return;
  const W = tex.image.width, H = tex.image.height;

  const canvas = document.createElement('canvas');
  canvas.width = W;
  canvas.height = H;
  const ctx = canvas.getContext('2d');
  ctx.drawImage(tex.image, 0, 0);
  const imgData = ctx.getImageData(0, 0, W, H);
  const d = imgData.data;

  // === Color replacement pass ===
  for (let i = 0; i < d.length; i += 4) {
    const r = d[i], g = d[i + 1], b = d[i + 2], a = d[i + 3];
    if (a < 10) continue;

    const brightness = r * 0.299 + g * 0.587 + b * 0.114;
    const maxC = Math.max(r, g, b);
    const minC = Math.min(r, g, b);
    const saturation = maxC > 0 ? (maxC - minC) / maxC : 0;

    // Red/maroon/crimson fabric → steel plate
    if (r > 50 && r >= g && r > b + 15 && saturation > 0.15 && g < 180) {
      const s = toSteel(brightness * 1.3 + 30, 0.0);
      d[i] = s[0]; d[i + 1] = s[1]; d[i + 2] = s[2];
      continue;
    }
    // Brown leather → dark steel
    if (r > 40 && r > b + 10 && g > b && saturation > 0.1 && brightness < 180
        && !(r > 170 && g > 130 && b < 90)) {
      const s = toSteel(brightness * 1.1 + 20, 0.0);
      d[i] = s[0]; d[i + 1] = s[1]; d[i + 2] = s[2];
      continue;
    }
    // Skin tones → helmet steel
    if (r > 120 && g > 70 && b > 50 && r > g && r > b
        && (r - b) > 20 && (r - b) < 140 && brightness > 80 && brightness < 230) {
      const s = toSteel(brightness * 0.9 + 50, 0.05);
      d[i] = s[0]; d[i + 1] = s[1]; d[i + 2] = s[2];
      continue;
    }
    // Dark warm tones (hair, shadows) → dark steel
    if (r > 30 && r > b && brightness < 100 && brightness > 15
        && saturation > 0.05 && r > g * 0.8) {
      const s = toSteel(brightness * 1.2 + 15, 0.0);
      d[i] = s[0]; d[i + 1] = s[1]; d[i + 2] = s[2];
      continue;
    }
    // Gold trim → enhanced gold
    if (r > 160 && g > 120 && b < 100 && r > b * 1.8) {
      d[i] = Math.min(255, 220 + Math.floor(Math.random() * 20));
      d[i + 1] = Math.min(255, 175 + Math.floor(Math.random() * 15));
      d[i + 2] = Math.min(255, 30 + Math.floor(Math.random() * 15));
      continue;
    }
    // Existing gray armor → cool steel boost
    if (saturation < 0.15 && brightness > 60 && brightness < 220) {
      d[i] = Math.min(255, d[i] * 0.92 + 10);
      d[i + 1] = Math.min(255, d[i + 1] * 0.95 + 10);
      d[i + 2] = Math.min(255, d[i + 2] * 1.02 + 12);
      continue;
    }
    // White/bright → bright steel
    if (brightness > 220) { d[i] = 210; d[i + 1] = 215; d[i + 2] = 225; continue; }
    // Eye colors → golden visor
    if ((g > r && g > b && g > 100) || (b > r && b > g && b > 100)) {
      d[i] = 200; d[i + 1] = 170; d[i + 2] = 40;
      continue;
    }
  }

  // === Gold inlay filigree ===
  const G = [218, 165, 32]; // gold
  const DS = [60, 65, 75];   // dark steel seam
  // Chest plate
  drawLine(d, W, H, 1410, 80, 1410, 920, 4, ...G);
  drawLine(d, W, H, 1200, 300, 1620, 300, 3, ...G);
  drawLine(d, W, H, 1200, 600, 1620, 600, 3, ...G);
  drawLine(d, W, H, 1100, 40, 1720, 40, 3, ...G);
  drawLine(d, W, H, 1100, 960, 1720, 960, 3, ...G);
  drawLine(d, W, H, 1100, 40, 1100, 960, 3, ...G);
  drawLine(d, W, H, 1720, 40, 1720, 960, 3, ...G);
  drawLine(d, W, H, 1150, 100, 1350, 280, 2, ...G);
  drawLine(d, W, H, 1670, 100, 1470, 280, 2, ...G);
  drawLine(d, W, H, 1150, 640, 1350, 920, 2, ...G);
  drawLine(d, W, H, 1670, 640, 1470, 920, 2, ...G);
  // Shoulder/cloak
  drawLine(d, W, H, 80, 350, 520, 350, 2, ...G);
  drawLine(d, W, H, 80, 550, 520, 550, 2, ...G);
  drawLine(d, W, H, 300, 250, 300, 650, 2, ...G);
  // Plate segmentation
  drawLine(d, W, H, 1200, 200, 1620, 200, 1, ...DS);
  drawLine(d, W, H, 1200, 450, 1620, 450, 1, ...DS);
  drawLine(d, W, H, 1200, 750, 1620, 750, 1, ...DS);
  // Chain mail → plate bands
  for (let y = 1400; y < 2048; y += 50) {
    drawLine(d, W, H, 1350, y, 2048, y, 1, ...DS);
    drawLine(d, W, H, 1350, y + 25, 2048, y + 25, 1, ...G);
  }
  // Helmet visor slit
  drawLine(d, W, H, 100, 200, 400, 200, 3, 30, 25, 20);
  drawLine(d, W, H, 100, 202, 400, 202, 1, ...G);
  drawLine(d, W, H, 100, 198, 400, 198, 1, ...G);

  ctx.putImageData(imgData, 0, 0);
  tex.image = canvas;
  tex.needsUpdate = true;
}

function applyKnightTextures(fbxModel) {
  fbxModel.traverse((child) => {
    if (!child.isMesh) return;
    child.castShadow = true;
    child.receiveShadow = true;

    if (child.material) {
      const mats = Array.isArray(child.material) ? child.material : [child.material];
      mats.forEach(mat => {
        // Defer texture repaint — the embedded image may not be decoded yet.
        // The repaint will run on first access via window.repaintArmor().
        if (mat.map && !mat._repaintScheduled) {
          mat._repaintScheduled = true;
          const tex = mat.map;
          const tryRepaint = () => {
            const img = tex.image;
            if (img && ((img.complete !== undefined && img.complete) || img instanceof HTMLCanvasElement)) {
              repaintTextureAtlas(tex);
            } else if (img) {
              img.addEventListener('load', () => repaintTextureAtlas(tex), { once: true });
            }
          };
          // Try immediately, schedule fallback
          setTimeout(tryRepaint, 100);
        }

        // Full plate armor material properties
        mat.envMap = envMap;
        mat.envMapIntensity = 2.5;
        mat.metalness = 0.95;
        mat.roughness = 0.08;
        mat.needsUpdate = true;
      });
    }

    // Fallback for untextured meshes
    const name = (child.name || '').toLowerCase();
    if (!child.material || (child.material.color && child.material.color.r === 1 && child.material.color.g === 1 && child.material.color.b === 1)) {
      child.material = name.includes('joint') || name.includes('ball') ? goldMat : steelMat;
    }
  });
}

export { steelMat, goldMat, darkSteelMat, eyeMat, applyKnightTextures, repaintTextureAtlas };
