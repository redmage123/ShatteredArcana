/**
 * background.js — Environment presets for all 8 planes
 */
import { THREE, scene, registerUpdatable } from './core.js';

let particles = null;
let particleMat = null;
let particleIntensity = 1.0;

const presets = {
  aurelith: {
    bgColor: 0x0a0a1a,
    fogColor: 0x0a0a1a,
    fogDensity: 0.12,
    groundColor: 0x3a2a1a,
    particleColor: 0xdaa520,
    particleCount: 200,
    particleOpacity: 0.35,
  },
  noctharion: {
    bgColor: 0x080010,
    fogColor: 0x0a0015,
    fogDensity: 0.14,
    groundColor: 0x1a0a20,
    particleColor: 0x6622aa,
    particleCount: 150,
    particleOpacity: 0.25,
  },
  verdantis: {
    bgColor: 0x051008,
    fogColor: 0x071510,
    fogDensity: 0.10,
    groundColor: 0x1a2a10,
    particleColor: 0x44cc44,
    particleCount: 250,
    particleOpacity: 0.30,
  },
  infernyx: {
    bgColor: 0x150500,
    fogColor: 0x200800,
    fogDensity: 0.13,
    groundColor: 0x2a1005,
    particleColor: 0xff4400,
    particleCount: 300,
    particleOpacity: 0.40,
  },
  aethermist: {
    bgColor: 0x050818,
    fogColor: 0x081020,
    fogDensity: 0.11,
    groundColor: 0x101830,
    particleColor: 0x6688ff,
    particleCount: 250,
    particleOpacity: 0.30,
  },
  abyssal: {
    bgColor: 0x000a10,
    fogColor: 0x001018,
    fogDensity: 0.16,
    groundColor: 0x0a1520,
    particleColor: 0x00aaaa,
    particleCount: 120,
    particleOpacity: 0.20,
  },
  ethereal: {
    bgColor: 0x0c0c18,
    fogColor: 0x101020,
    fogDensity: 0.08,
    groundColor: 0x181828,
    particleColor: 0xccccff,
    particleCount: 300,
    particleOpacity: 0.35,
  },
  feywild: {
    bgColor: 0x080510,
    fogColor: 0x100818,
    fogDensity: 0.09,
    groundColor: 0x1a1020,
    particleColor: 0xff66cc,
    particleCount: 350,
    particleOpacity: 0.35,
  },
  dark: {
    bgColor: 0x050510,
    fogColor: 0x050510,
    fogDensity: 0.15,
    groundColor: 0x111111,
    particleColor: 0x333366,
    particleCount: 50,
    particleOpacity: 0.15,
  },
  neutral: {
    bgColor: 0x1a1a2e,
    fogColor: 0x1a1a2e,
    fogDensity: 0,
    groundColor: 0x222222,
    particleColor: 0x888888,
    particleCount: 0,
    particleOpacity: 0,
  },
};

function setBackground(preset) {
  const p = presets[preset] || presets.neutral;

  scene.background = new THREE.Color(p.bgColor);

  if (p.fogDensity > 0) {
    scene.fog = new THREE.FogExp2(p.fogColor, p.fogDensity);
  } else {
    scene.fog = null;
  }

  // Update ground plane color if it exists
  scene.traverse(child => {
    if (child.isMesh && child.name === 'ground') {
      child.material.color.setHex(p.groundColor);
    }
  });

  // Remove old particles
  if (particles && particles.parent) {
    particles.parent.remove(particles);
    particles.geometry.dispose();
    particles = null;
  }

  if (p.particleCount <= 0) return;

  const count = p.particleCount;
  const geo = new THREE.BufferGeometry();
  const positions = new Float32Array(count * 3);
  const velocities = new Float32Array(count * 3);

  for (let i = 0; i < count; i++) {
    positions[i * 3] = (Math.random() - 0.5) * 6;
    positions[i * 3 + 1] = Math.random() * 4;
    positions[i * 3 + 2] = (Math.random() - 0.5) * 6;
    velocities[i * 3] = (Math.random() - 0.5) * 0.002;
    velocities[i * 3 + 1] = Math.random() * 0.003 + 0.001;
    velocities[i * 3 + 2] = (Math.random() - 0.5) * 0.002;
  }
  geo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  geo.userData.velocities = velocities;

  particleMat = new THREE.PointsMaterial({
    color: p.particleColor,
    size: 0.03,
    transparent: true,
    opacity: p.particleOpacity * particleIntensity,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
  });

  particles = new THREE.Points(geo, particleMat);
  scene.add(particles);
}

function setParticleIntensity(v) {
  particleIntensity = v;
  if (particleMat) {
    particleMat.opacity = 0.35 * v;
  }
}

// Frame counter for deterministic particle positioning during batch render.
let particleFrame = 0;

/**
 * Update particles deterministically by frame count (not real-time dt).
 * This prevents jitter/oscillation during batch frame-by-frame rendering.
 * Each particle follows a smooth sine-wave path — no random respawn jumps.
 */
function updateParticles(dt) {
  if (!particles) return;
  particleFrame++;

  const pos = particles.geometry.attributes.position;
  const seeds = particles.geometry.userData.velocities; // reuse as per-particle seeds
  const count = pos.count;
  const t = particleFrame * 0.005; // slow time progression

  for (let i = 0; i < count; i++) {
    // Each particle has a unique seed from the velocity array
    const sx = seeds[i * 3];      // horizontal seed
    const sy = seeds[i * 3 + 1];  // vertical seed (always positive)
    const sz = seeds[i * 3 + 2];  // depth seed

    // Smooth vertical rise (loops via modulo, no sudden respawn)
    const yPhase = (t * sy * 500 + i * 0.137) % 1.0; // 0..1 cycling
    const y = yPhase * 4.0;

    // Gentle horizontal drift via sine — no random direction changes
    const x = Math.sin(t * 0.3 + i * 0.97) * 3.0 + sx * 1000;
    const z = Math.cos(t * 0.2 + i * 1.31) * 3.0 + sz * 1000;

    pos.array[i * 3]     = x;
    pos.array[i * 3 + 1] = y;
    pos.array[i * 3 + 2] = z;
  }
  pos.needsUpdate = true;
}

/**
 * Set the particle frame counter (for deterministic batch rendering).
 * Call this before forceRender() to ensure particles are at the correct frame position.
 */
function setParticleFrame(frame) {
  particleFrame = frame;
  updateParticles(0);
}

registerUpdatable(updateParticles);

export { setBackground, setParticleIntensity, setParticleFrame, presets };
