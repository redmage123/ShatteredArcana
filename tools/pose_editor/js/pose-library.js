/**
 * pose-library.js — Named pose save/recall
 */
import * as rig from './rig.js';

const poses = new Map();

function savePose(name) {
  poses.set(name, rig.capturePose());
}

function savePoseSubset(name, boneNames) {
  const state = rig.getBonesState();
  const subset = {};
  boneNames.forEach(n => { if (state[n]) subset[n] = state[n]; });
  poses.set(name, subset);
}

function loadPose(name) {
  const pose = poses.get(name);
  if (!pose) return false;
  rig.applyPose(pose);
  return true;
}

function listPoses() { return Array.from(poses.keys()); }
function deletePose(name) { poses.delete(name); }
function getPose(name) { return poses.get(name) || null; }

function exportAll() {
  const obj = {};
  poses.forEach((v, k) => { obj[k] = v; });
  return JSON.stringify(obj);
}

function importAll(json) {
  const data = JSON.parse(json);
  for (const [k, v] of Object.entries(data)) {
    poses.set(k, v);
  }
}

export { savePose, savePoseSubset, loadPose, listPoses, deletePose, getPose, exportAll, importAll };
