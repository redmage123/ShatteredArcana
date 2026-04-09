/**
 * clip-loader.js — Load Mixamo FBX animation clips and extract AnimationClip data
 */
import { THREE } from './core.js';
import { FBXLoader } from 'three/addons/loaders/FBXLoader.js';
import { getMixer, getBones, getBone } from './rig.js';

const loader = new FBXLoader();
const clips = new Map();   // name -> AnimationClip
const actions = new Map();  // name -> AnimationAction

/**
 * Load a Mixamo FBX animation file and extract its AnimationClip.
 * The clip is retargeted to match the X Bot bone naming if needed.
 */
function loadClip(url, name) {
  return new Promise((resolve, reject) => {
    loader.load(url,
      (fbx) => {
        if (!fbx.animations || fbx.animations.length === 0) {
          reject(new Error(`No animation found in ${url}`));
          return;
        }
        let clip = fbx.animations[0];
        clip.name = name;

        // Retarget: ensure track bone names match our rig
        clip = retargetClip(clip);

        // Strip root motion (hips position track) so character stays centered
        clip = stripRootMotion(clip);

        clips.set(name, clip);
        resolve({
          name,
          duration: clip.duration,
          trackCount: clip.tracks.length,
        });
      },
      undefined,
      (err) => reject(err)
    );
  });
}

/**
 * Retarget clip track names to match the X Bot rig.
 * Mixamo tracks use paths like "mixamorig:Hips.quaternion" or "mixamorig:Hips.position".
 * Some exports may have different prefixes. We normalize to match our bone names.
 */
function retargetClip(clip) {
  const ourBoneNames = new Set(getBones().map(b => b.name));
  const newTracks = [];

  for (const track of clip.tracks) {
    // Track name format: "BoneName.property" (e.g., "mixamorigHips.quaternion")
    const dotIdx = track.name.lastIndexOf('.');
    if (dotIdx === -1) continue;

    let boneName = track.name.substring(0, dotIdx);
    const property = track.name.substring(dotIdx); // ".quaternion", ".position", ".scale"

    // Try direct match first
    if (ourBoneNames.has(boneName)) {
      newTracks.push(track);
      continue;
    }

    // Try replacing colon separator: "mixamorig:Hips" -> "mixamorigHips"
    const noColon = boneName.replace(/:/g, '');
    if (ourBoneNames.has(noColon)) {
      const newTrack = track.clone();
      newTrack.name = noColon + property;
      newTracks.push(newTrack);
      continue;
    }

    // Try adding colon: "mixamorigHips" -> "mixamorig:Hips"
    const withColon = boneName.replace(/^(mixamorig)(.*)$/, '$1:$2');
    if (ourBoneNames.has(withColon)) {
      const newTrack = track.clone();
      newTrack.name = withColon + property;
      newTracks.push(newTrack);
      continue;
    }

    // Skip unmatched tracks (may be root motion, mesh transforms, etc.)
  }

  return new THREE.AnimationClip(clip.name, clip.duration, newTracks);
}

/**
 * Strip root motion — remove or flatten the hips position track
 * so the character stays centered in the scene.
 */
function stripRootMotion(clip) {
  const filteredTracks = clip.tracks.filter(track => {
    // Remove position tracks on the hips (root bone) — this is the root motion
    if (track.name.toLowerCase().includes('hips') && track.name.endsWith('.position')) {
      return false;
    }
    return true;
  });
  return new THREE.AnimationClip(clip.name, clip.duration, filteredTracks);
}

/**
 * Get a loaded clip by name.
 */
function getClip(name) { return clips.get(name) || null; }

/**
 * List all loaded clips.
 */
function listClips() {
  return Array.from(clips.entries()).map(([name, clip]) => ({
    name,
    duration: clip.duration,
    trackCount: clip.tracks.length,
  }));
}

// Per-clip action cache — survives stopAllClips so we don't recreate actions.
// The jitter bug was caused by mixer.stopAllAction() destroying the action,
// then clipAction() creating a new one with a fresh internal state each frame.
const _actionCache = new Map();  // clipName -> { action, needsPlay }

/**
 * Sample a clip at a specific time using AnimationMixer.
 *
 * Uses a persistent action cache to avoid recreating actions. The old code
 * called mixer.stopAllAction() + mixer.clipAction() + action.play() every
 * time the clip name changed, which caused frame-to-frame jitter because
 * Three.js AnimationAction has internal interpolation state that resets
 * on creation.
 */
function sampleClipAtTime(name, timeSeconds) {
  const clip = clips.get(name);
  const mixer = getMixer();
  if (!clip || !mixer) return false;

  // Get or create a persistent action for this clip
  let entry = _actionCache.get(name);
  if (!entry) {
    const action = mixer.clipAction(clip);
    action.setLoop(THREE.LoopOnce);
    action.clampWhenFinished = true;
    action.play();
    entry = { action };
    _actionCache.set(name, entry);
  }

  // Ensure this action is enabled and weighted (it may have been
  // de-weighted by stopAllClips or another clip taking over)
  const action = entry.action;
  if (!action.isRunning()) {
    action.play();
  }
  action.enabled = true;
  action.setEffectiveWeight(1.0);

  // Mute all OTHER actions so only this clip drives the rig
  for (const [otherName, otherEntry] of _actionCache) {
    if (otherName !== name) {
      otherEntry.action.setEffectiveWeight(0);
    }
  }

  // Seek to exact time — clamp to valid range.
  // For reversed playback, the sequencer may send negative times; we
  // handle that by reflecting: if t < 0, use abs(t) (plays the clip
  // backwards from t=0 which is the start pose — effectively a mirror).
  let t = timeSeconds;
  if (t < 0) t = Math.min(Math.abs(t), clip.duration);
  t = Math.max(0, Math.min(t, clip.duration));
  mixer.setTime(t);

  return true;
}

/**
 * Sample a clip at a specific frame number (using 24fps).
 */
function sampleClipAtFrame(name, frame) {
  return sampleClipAtTime(name, frame / 24);
}

/**
 * Play a clip using AnimationMixer (for preview).
 */
function playClip(name, opts = {}) {
  const clip = clips.get(name);
  const mixer = getMixer();
  if (!clip || !mixer) return false;

  stopAllClips();

  const action = mixer.clipAction(clip);
  action.setLoop(opts.loop ? THREE.LoopRepeat : THREE.LoopOnce);
  action.clampWhenFinished = true;
  action.timeScale = opts.speed || 1.0;
  action.play();
  actions.set(name, action);
  return true;
}

/**
 * Stop all clips — but do NOT destroy cached actions.
 * We only set their weight to 0 so they can be resumed without jitter.
 * The _actionCache keeps them alive.
 */
function stopAllClips() {
  for (const [, entry] of _actionCache) {
    entry.action.setEffectiveWeight(0);
    entry.action.paused = true;
  }
  actions.clear();
}

export {
  loadClip,
  getClip,
  listClips,
  sampleClipAtTime,
  sampleClipAtFrame,
  playClip,
  stopAllClips,
  retargetClip,
};
