/**
 * api.js — All window.* Playwright-callable functions
 *
 * This is the contract between the browser and the Python automation layer.
 * Every function here is callable via page.evaluate() in Playwright.
 */
import * as rig from './rig.js';
import * as kfe from './keyframe-engine.js';
import * as clipLoader from './clip-loader.js';
import * as clipSeq from './clip-sequencer.js';
import * as weapons from './weapon-system.js';
import * as camAnim from './camera-animator.js';
import * as bg from './background.js';
import { repaintTextureAtlas } from './materials.js';
import * as poseLib from './pose-library.js';
import * as undoMgr from './undo-manager.js';
import * as onionSkin from './onion-skin.js';
import * as renderPipe from './render-pipeline.js';
import { camera, controls, forceRender, setRenderSize } from './core.js';

// ── Model / Ready ──────────────────────────────────────────
window.getModelReady = () => rig.getBoneNames().length > 0;
window.getBoneMap = () => rig.boneMap;

// ── Auto-center camera ────────────────────────────────────
// Track the pelvis (root bone) world position and adjust camera
// target so the character stays centered in frame regardless of
// root motion from generated animations.
window.getCharacterCenter = () => {
  const hips = rig.getBone('mixamorigHips');
  if (!hips) return { x: 0, y: 1, z: 0 };
  const wp = new camera.position.constructor(); // THREE.Vector3
  hips.getWorldPosition(wp);
  return { x: wp.x, y: wp.y, z: wp.z };
};

window.autoCenterCamera = (baseTarget, basePos, fov) => {
  const center = window.getCharacterCenter();
  // Offset camera target to follow character's horizontal movement
  // but keep vertical target fixed (don't bob with crouch/jump)
  const tx = baseTarget[0] + center.x;
  const tz = baseTarget[2] + center.z;
  const ty = baseTarget[1]; // fixed vertical target
  camAnim.setCameraPosition(basePos[0] + center.x, basePos[1], basePos[2] + center.z);
  camAnim.setCameraTarget(tx, ty, tz);
  camAnim.setCameraFOV(fov);
};
window.repaintArmor = () => {
  let count = 0;
  let root = rig.getBones()[0];
  if (!root) return 0;
  while (root.parent && root.parent.type !== 'Scene') root = root.parent;
  root.traverse(child => {
    if (child.isMesh && child.material?.map) {
      repaintTextureAtlas(child.material.map);
      count++;
    }
  });
  return count;
};
window.getSceneInfo = () => ({
  boneCount: rig.getBoneNames().length,
  clipCount: clipLoader.listClips().length,
  segmentCount: clipSeq.getSegments().length,
  poseCount: poseLib.listPoses().length,
});

// ── Bone Selection ─────────────────────────────────────────
window.selectBone = (name) => rig.selectBone(name);
window.getSelectedBone = () => rig.getSelectedBone();

// ── Bone Rotation ──────────────────────────────────────────
window.setBoneRotation = (name, rx, ry, rz) => {
  const old = rig.getBoneRotation(name);
  const result = rig.setBoneRotation(name, rx, ry, rz);
  if (result && old) {
    undoMgr.push({
      type: 'bone_rotation', boneName: name,
      frame: kfe.getCurrentFrame(),
      oldRot: old,
      newRot: { x: rx, y: ry, z: rz },
    });
  }
  return result;
};
window.getBoneRotation = (name) => rig.getBoneRotation(name);
window.getBoneNames = () => rig.getBoneNames();
window.getBonesState = () => rig.getBonesState();
window.resetBone = (name) => rig.resetBone(name);
window.resetAllBones = () => rig.resetAllBones();

// ── Pose multiple bones at once ────────────────────────────
window.poseBones = (rotations) => {
  // rotations: { boneName: [rx, ry, rz], ... }
  for (const [name, rot] of Object.entries(rotations)) {
    if (Array.isArray(rot)) {
      rig.setBoneRotation(name, rot[0], rot[1], rot[2]);
    } else {
      rig.setBoneRotation(name, rot.x || rot.rx || 0, rot.y || rot.ry || 0, rot.z || rot.rz || 0);
    }
  }
};

// ── Frame Navigation ───────────────────────────────────────
window.goToFrame = (frame) => {
  kfe.goToFrame(frame);
  // Update UI
  const el = document.getElementById('frame-display');
  if (el) el.textContent = `Frame ${kfe.getCurrentFrame()} / ${kfe.getTotalFrames()}`;
  const slider = document.getElementById('frame-slider');
  if (slider) slider.value = kfe.getCurrentFrame();
};
window.getCurrentFrame = () => kfe.getCurrentFrame();
window.getTotalFrames = () => kfe.getTotalFrames();

// ── Keyframes ──────────────────────────────────────────────
window.setKeyframe = (boneName) => kfe.setKeyframe(boneName || null);
window.setKeyframeAll = () => kfe.setKeyframe(null);
window.deleteKeyframe = (boneName) => kfe.deleteKeyframe(boneName);
window.getKeyframes = () => kfe.exportJSON();
window.importKeyframes = (json) => kfe.importJSON(json);
window.clearKeyframes = () => kfe.clearKeyframes();
window.setKeyframeMode = (mode) => kfe.setMode(mode);

// ── Clip System ────────────────────────────────────────────
window.loadClip = (url, name) => clipLoader.loadClip(url, name);
window.listClips = () => clipLoader.listClips();
window.sampleClipAtFrame = (name, frame) => clipLoader.sampleClipAtFrame(name, frame);
window.sampleClipAtTime = (name, t) => clipLoader.sampleClipAtTime(name, t);
window.playClip = (name, opts) => clipLoader.playClip(name, opts);
window.stopAllClips = () => clipLoader.stopAllClips();

// ── Clip Sequencer ─────────────────────────────────────────
window.addClipSegment = (clipName, startFrame, endFrame, opts) =>
  clipSeq.addSegment(clipName, startFrame, endFrame, opts);
window.removeClipSegment = (index) => clipSeq.removeSegment(index);
window.clearClipSegments = () => clipSeq.clearSegments();
window.getClipSegments = () => clipSeq.getSegments();
window.evaluateSequencerAtFrame = (frame) => clipSeq.evaluateAtFrame(frame);

// ── Quaternion-space keyframe overrides ────────────────────
// Pre-computed quaternion targets for keyframed bones.
// Built once when keyframes are set, used every frame to avoid
// the Euler↔Quaternion sync issue with the AnimationMixer.
const _quatOverrides = new Map(); // boneName -> { frames: [{ frame, quat }] }

/**
 * Register a keyframe override in quaternion space.
 * Call this AFTER setting the bone rotation (while mixer is stopped)
 * to capture the quaternion value for later blending.
 */
window.captureQuatKeyframe = (boneName, frame) => {
  const bone = rig.getBone(boneName);
  if (!bone) return false;
  if (!_quatOverrides.has(boneName)) _quatOverrides.set(boneName, { frames: [] });
  const entry = _quatOverrides.get(boneName);
  // Remove existing keyframe at this frame
  entry.frames = entry.frames.filter(kf => kf.frame !== frame);
  entry.frames.push({ frame, quat: bone.quaternion.clone() });
  entry.frames.sort((a, b) => a.frame - b.frame);
  return true;
};

window.clearQuatOverrides = () => _quatOverrides.clear();

/**
 * Apply quaternion-space keyframe overrides at a given frame.
 * Interpolates between keyframed quaternions using slerp, then
 * applies directly to bone.quaternion — no Euler conversion needed,
 * so this works even while the mixer is active.
 */
function applyQuatOverrides(frame) {
  const THREE_quat = new camera.quaternion.constructor(); // reuse THREE.Quaternion
  for (const [boneName, entry] of _quatOverrides) {
    const bone = rig.getBone(boneName);
    if (!bone) continue;
    const frames = entry.frames;
    if (frames.length === 0) continue;

    const first = frames[0].frame;
    const last = frames[frames.length - 1].frame;
    // Only apply within keyframed range
    if (frame < first || frame > last) continue;

    let prev = null, next = null;
    for (const kf of frames) {
      if (kf.frame <= frame) prev = kf;
      if (kf.frame >= frame && !next) next = kf;
    }

    if (prev && next && prev.frame !== next.frame) {
      const t = (frame - prev.frame) / (next.frame - prev.frame);
      // Smoothstep interpolation
      const st = t * t * (3 - 2 * t);
      THREE_quat.copy(prev.quat).slerp(next.quat, st);
      bone.quaternion.copy(THREE_quat);
    } else if (prev) {
      bone.quaternion.copy(prev.quat);
    }
  }
}

// ── Full frame evaluation (sequencer + keyframes + camera) ─
window.evaluateFullFrame = (frame) => {
  // 1. Sequencer (clips) — sets bone quaternions via AnimationMixer
  clipSeq.evaluateAtFrame(frame);
  // 2. Quaternion-space keyframe overrides — works WITH the mixer,
  //    no need to stop clips. Directly sets bone.quaternion via slerp.
  applyQuatOverrides(frame);
  // 3. Camera
  camAnim.evaluateAtFrame(frame);
  // 4. Render
  forceRender();
};

// ── Weapons ────────────────────────────────────────────────
window.attachWeapons = () => weapons.attachWeapons();
window.detachWeapons = () => weapons.detachWeapons();
window.setSwordOffset = (x, y, z, rx, ry, rz) => weapons.setSwordOffset(x, y, z, rx, ry, rz);
window.setShieldOffset = (x, y, z, rx, ry, rz) => weapons.setShieldOffset(x, y, z, rx, ry, rz);
window.setSwordVisible = (v) => weapons.setSwordVisible(v);
window.setShieldVisible = (v) => weapons.setShieldVisible(v);

// ── Camera ─────────────────────────────────────────────────
window.setCameraPosition = (x, y, z) => camAnim.setCameraPosition(x, y, z);
window.setCameraTarget = (tx, ty, tz) => camAnim.setCameraTarget(tx, ty, tz);
window.setCameraFOV = (fov) => camAnim.setCameraFOV(fov);
window.setCameraKeyframe = (frame) => {
  if (frame !== undefined) {
    // Save at specific frame without moving
    const state = camAnim.getCameraState();
    // Temporarily move to frame, set keyframe, move back
  }
  camAnim.setCameraKeyframe(frame || kfe.getCurrentFrame());
};
window.getCameraState = () => camAnim.getCameraState();
window.getCameraKeyframes = () => camAnim.exportJSON();
window.importCameraKeyframes = (json) => camAnim.importJSON(json);

// ── Background ─────────────────────────────────────────────
window.setBackground = (preset) => bg.setBackground(preset);
window.setParticleIntensity = (v) => bg.setParticleIntensity(v);

// ── Pose Library ───────────────────────────────────────────
window.savePose = (name) => poseLib.savePose(name);
window.loadPose = (name) => poseLib.loadPose(name);
window.listPoses = () => poseLib.listPoses();
window.deletePose = (name) => poseLib.deletePose(name);
window.exportPoseLibrary = () => poseLib.exportAll();
window.importPoseLibrary = (json) => poseLib.importAll(json);

// ── Undo/Redo ──────────────────────────────────────────────
window.undo = () => undoMgr.undo((action, dir) => {
  if (action.type === 'bone_rotation') {
    const rot = dir === 'undo' ? action.oldRot : action.newRot;
    rig.setBoneRotation(action.boneName, rot.x, rot.y, rot.z);
  }
});
window.redo = () => undoMgr.redo((action, dir) => {
  if (action.type === 'bone_rotation') {
    const rot = dir === 'redo' ? action.newRot : action.oldRot;
    rig.setBoneRotation(action.boneName, rot.x, rot.y, rot.z);
  }
});

// ── Onion Skin ─────────────────────────────────────────────
window.toggleOnionSkin = (v) => onionSkin.toggle(v);
window.setOnionSkinRange = (before, after) => onionSkin.setRange(before, after);

// ── Render Pipeline ────────────────────────────────────────
window.hideUI = () => renderPipe.hideUI();
window.showUI = () => renderPipe.showUI();
window.captureFrame = (w, h) => renderPipe.captureFrame(w, h);
window.setRenderSize = (w, h) => setRenderSize(w, h);
window.forceRender = () => forceRender();
