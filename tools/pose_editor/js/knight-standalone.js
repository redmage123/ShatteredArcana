/**
 * knight-standalone.js — Self-contained SDF knight with custom skeleton
 *
 * No Mixamo dependency. Defines its own skeleton, generates SDF geometry,
 * skins it, and provides an animation interface. Everything in one
 * coordinate system that we control.
 *
 * Coordinate system: Y-up, meters. Character ~2m tall, centered at origin.
 */
import { THREE, scene, envMap, camera, controls, forceRender, registerUpdatable } from './core.js';

// ============================================================
// SKELETON DEFINITION
// ============================================================

const BONE_DEFS = [
  // [name, parentIndex, restPosition [x,y,z] relative to parent]
  ['Hips',           -1, [0, 1.0, 0]],        // 0 - root
  ['Spine',           0, [0, 0.13, 0]],        // 1
  ['Chest',           1, [0, 0.13, 0]],        // 2
  ['UpperChest',      2, [0, 0.13, 0]],        // 3
  ['Neck',            3, [0, 0.10, 0]],        // 4
  ['Head',            4, [0, 0.10, 0]],        // 5
  ['HeadTop',         5, [0, 0.15, 0]],        // 6

  ['LeftShoulder',    3, [0.08, 0.08, 0]],     // 7
  ['LeftUpperArm',    7, [0.12, 0, 0]],        // 8
  ['LeftLowerArm',    8, [0.28, 0, 0]],        // 9
  ['LeftHand',        9, [0.25, 0, 0]],        // 10

  ['RightShoulder',   3, [-0.08, 0.08, 0]],    // 11
  ['RightUpperArm',  11, [-0.12, 0, 0]],       // 12
  ['RightLowerArm',  12, [-0.28, 0, 0]],       // 13
  ['RightHand',      13, [-0.25, 0, 0]],       // 14

  ['LeftUpperLeg',    0, [0.09, -0.05, 0]],    // 15
  ['LeftLowerLeg',   15, [0, -0.45, 0]],       // 16
  ['LeftFoot',       16, [0, -0.45, 0]],       // 17
  ['LeftToe',        17, [0, -0.05, 0.12]],    // 18

  ['RightUpperLeg',   0, [-0.09, -0.05, 0]],   // 19
  ['RightLowerLeg',  19, [0, -0.45, 0]],       // 20
  ['RightFoot',      20, [0, -0.45, 0]],       // 21
  ['RightToe',       21, [0, -0.05, 0.12]],    // 22
];

// Body segment radii for SDF capsules (in meters)
const SEGMENT_RADII = {
  'Hips→Spine': 0.11,
  'Spine→Chest': 0.10,
  'Chest→UpperChest': 0.10,
  'UpperChest→Neck': 0.08,
  'Neck→Head': 0.06,
  'Head→HeadTop': 0.10,
  'LeftShoulder→LeftUpperArm': 0.055,
  'LeftUpperArm→LeftLowerArm': 0.045,
  'LeftLowerArm→LeftHand': 0.038,
  'RightShoulder→RightUpperArm': 0.055,
  'RightUpperArm→RightLowerArm': 0.045,
  'RightLowerArm→RightHand': 0.038,
  'LeftUpperLeg→LeftLowerLeg': 0.065,
  'LeftLowerLeg→LeftFoot': 0.052,
  'LeftFoot→LeftToe': 0.040,
  'RightUpperLeg→RightLowerLeg': 0.065,
  'RightLowerLeg→RightFoot': 0.052,
  'RightFoot→RightToe': 0.040,
};

// ============================================================
// SDF PRIMITIVES
// ============================================================

function sdCapsule(p, a, b, r) {
  const pa0 = p[0]-a[0], pa1 = p[1]-a[1], pa2 = p[2]-a[2];
  const ba0 = b[0]-a[0], ba1 = b[1]-a[1], ba2 = b[2]-a[2];
  const baDot = ba0*ba0 + ba1*ba1 + ba2*ba2;
  let h = (pa0*ba0 + pa1*ba1 + pa2*ba2) / (baDot || 1e-8);
  h = Math.max(0, Math.min(1, h));
  const d0 = pa0-ba0*h, d1 = pa1-ba1*h, d2 = pa2-ba2*h;
  return Math.sqrt(d0*d0 + d1*d1 + d2*d2) - r;
}

function sdSphere(p, c, r) {
  const d0 = p[0]-c[0], d1 = p[1]-c[1], d2 = p[2]-c[2];
  return Math.sqrt(d0*d0 + d1*d1 + d2*d2) - r;
}

function sdBox(p, c, h, bevel) {
  const d0 = Math.abs(p[0]-c[0])-h[0], d1 = Math.abs(p[1]-c[1])-h[1], d2 = Math.abs(p[2]-c[2])-h[2];
  const m0 = Math.max(d0,0), m1 = Math.max(d1,0), m2 = Math.max(d2,0);
  return Math.sqrt(m0*m0+m1*m1+m2*m2) + Math.min(Math.max(d0,d1,d2), 0) - (bevel||0);
}

// Polynomial smooth min (Inigo Quilez)
function smin(a, b, k) {
  const h = Math.max(k - Math.abs(a-b), 0) / k;
  return Math.min(a,b) - h*h*h*k/6;
}

// ============================================================
// BUILD SKELETON
// ============================================================

function buildSkeleton() {
  const bones = [];
  const worldPositions = []; // absolute world positions for each bone

  for (let i = 0; i < BONE_DEFS.length; i++) {
    const [name, parentIdx, relPos] = BONE_DEFS[i];
    const bone = new THREE.Bone();
    bone.name = name;
    bone.position.set(relPos[0], relPos[1], relPos[2]);
    bones.push(bone);

    if (parentIdx >= 0) {
      bones[parentIdx].add(bone);
      // Compute absolute position
      const pp = worldPositions[parentIdx];
      worldPositions.push([pp[0]+relPos[0], pp[1]+relPos[1], pp[2]+relPos[2]]);
    } else {
      worldPositions.push([relPos[0], relPos[1], relPos[2]]);
    }
  }

  const skeleton = new THREE.Skeleton(bones);
  return { skeleton, bones, worldPositions };
}

// ============================================================
// SDF → MARCHING CUBES
// ============================================================

function buildKnightSDF(worldPositions) {
  const wp = worldPositions;
  const K = 0.06; // smooth blend radius (meters)

  // Build segments from parent→child pairs
  const bodySegments = [];
  for (let i = 1; i < BONE_DEFS.length; i++) {
    const [name, parentIdx] = BONE_DEFS[i];
    if (parentIdx < 0) continue;
    const parentName = BONE_DEFS[parentIdx][0];
    const key = `${parentName}→${name}`;
    const r = SEGMENT_RADII[key] || 0.04;
    bodySegments.push({ a: wp[parentIdx], b: wp[i], r, boneIdx: i });
  }

  return function sdf(p) {
    let d = 1e6;
    for (const seg of bodySegments) {
      d = smin(d, sdCapsule(p, seg.a, seg.b, seg.r), K);
    }

    // Armor: breastplate
    const chest = [(wp[2][0]+wp[3][0])/2, (wp[2][1]+wp[3][1])/2, (wp[2][2]+wp[3][2])/2+0.04];
    d = smin(d, sdBox(p, chest, [0.13, 0.12, 0.04], 0.01), 0.03);

    // Pauldrons
    d = smin(d, sdSphere(p, [wp[8][0]+0.02, wp[8][1]+0.04, wp[8][2]], 0.07), 0.03);
    d = smin(d, sdSphere(p, [wp[12][0]-0.02, wp[12][1]+0.04, wp[12][2]], 0.07), 0.03);

    // Helmet
    const head = [(wp[5][0]+wp[6][0])/2, (wp[5][1]+wp[6][1])/2, (wp[5][2]+wp[6][2])/2];
    d = smin(d, sdSphere(p, head, 0.125), 0.03);

    // Knee guards
    d = smin(d, sdSphere(p, [wp[16][0], wp[16][1], wp[16][2]+0.02], 0.05), 0.02);
    d = smin(d, sdSphere(p, [wp[20][0], wp[20][1], wp[20][2]+0.02], 0.05), 0.02);

    // Boots
    d = smin(d, sdBox(p, [wp[17][0], wp[17][1]-0.01, wp[17][2]+0.04], [0.05, 0.04, 0.08], 0.01), 0.02);
    d = smin(d, sdBox(p, [wp[21][0], wp[21][1]-0.01, wp[21][2]+0.04], [0.05, 0.04, 0.08], 0.01), 0.02);

    return d;
  };
}

function buildGoldSDF(worldPositions) {
  const wp = worldPositions;
  const head = [(wp[5][0]+wp[6][0])/2, (wp[5][1]+wp[6][1])/2, (wp[5][2]+wp[6][2])/2];
  const chest = [(wp[2][0]+wp[3][0])/2, (wp[2][1]+wp[3][1])/2, (wp[2][2]+wp[3][2])/2];

  return function(p) {
    let d = 1e6;
    // Helmet crest — raised ridge on top (thicker to resolve at grid)
    d = Math.min(d, sdBox(p, [head[0], head[1]+0.11, head[2]-0.01], [0.015, 0.05, 0.07], 0.008));
    // Chest vertical gold band
    d = Math.min(d, sdBox(p, [chest[0], chest[1], chest[2]+0.085], [0.012, 0.10, 0.012], 0.005));
    // Chest horizontal gold band
    d = Math.min(d, sdBox(p, [chest[0], chest[1], chest[2]+0.085], [0.11, 0.012, 0.012], 0.005));
    // Waist belt — thick ring
    d = Math.min(d, sdCapsule(p, [wp[0][0]-0.13, wp[0][1]+0.04, wp[0][2]+0.02],
                                  [wp[0][0]+0.13, wp[0][1]+0.04, wp[0][2]+0.02], 0.018));
    // Pauldron rims — thick bands around shoulder tops
    const lArm = wp[8], rArm = wp[12];
    d = Math.min(d, sdCapsule(p, [lArm[0]+0.03, lArm[1]+0.06, lArm[2]-0.05],
                                  [lArm[0]+0.03, lArm[1]+0.06, lArm[2]+0.05], 0.020));
    d = Math.min(d, sdCapsule(p, [rArm[0]-0.03, rArm[1]+0.06, rArm[2]-0.05],
                                  [rArm[0]-0.03, rArm[1]+0.06, rArm[2]+0.05], 0.020));
    // Knee cap accents — golden knee pads
    d = Math.min(d, sdSphere(p, [wp[16][0], wp[16][1], wp[16][2]+0.04], 0.035));
    d = Math.min(d, sdSphere(p, [wp[20][0], wp[20][1], wp[20][2]+0.04], 0.035));
    // Boot rims
    d = Math.min(d, sdCapsule(p, [wp[17][0]-0.055, wp[17][1]+0.03, wp[17][2]],
                                  [wp[17][0]+0.055, wp[17][1]+0.03, wp[17][2]], 0.015));
    d = Math.min(d, sdCapsule(p, [wp[21][0]-0.055, wp[21][1]+0.03, wp[21][2]],
                                  [wp[21][0]+0.055, wp[21][1]+0.03, wp[21][2]], 0.015));
    // Gauntlet cuffs
    d = Math.min(d, sdCapsule(p, [wp[9][0]-0.01, wp[9][1], wp[9][2]-0.04],
                                  [wp[9][0]-0.01, wp[9][1], wp[9][2]+0.04], 0.014));
    d = Math.min(d, sdCapsule(p, [wp[13][0]+0.01, wp[13][1], wp[13][2]-0.04],
                                  [wp[13][0]+0.01, wp[13][1], wp[13][2]+0.04], 0.014));
    // Sword crossguard (gold)
    const rHand = wp[14];
    d = Math.min(d, sdBox(p, [rHand[0]-0.02, rHand[1]+0.05, rHand[2]], [0.006, 0.006, 0.065], 0.005));
    // Sword pommel
    d = Math.min(d, sdSphere(p, [rHand[0]-0.02, rHand[1]-0.06, rHand[2]], 0.018));
    return d;
  };
}

/**
 * Build a visor SDF (subtracted from helmet to create the slit).
 */
function buildVisorSDF(worldPositions) {
  const wp = worldPositions;
  const head = [(wp[5][0]+wp[6][0])/2, (wp[5][1]+wp[6][1])/2, (wp[5][2]+wp[6][2])/2];
  return function(p) {
    return sdBox(p, [head[0], head[1]-0.015, head[2]+0.13], [0.065, 0.010, 0.03], 0);
  };
}

/**
 * Build weapon SDFs — sword (right hand) and shield (left hand).
 * These are part of the SDF field, not separate objects.
 */
function buildWeaponSDF(worldPositions) {
  const wp = worldPositions;
  const rHand = wp[14]; // RightHand
  const lHand = wp[10]; // LeftHand

  return function(p) {
    let d = 1e6;
    // Sword: long thin box extending from right hand
    const swordBase = [rHand[0]-0.02, rHand[1], rHand[2]];
    const swordTip = [rHand[0]-0.02, rHand[1]+0.75, rHand[2]];
    d = Math.min(d, sdCapsule(p, swordBase, swordTip, 0.012)); // blade
    d = Math.min(d, sdCapsule(p, [rHand[0]-0.02, rHand[1]-0.05, rHand[2]],
                                  [rHand[0]-0.02, rHand[1]+0.05, rHand[2]], 0.02)); // grip
    // Crossguard
    d = Math.min(d, sdBox(p, [rHand[0]-0.02, rHand[1]+0.05, rHand[2]], [0.002, 0.002, 0.06], 0.003));

    // Shield: flat box on left forearm
    const shieldCenter = [(lHand[0]+wp[9][0])/2 + 0.02, (lHand[1]+wp[9][1])/2, (lHand[2]+wp[9][2])/2 + 0.06];
    d = Math.min(d, sdBox(p, shieldCenter, [0.005, 0.18, 0.14], 0.015)); // shield body

    return d;
  };
}

// Standard marching cubes edge and tri tables (compressed)
const ET=[0x0,0x109,0x203,0x30a,0x406,0x50f,0x605,0x70c,0x80c,0x905,0xa0f,0xb06,0xc0a,0xd03,0xe09,0xf00,0x190,0x99,0x393,0x29a,0x596,0x49f,0x795,0x69c,0x99c,0x895,0xb9f,0xa96,0xd9a,0xc93,0xf99,0xe90,0x230,0x339,0x33,0x13a,0x636,0x73f,0x435,0x53c,0xa3c,0xb35,0x83f,0x936,0xe3a,0xf33,0xc39,0xd30,0x3a0,0x2a9,0x1a3,0xaa,0x7a6,0x6af,0x5a5,0x4ac,0xbac,0xaa5,0x9af,0x8a6,0xfaa,0xea3,0xda9,0xca0,0x460,0x569,0x663,0x76a,0x66,0x16f,0x265,0x36c,0xc6c,0xd65,0xe6f,0xf66,0x86a,0x963,0xa69,0xb60,0x5f0,0x4f9,0x7f3,0x6fa,0x1f6,0xff,0x3f5,0x2fc,0xdfc,0xcf5,0xfff,0xef6,0x9fa,0x8f3,0xbf9,0xaf0,0x650,0x759,0x453,0x55a,0x256,0x35f,0x55,0x15c,0xe5c,0xf55,0xc5f,0xd56,0xa5a,0xb53,0x859,0x950,0x7c0,0x6c9,0x5c3,0x4ca,0x3c6,0x2cf,0x1c5,0xcc,0xfcc,0xec5,0xdcf,0xcc6,0xbca,0xac3,0x9c9,0x8c0,0x8c0,0x9c9,0xac3,0xbca,0xcc6,0xdcf,0xec5,0xfcc,0xcc,0x1c5,0x2cf,0x3c6,0x4ca,0x5c3,0x6c9,0x7c0,0x950,0x859,0xb53,0xa5a,0xd56,0xc5f,0xf55,0xe5c,0x15c,0x55,0x35f,0x256,0x55a,0x453,0x759,0x650,0xaf0,0xbf9,0x8f3,0x9fa,0xef6,0xfff,0xcf5,0xdfc,0x2fc,0x3f5,0xff,0x1f6,0x6fa,0x7f3,0x4f9,0x5f0,0xb60,0xa69,0x963,0x86a,0xf66,0xe6f,0xd65,0xc6c,0x36c,0x265,0x16f,0x66,0x76a,0x663,0x569,0x460,0xca0,0xda9,0xea3,0xfaa,0x8a6,0x9af,0xaa5,0xbac,0x4ac,0x5a5,0x6af,0x7a6,0xaa,0x1a3,0x2a9,0x3a0,0xd30,0xc39,0xf33,0xe3a,0x936,0x83f,0xb35,0xa3c,0x53c,0x435,0x73f,0x636,0x13a,0x33,0x339,0x230,0xe90,0xf99,0xc93,0xd9a,0xa96,0xb9f,0x895,0x99c,0x69c,0x795,0x49f,0x596,0x29a,0x393,0x99,0x190,0xf00,0xe09,0xd03,0xc0a,0xb06,0xa0f,0x905,0x80c,0x70c,0x605,0x50f,0x406,0x30a,0x203,0x109,0x0];
// Using the same TRI_TABLE from sdf-knight.js (imported at module level would be circular)
// For standalone, we embed it. Due to size, using the same reference.
import { marchingCubes as mc } from './sdf-knight.js';

function marchCubes(sdf, bbMin, bbMax, res) {
  return mc(sdf, bbMin, bbMax, res);
}

// ============================================================
// SKINNING WEIGHTS
// ============================================================

function computeWeights(positions, worldPositions, sigma) {
  const n = positions.length / 3;
  const nBones = BONE_DEFS.length;
  const indices = new Float32Array(n * 4);
  const weights = new Float32Array(n * 4);

  // Build segments for distance computation
  const segments = [];
  for (let i = 1; i < BONE_DEFS.length; i++) {
    const parentIdx = BONE_DEFS[i][1];
    if (parentIdx < 0) continue;
    segments.push({ a: worldPositions[parentIdx], b: worldPositions[i], idx: i });
  }
  // Also add root
  segments.push({ a: worldPositions[0], b: worldPositions[0], idx: 0 });

  for (let v = 0; v < n; v++) {
    const p = [positions[v*3], positions[v*3+1], positions[v*3+2]];

    // Distance to each segment
    const dists = segments.map(s => {
      const pa = [p[0]-s.a[0], p[1]-s.a[1], p[2]-s.a[2]];
      const ba = [s.b[0]-s.a[0], s.b[1]-s.a[1], s.b[2]-s.a[2]];
      const dot = ba[0]*ba[0]+ba[1]*ba[1]+ba[2]*ba[2];
      let h = (pa[0]*ba[0]+pa[1]*ba[1]+pa[2]*ba[2])/(dot||1e-8);
      h = Math.max(0, Math.min(1, h));
      const dx = pa[0]-ba[0]*h, dy = pa[1]-ba[1]*h, dz = pa[2]-ba[2]*h;
      return { d: Math.sqrt(dx*dx+dy*dy+dz*dz), i: s.idx };
    });

    dists.sort((a,b) => a.d - b.d);
    const top = dists.slice(0, 4);
    const exps = top.map(t => Math.exp(-t.d / sigma));
    const sum = exps.reduce((a,b) => a+b, 0) || 1;

    for (let j = 0; j < 4; j++) {
      indices[v*4+j] = top[j] ? top[j].i : 0;
      weights[v*4+j] = top[j] ? exps[j] / sum : 0;
    }
  }

  return { indices, weights };
}

// ============================================================
// MAIN BUILD FUNCTION
// ============================================================

let _knight = null;

function buildKnight(opts = {}) {
  const resolution = opts.resolution || 64;
  console.time('Standalone Knight');

  // 1. Build skeleton
  const { skeleton, bones, worldPositions } = buildSkeleton();
  console.log(`  Skeleton: ${bones.length} bones`);

  // 2. Generate SDFs
  const bodySDF = buildKnightSDF(worldPositions);
  const goldSDF = buildGoldSDF(worldPositions);
  const visorSDF = buildVisorSDF(worldPositions);
  const weaponSDF = buildWeaponSDF(worldPositions);

  // Combined body SDF: body + weapons, minus visor slit
  const fullSDF = (p) => {
    let d = bodySDF(p);
    d = smin(d, weaponSDF(p), 0.02); // sharp join for weapons
    // Subtract visor slit
    const v = visorSDF(p);
    d = Math.max(d, -v); // CSG subtraction
    return d;
  };

  // 3. Bounding box — expanded for weapons
  let bbMin = [Infinity, Infinity, Infinity];
  let bbMax = [-Infinity, -Infinity, -Infinity];
  for (const wp of worldPositions) {
    for (let i = 0; i < 3; i++) {
      bbMin[i] = Math.min(bbMin[i], wp[i]);
      bbMax[i] = Math.max(bbMax[i], wp[i]);
    }
  }
  // Expand for sword reach and shield
  bbMin = [bbMin[0] - 0.25, bbMin[1] - 0.2, bbMin[2] - 0.25];
  bbMax = [bbMax[0] + 0.25, bbMax[1] + 0.9, bbMax[2] + 0.25]; // +0.9 for sword height

  // 4. Marching cubes — body + weapons
  console.log(`  Marching cubes res=${resolution}...`);
  const bodyMesh = marchCubes(fullSDF, bbMin, bbMax, resolution);
  console.log(`  Body+weapons: ${bodyMesh.vertexCount} vertices`);

  // Gold trim — same bounds, same resolution so thick features resolve
  const goldMesh = marchCubes(goldSDF, bbMin, bbMax, resolution);
  console.log(`  Gold trim: ${goldMesh.vertexCount} vertices`);

  if (bodyMesh.vertexCount === 0) {
    console.error('  ERROR: No body vertices!');
    console.timeEnd('Standalone Knight');
    return null;
  }

  // 5. Skinning weights
  console.log('  Computing skinning weights...');
  const { indices: skinIdx, weights: skinWt } = computeWeights(
    bodyMesh.positions, worldPositions, opts.sigma || 0.08
  );

  // 6. Build body geometry
  const bodyGeo = new THREE.BufferGeometry();
  bodyGeo.setAttribute('position', new THREE.BufferAttribute(bodyMesh.positions, 3));
  bodyGeo.setAttribute('normal', new THREE.BufferAttribute(bodyMesh.normals, 3));
  bodyGeo.setAttribute('skinIndex', new THREE.BufferAttribute(new Uint16Array(skinIdx), 4));
  bodyGeo.setAttribute('skinWeight', new THREE.BufferAttribute(skinWt, 4));

  // 7. Materials — Aurelith palette (bright polished steel + warm gold)
  const steelMat = new THREE.MeshStandardMaterial({
    color: opts.steelColor || 0xc8d0dc,
    metalness: 0.88,
    roughness: 0.18,
    envMap,
    envMapIntensity: 3.0,
  });

  const goldMat = new THREE.MeshStandardMaterial({
    color: opts.goldColor || 0xf0c040,
    metalness: 0.95,
    roughness: 0.08,
    emissive: 0x553300,
    emissiveIntensity: 0.4,
    envMap,
    envMapIntensity: 3.0,
  });

  // 8. SkinnedMesh — body
  const bodySkinnedMesh = new THREE.SkinnedMesh(bodyGeo, steelMat);
  bodySkinnedMesh.name = 'StandaloneKnight_Body';
  bodySkinnedMesh.add(bones[0]);
  bodySkinnedMesh.bind(skeleton);
  bodySkinnedMesh.castShadow = true;
  bodySkinnedMesh.frustumCulled = false;
  scene.add(bodySkinnedMesh);

  // Gold trim — separate skinned mesh
  if (goldMesh.vertexCount > 0) {
    const goldGeo = new THREE.BufferGeometry();
    goldGeo.setAttribute('position', new THREE.BufferAttribute(goldMesh.positions, 3));
    goldGeo.setAttribute('normal', new THREE.BufferAttribute(goldMesh.normals, 3));
    const goldSkin = computeWeights(goldMesh.positions, worldPositions, opts.sigma || 0.08);
    goldGeo.setAttribute('skinIndex', new THREE.BufferAttribute(new Uint16Array(goldSkin.indices), 4));
    goldGeo.setAttribute('skinWeight', new THREE.BufferAttribute(goldSkin.weights, 4));

    const goldSkinnedMesh = new THREE.SkinnedMesh(goldGeo, goldMat);
    goldSkinnedMesh.name = 'StandaloneKnight_Gold';
    goldSkinnedMesh.bind(skeleton);
    goldSkinnedMesh.castShadow = true;
    goldSkinnedMesh.frustumCulled = false;
    bodySkinnedMesh.add(goldSkinnedMesh);
    console.log(`  Gold mesh: ${goldMesh.vertexCount} verts`);
  }

  console.timeEnd('Standalone Knight');
  const totalVerts = bodyMesh.vertexCount + goldMesh.vertexCount;
  console.log(`  Knight ready: ${totalVerts} verts, ${bones.length} bones`);

  _knight = { mesh: bodySkinnedMesh, skeleton, bones, worldPositions };
  return _knight;
}

// ============================================================
// ANIMATION INTERFACE
// ============================================================

/**
 * Set a bone rotation by name. Euler XYZ in degrees.
 */
function setBoneRotation(name, rx, ry, rz) {
  if (!_knight) return false;
  const bone = _knight.bones.find(b => b.name === name);
  if (!bone) return false;
  bone.rotation.set(
    THREE.MathUtils.degToRad(rx),
    THREE.MathUtils.degToRad(ry),
    THREE.MathUtils.degToRad(rz),
    bone.rotation.order
  );
  return true;
}

function getBoneRotation(name) {
  if (!_knight) return null;
  const bone = _knight.bones.find(b => b.name === name);
  if (!bone) return null;
  return {
    x: THREE.MathUtils.radToDeg(bone.rotation.x),
    y: THREE.MathUtils.radToDeg(bone.rotation.y),
    z: THREE.MathUtils.radToDeg(bone.rotation.z),
  };
}

function getBoneNames() {
  return BONE_DEFS.map(d => d[0]);
}

/**
 * Apply a full pose: { boneName: [rx, ry, rz], ... }
 */
function applyPose(pose) {
  for (const [name, rot] of Object.entries(pose)) {
    setBoneRotation(name, rot[0], rot[1], rot[2]);
  }
  forceRender();
}

/**
 * Reset all bones to rest pose.
 */
function resetPose() {
  if (!_knight) return;
  for (const bone of _knight.bones) {
    bone.rotation.set(0, 0, 0);
  }
  forceRender();
}

// ============================================================
// WINDOW API
// ============================================================

window.buildStandaloneKnight = (opts) => {
  const k = buildKnight(opts);
  return k ? k.bones.length : 0;
};

window.knightSetBone = (name, rx, ry, rz) => setBoneRotation(name, rx, ry, rz);
window.knightGetBone = (name) => getBoneRotation(name);
window.knightApplyPose = (pose) => applyPose(pose);
window.knightResetPose = () => resetPose();
window.knightGetBoneNames = () => getBoneNames();

export { buildKnight, setBoneRotation, getBoneRotation, applyPose, resetPose, getBoneNames };
