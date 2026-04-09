/**
 * rig.js — FBX model loading, bone map, rest pose
 */
import { THREE, scene, boneHelperGroup } from './core.js';
import { FBXLoader } from 'three/addons/loaders/FBXLoader.js';
import { applyKnightTextures } from './materials.js';

let model = null;
const bones = [];
const boneMap = {};
const restPose = {};
const boneSpheres = {};
let mixer = null;
let selectedBone = null;

const loader = new FBXLoader();

function loadModel(url) {
  return new Promise((resolve, reject) => {
    loader.load(url,
      (fbx) => {
        model = fbx;
        model.scale.setScalar(0.01); // Mixamo FBX is in cm

        applyKnightTextures(model);
        scene.add(model);

        // Collect bones, build map, store rest pose
        // IMPORTANT: Only store the FIRST bone with each name — that's the
        // deform bone that the AnimationMixer and SkinnedMesh target.
        // The second duplicate is a helper/null that doesn't affect the mesh.
        model.traverse((child) => {
          if (child.isBone) {
            bones.push(child);
            if (!boneMap[child.name]) {
              boneMap[child.name] = child;
              restPose[child.name] = {
                rx: THREE.MathUtils.radToDeg(child.rotation.x),
                ry: THREE.MathUtils.radToDeg(child.rotation.y),
                rz: THREE.MathUtils.radToDeg(child.rotation.z),
              };
            }
          }
        });

        // Create animation mixer (used by clip system)
        mixer = new THREE.AnimationMixer(model);

        // Kill embedded animations
        if (fbx.animations) fbx.animations.length = 0;

        createBoneVisuals();
        resolve({ boneCount: bones.length });
      },
      undefined,
      (err) => reject(err)
    );
  });
}

// Bone sphere visuals for click-selection
function createBoneVisuals() {
  boneHelperGroup.clear();
  const geo = new THREE.SphereGeometry(0.025, 8, 8);
  bones.forEach(bone => {
    const mat = new THREE.MeshBasicMaterial({ color: 0x00ff88, transparent: true, opacity: 0.5 });
    const sphere = new THREE.Mesh(geo.clone(), mat);
    sphere.userData.boneName = bone.name;
    boneSpheres[bone.name] = sphere;
    boneHelperGroup.add(sphere);
  });
}

function updateBoneVisuals() {
  const wp = new THREE.Vector3();
  bones.forEach(bone => {
    const s = boneSpheres[bone.name];
    if (s) { bone.getWorldPosition(wp); s.position.copy(wp); }
  });
}

// Selection
function selectBone(name) {
  if (selectedBone && boneSpheres[selectedBone]) {
    boneSpheres[selectedBone].material.color.setHex(0x00ff88);
    boneSpheres[selectedBone].material.opacity = 0.5;
  }
  selectedBone = name;
  if (boneSpheres[name]) {
    boneSpheres[name].material.color.setHex(0xff4444);
    boneSpheres[name].material.opacity = 0.8;
  }
}

function getSelectedBone() { return selectedBone; }

// Rotation get/set
function setBoneRotation(name, rx, ry, rz) {
  const b = boneMap[name];
  if (!b) return false;
  // Use atomic .set() to avoid quaternion sync issues when mixer is active
  b.rotation.set(
    THREE.MathUtils.degToRad(rx),
    THREE.MathUtils.degToRad(ry),
    THREE.MathUtils.degToRad(rz),
    b.rotation.order
  );
  return true;
}

function getBoneRotation(name) {
  const b = boneMap[name];
  if (!b) return null;
  return {
    x: THREE.MathUtils.radToDeg(b.rotation.x),
    y: THREE.MathUtils.radToDeg(b.rotation.y),
    z: THREE.MathUtils.radToDeg(b.rotation.z),
  };
}

function getBoneNames() { return bones.map(b => b.name); }

function getBonesState() {
  const s = {};
  bones.forEach(b => {
    s[b.name] = {
      x: THREE.MathUtils.radToDeg(b.rotation.x),
      y: THREE.MathUtils.radToDeg(b.rotation.y),
      z: THREE.MathUtils.radToDeg(b.rotation.z),
    };
  });
  return s;
}

function resetBone(name) {
  const rest = restPose[name];
  if (!rest) return;
  setBoneRotation(name, rest.rx, rest.ry, rest.rz);
}

function resetAllBones() {
  for (const name of Object.keys(restPose)) resetBone(name);
}

function capturePose() {
  return getBonesState();
}

function applyPose(poseData) {
  for (const [name, rot] of Object.entries(poseData)) {
    setBoneRotation(name, rot.x || rot.rx || 0, rot.y || rot.ry || 0, rot.z || rot.rz || 0);
  }
}

function getBone(name) { return boneMap[name] || null; }
function getModel() { return model; }
function getMixer() { return mixer; }
function getBones() { return bones; }
function getRestPose() { return restPose; }

export {
  loadModel,
  updateBoneVisuals,
  selectBone,
  getSelectedBone,
  setBoneRotation,
  getBoneRotation,
  getBoneNames,
  getBonesState,
  resetBone,
  resetAllBones,
  capturePose,
  applyPose,
  getBone,
  getModel,
  getMixer,
  getBones,
  getRestPose,
  boneSpheres,
  boneMap,
};
