/**
 * render-pipeline.js — Frame capture and batch export coordination
 */
import { renderer, scene, camera, boneHelperGroup, forceRender, setRenderSize } from './core.js';

/**
 * Hide all UI overlays for clean render.
 */
function hideUI() {
  boneHelperGroup.visible = false;
  ['bone-list', 'rotation-controls', 'toolbar', 'timeline'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.style.display = 'none';
  });
}

/**
 * Show UI overlays.
 */
function showUI() {
  boneHelperGroup.visible = true;
  ['bone-list', 'rotation-controls'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.style.display = '';
  });
  ['toolbar', 'timeline'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.style.display = 'flex';
  });
}

/**
 * Capture the current frame as a base64-encoded PNG.
 */
function captureFrame(width, height) {
  const prevW = renderer.domElement.width;
  const prevH = renderer.domElement.height;
  const prevAspect = camera.aspect;

  if (width && height) {
    setRenderSize(width, height);
  }

  forceRender();
  const dataUrl = renderer.domElement.toDataURL('image/png');

  // Restore
  if (width && height) {
    setRenderSize(prevW, prevH);
  }

  return dataUrl;
}

/**
 * Signal that the pipeline is ready for the Python side to screenshot.
 * The Python Playwright script will call this per-frame during batch render.
 */
let _currentRenderFrame = 0;
function setCurrentRenderFrame(f) { _currentRenderFrame = f; }
function getCurrentRenderFrame() { return _currentRenderFrame; }

export {
  hideUI, showUI,
  captureFrame,
  setCurrentRenderFrame, getCurrentRenderFrame,
};
