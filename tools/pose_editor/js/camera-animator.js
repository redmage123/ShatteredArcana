/**
 * camera-animator.js — Camera keyframes with smoothstep interpolation
 */
import { THREE, camera, controls } from './core.js';

// { frame: { px, py, pz, tx, ty, tz, fov } }
const camKeyframes = {};

function smoothstep(t) { return t * t * (3 - 2 * t); }

function setCameraKeyframe(frame) {
  camKeyframes[frame] = {
    px: camera.position.x, py: camera.position.y, pz: camera.position.z,
    tx: controls.target.x, ty: controls.target.y, tz: controls.target.z,
    fov: camera.fov,
  };
}

function deleteCameraKeyframe(frame) { delete camKeyframes[frame]; }

function setCameraPosition(x, y, z) {
  camera.position.set(x, y, z);
  controls.update();
}

function setCameraTarget(tx, ty, tz) {
  controls.target.set(tx, ty, tz);
  controls.update();
}

function setCameraFOV(fov) {
  camera.fov = fov;
  camera.updateProjectionMatrix();
}

function getCameraState() {
  return {
    px: camera.position.x, py: camera.position.y, pz: camera.position.z,
    tx: controls.target.x, ty: controls.target.y, tz: controls.target.z,
    fov: camera.fov,
  };
}

function evaluateAtFrame(frame) {
  const frames = Object.keys(camKeyframes).map(Number).sort((a, b) => a - b);
  if (frames.length === 0) return false;

  let prev = null, next = null;
  for (let i = 0; i < frames.length; i++) {
    if (frames[i] <= frame) prev = frames[i];
    if (frames[i] >= frame && next === null) next = frames[i];
  }

  if (prev !== null && next !== null && prev !== next) {
    const t = smoothstep((frame - prev) / (next - prev));
    const p = camKeyframes[prev], n = camKeyframes[next];
    camera.position.set(
      p.px + (n.px - p.px) * t,
      p.py + (n.py - p.py) * t,
      p.pz + (n.pz - p.pz) * t
    );
    controls.target.set(
      p.tx + (n.tx - p.tx) * t,
      p.ty + (n.ty - p.ty) * t,
      p.tz + (n.tz - p.tz) * t
    );
    camera.fov = p.fov + (n.fov - p.fov) * t;
    camera.updateProjectionMatrix();
    controls.update();
    return true;
  } else if (prev !== null) {
    const kf = camKeyframes[prev];
    camera.position.set(kf.px, kf.py, kf.pz);
    controls.target.set(kf.tx, kf.ty, kf.tz);
    camera.fov = kf.fov;
    camera.updateProjectionMatrix();
    controls.update();
    return true;
  }
  return false;
}

function getKeyframes() { return { ...camKeyframes }; }
function clearKeyframes() { Object.keys(camKeyframes).forEach(k => delete camKeyframes[k]); }

function exportJSON() { return JSON.stringify(camKeyframes); }
function importJSON(json) {
  const data = JSON.parse(json);
  Object.assign(camKeyframes, data);
}

export {
  setCameraKeyframe, deleteCameraKeyframe,
  setCameraPosition, setCameraTarget, setCameraFOV,
  getCameraState, evaluateAtFrame,
  getKeyframes, clearKeyframes,
  exportJSON, importJSON,
};
