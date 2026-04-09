/**
 * undo-manager.js — Undo/redo stack for bone operations
 */

const MAX_STACK = 100;
const undoStack = [];
const redoStack = [];

function push(action) {
  // action: { type: 'bone_rotation', boneName, frame, oldRot: {rx,ry,rz}, newRot: {rx,ry,rz} }
  undoStack.push(action);
  if (undoStack.length > MAX_STACK) undoStack.shift();
  redoStack.length = 0; // clear redo on new action
}

function undo(applyFn) {
  if (undoStack.length === 0) return null;
  const action = undoStack.pop();
  redoStack.push(action);
  if (applyFn) applyFn(action, 'undo');
  return action;
}

function redo(applyFn) {
  if (redoStack.length === 0) return null;
  const action = redoStack.pop();
  undoStack.push(action);
  if (applyFn) applyFn(action, 'redo');
  return action;
}

function canUndo() { return undoStack.length > 0; }
function canRedo() { return redoStack.length > 0; }
function clear() { undoStack.length = 0; redoStack.length = 0; }

export { push, undo, redo, canUndo, canRedo, clear };
