/**
 * onion-skin.js — Ghost previous/next frame poses for motion reference
 *
 * Lightweight: stores captured poses and renders as wireframe overlays.
 * Default off — toggle-able via API.
 */
import { THREE, scene } from './core.js';
import { getModel } from './rig.js';

let enabled = false;
let beforeCount = 2;
let afterCount = 1;
const ghostGroup = new THREE.Group();
ghostGroup.visible = false;
scene.add(ghostGroup);

// Onion skin materials
const beforeMat = new THREE.MeshBasicMaterial({
  color: 0xff4444, wireframe: true, transparent: true, opacity: 0.15,
});
const afterMat = new THREE.MeshBasicMaterial({
  color: 0x4444ff, wireframe: true, transparent: true, opacity: 0.15,
});

function toggle(v) {
  enabled = (v !== undefined) ? v : !enabled;
  ghostGroup.visible = enabled;
}

function setRange(before, after) {
  beforeCount = before;
  afterCount = after;
}

function isEnabled() { return enabled; }

/**
 * Update ghosts. This is called externally when the frame changes.
 * For now, this is a stub — full implementation requires cloning
 * skinned meshes which is expensive. A simpler approach is to
 * draw wireframe bounding shapes at cached bone positions.
 */
function update(currentFrame, goToFrameFn) {
  // Clear existing ghosts
  ghostGroup.clear();

  if (!enabled) return;

  // Stub: in a full implementation, this would:
  // 1. Save current pose
  // 2. Go to previous frames, capture bone world positions
  // 3. Draw simple line skeletons at those positions
  // 4. Restore current pose
  //
  // For now, onion skin is a toggle-able placeholder.
}

export { toggle, setRange, isEnabled, update };
