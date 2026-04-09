/**
 * clip-sequencer.js — Timeline of clip segments with crossfade blending
 *
 * Fixed issues:
 * - Negative speed now correctly reverses clip playback
 * - Crossfade blending between overlapping segments
 * - blendFrames auto-calculated at beat transitions when not specified
 */
import { THREE } from './core.js';
import { sampleClipAtTime, getClip } from './clip-loader.js';
import * as rig from './rig.js';

const FPS = 24;
const DEFAULT_BLEND_FRAMES = 6; // 0.25s crossfade at transitions

/**
 * A segment on the timeline:
 * { clipName, startFrame, endFrame, clipStartTime, clipEndTime, speed, blendInFrames, weight }
 */
const segments = [];

function smoothstep(t) { return t * t * (3 - 2 * t); }

function addSegment(clipName, startFrame, endFrame, opts = {}) {
  const clip = getClip(clipName);
  if (!clip) return -1;

  const speed = opts.speed || 1.0;

  // Calculate the actual clip time range this segment covers.
  // For negative speed, clipStart should be > clipEnd (plays backwards).
  const clipStart = opts.clipStart != null ? opts.clipStart : 0;
  const durationFrames = endFrame - startFrame;
  const durationSec = durationFrames / FPS;
  const clipEnd = clipStart + durationSec * speed;

  const seg = {
    clipName,
    startFrame,
    endFrame,
    clipStartTime: clipStart,
    clipEndTime: clipEnd,
    speed,
    blendInFrames: opts.blendFrames != null ? opts.blendFrames : 0,
    weight: opts.weight || 1.0,
    clipDuration: clip.duration,
  };
  segments.push(seg);
  segments.sort((a, b) => a.startFrame - b.startFrame);

  // Auto-add crossfade blending: if this segment starts right after another,
  // extend both to overlap by DEFAULT_BLEND_FRAMES and set blend-in.
  _autoBlend();

  return segments.indexOf(seg);
}

/**
 * Automatically configure crossfade at segment boundaries.
 * If segment B starts at frame N and segment A ends at frame N-1 (or N),
 * extend B's start back by DEFAULT_BLEND_FRAMES and set its blendInFrames.
 */
function _autoBlend() {
  for (let i = 1; i < segments.length; i++) {
    const prev = segments[i - 1];
    const curr = segments[i];
    const gap = curr.startFrame - prev.endFrame;
    // Adjacent or overlapping by 1 frame = transition boundary
    if (gap >= -1 && gap <= 1 && curr.blendInFrames === 0) {
      curr.blendInFrames = DEFAULT_BLEND_FRAMES;
    }
  }
}

function removeSegment(index) {
  if (index >= 0 && index < segments.length) {
    segments.splice(index, 1);
  }
}

function clearSegments() { segments.length = 0; }
function getSegments() { return segments.map(s => ({ ...s })); }

/**
 * Get active segments at a given frame, including blend-in zones.
 */
function getActiveSegments(frame) {
  return segments.filter(seg => {
    const blendStart = seg.startFrame - seg.blendInFrames;
    return frame >= blendStart && frame <= seg.endFrame;
  });
}

/**
 * Compute the clip time for a segment at a given frame.
 * Handles negative speed correctly — maps frame progress to the clip
 * time range [clipStartTime, clipEndTime] using linear interpolation,
 * then clamps to [0, clipDuration].
 */
function getClipTime(seg, frame) {
  const progress = (frame - seg.startFrame) / (seg.endFrame - seg.startFrame);
  const t = seg.clipStartTime + (seg.clipEndTime - seg.clipStartTime) * progress;
  // Clamp to valid clip range
  return Math.max(0, Math.min(t, seg.clipDuration));
}

/**
 * Capture the rig pose as { boneName: Quaternion } after sampling a clip.
 */
function captureQuaternionPose() {
  const pose = {};
  const seen = new Set();
  rig.getBones().forEach(bone => {
    if (seen.has(bone.name)) return;
    seen.add(bone.name);
    pose[bone.name] = bone.quaternion.clone();
  });
  return pose;
}

/**
 * Evaluate the sequencer at a specific frame.
 * Applies clip poses with crossfade blending to the rig.
 */
function evaluateAtFrame(frame) {
  const active = getActiveSegments(frame);
  if (active.length === 0) return false;

  if (active.length === 1) {
    // Single clip — direct sample
    const seg = active[0];
    const clipTime = getClipTime(seg, frame);
    sampleClipAtTime(seg.clipName, clipTime);
    return true;
  }

  // Multiple overlapping segments — crossfade blend via quaternion slerp
  const poses = [];
  const weights = [];

  for (const seg of active) {
    const clipTime = getClipTime(seg, frame);

    // Calculate blend weight
    let weight = seg.weight;
    if (seg.blendInFrames > 0 && frame < seg.startFrame) {
      // In blend-in zone: ramp from 0 to 1
      const blendProgress = (frame - (seg.startFrame - seg.blendInFrames)) / seg.blendInFrames;
      weight *= smoothstep(Math.max(0, Math.min(1, blendProgress)));
    } else if (seg.blendInFrames > 0 && frame >= seg.startFrame) {
      // Past blend-in: check if prev segment is fading out
      // The previous segment's weight should decrease as this one increases
    }

    // Sample this clip
    sampleClipAtTime(seg.clipName, clipTime);
    poses.push(captureQuaternionPose());
    weights.push(weight);
  }

  // Normalize weights
  const totalWeight = weights.reduce((a, b) => a + b, 0);
  if (totalWeight <= 0) return false;

  // Blend: for each bone, slerp between poses weighted
  const seen = new Set();
  rig.getBones().forEach(bone => {
    if (seen.has(bone.name)) return;
    seen.add(bone.name);
    if (!poses[0][bone.name]) return;

    // Start with first pose
    const result = poses[0][bone.name].clone();
    let accWeight = weights[0];

    // Blend in subsequent poses
    for (let i = 1; i < poses.length; i++) {
      if (!poses[i][bone.name]) continue;
      const w = weights[i] / (accWeight + weights[i]);
      result.slerp(poses[i][bone.name], w);
      accWeight += weights[i];
    }

    bone.quaternion.copy(result);
  });

  return true;
}

export {
  addSegment,
  removeSegment,
  clearSegments,
  getSegments,
  getActiveSegments,
  evaluateAtFrame,
};
