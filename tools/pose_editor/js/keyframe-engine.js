/**
 * keyframe-engine.js — Manual per-bone keyframe system with smoothstep interpolation
 */
import { THREE } from './core.js';
import * as rig from './rig.js';

const FPS = 24;
const TOTAL_FRAMES = 360; // 15 seconds
let currentFrame = 1;
let isPlaying = false;

// { boneName: { frame: { rx, ry, rz } } }
const keyframes = {};

// Mode: 'override' replaces clip data, 'additive' adds on top
let mode = 'override';

function setMode(m) { mode = m; }
function getMode() { return mode; }

function setKeyframe(boneName) {
  if (!boneName) {
    // Keyframe all bones
    rig.getBones().forEach(bone => {
      if (!keyframes[bone.name]) keyframes[bone.name] = {};
      keyframes[bone.name][currentFrame] = {
        rx: THREE.MathUtils.radToDeg(bone.rotation.x),
        ry: THREE.MathUtils.radToDeg(bone.rotation.y),
        rz: THREE.MathUtils.radToDeg(bone.rotation.z),
      };
    });
    return `all@${currentFrame}`;
  }
  const bone = rig.getBone(boneName);
  if (!bone) return null;
  if (!keyframes[boneName]) keyframes[boneName] = {};
  keyframes[boneName][currentFrame] = {
    rx: THREE.MathUtils.radToDeg(bone.rotation.x),
    ry: THREE.MathUtils.radToDeg(bone.rotation.y),
    rz: THREE.MathUtils.radToDeg(bone.rotation.z),
  };
  return `${boneName}@${currentFrame}`;
}

function deleteKeyframe(boneName) {
  if (boneName && keyframes[boneName]) {
    delete keyframes[boneName][currentFrame];
  }
}

function hasKeyframe(boneName, frame) {
  return !!(keyframes[boneName] && keyframes[boneName][frame]);
}

function getKeyframeFrames(boneName) {
  if (!keyframes[boneName]) return [];
  return Object.keys(keyframes[boneName]).map(Number).sort((a, b) => a - b);
}

function smoothstep(t) { return t * t * (3 - 2 * t); }

/**
 * Interpolate keyframes at a given frame.
 *
 * Only applies to bones where the frame falls WITHIN the keyframed range
 * [first_keyframe, last_keyframe]. Frames outside this range are left
 * untouched so clip animation can play through without being overridden.
 * This prevents the salute pose from "leaking" into subsequent beats.
 */
function interpolateFrame(frame) {
  for (const boneName of Object.keys(keyframes)) {
    const bone = rig.getBone(boneName);
    if (!bone) continue;
    const bk = keyframes[boneName];
    const frames = Object.keys(bk).map(Number).sort((a, b) => a - b);
    if (frames.length === 0) continue;

    const firstKF = frames[0];
    const lastKF = frames[frames.length - 1];

    // Only apply if frame is within the keyframed range
    if (frame < firstKF || frame > lastKF) continue;

    let prev = null, next = null;
    for (let i = 0; i < frames.length; i++) {
      if (frames[i] <= frame) prev = frames[i];
      if (frames[i] >= frame && next === null) next = frames[i];
    }

    let rx, ry, rz;
    if (prev !== null && next !== null && prev !== next) {
      const t = smoothstep((frame - prev) / (next - prev));
      const p = bk[prev], n = bk[next];
      rx = p.rx + (n.rx - p.rx) * t;
      ry = p.ry + (n.ry - p.ry) * t;
      rz = p.rz + (n.rz - p.rz) * t;
    } else if (prev !== null) {
      ({ rx, ry, rz } = bk[prev]);
    } else {
      continue;
    }

    if (mode === 'additive') {
      bone.rotation.set(
        bone.rotation.x + THREE.MathUtils.degToRad(rx),
        bone.rotation.y + THREE.MathUtils.degToRad(ry),
        bone.rotation.z + THREE.MathUtils.degToRad(rz),
        bone.rotation.order
      );
    } else {
      bone.rotation.set(
        THREE.MathUtils.degToRad(rx),
        THREE.MathUtils.degToRad(ry),
        THREE.MathUtils.degToRad(rz),
        bone.rotation.order
      );
    }
  }
}

function goToFrame(frame) {
  currentFrame = Math.max(1, Math.min(TOTAL_FRAMES, frame));
  interpolateFrame(currentFrame);
}

function getCurrentFrame() { return currentFrame; }
function getTotalFrames() { return TOTAL_FRAMES; }
function getFPS() { return FPS; }
function getIsPlaying() { return isPlaying; }
function setIsPlaying(v) { isPlaying = v; }

function exportJSON() { return JSON.stringify(keyframes); }

function importJSON(jsonStr) {
  const data = JSON.parse(jsonStr);
  const kf = data.keyframes || data;
  Object.keys(kf).forEach(k => keyframes[k] = kf[k]);
}

function clearKeyframes() {
  Object.keys(keyframes).forEach(k => delete keyframes[k]);
}

function getKeyframes() { return keyframes; }

/**
 * Check if any bone has keyframes that bracket the given frame.
 * Returns true only if the frame falls within [first_keyframe, last_keyframe]
 * for at least one bone. Frames outside this range are NOT affected by
 * keyframes and should not trigger mixer stopping.
 */
function hasAnyKeyframeInRange(frame) {
  for (const boneName of Object.keys(keyframes)) {
    const bk = keyframes[boneName];
    const frames = Object.keys(bk).map(Number).sort((a, b) => a - b);
    if (frames.length === 0) continue;
    const first = frames[0];
    const last = frames[frames.length - 1];
    if (frame >= first && frame <= last) return true;
  }
  return false;
}

export {
  FPS, TOTAL_FRAMES,
  setKeyframe, deleteKeyframe, hasKeyframe, getKeyframeFrames,
  hasAnyKeyframeInRange,
  interpolateFrame, goToFrame,
  getCurrentFrame, getTotalFrames, getFPS,
  getIsPlaying, setIsPlaying,
  exportJSON, importJSON, clearKeyframes, getKeyframes,
  setMode, getMode,
};
