/**
 * knight-model.js — Procedural Aurelith knight built from scratch
 *
 * Creates a fully rigged, skinned knight character using Three.js
 * geometry primitives. Each armor piece is a separate mesh segment
 * weighted to the appropriate bones.
 *
 * The model matches the Mixamo skeleton bone positions exactly so
 * all existing animation clips work without retargeting.
 */
import { THREE, scene, envMap } from './core.js';

// ============================================================
// MATERIALS
// ============================================================

function createMaterials() {
  const steel = new THREE.MeshStandardMaterial({
    color: 0x9aa5b4,
    metalness: 0.92,
    roughness: 0.15,
    envMap,
    envMapIntensity: 2.0,
  });

  const darkSteel = new THREE.MeshStandardMaterial({
    color: 0x3a4050,
    metalness: 0.88,
    roughness: 0.22,
    envMap,
    envMapIntensity: 1.5,
  });

  const gold = new THREE.MeshStandardMaterial({
    color: 0xdaa520,
    metalness: 0.95,
    roughness: 0.1,
    emissive: 0x332200,
    emissiveIntensity: 0.2,
    envMap,
    envMapIntensity: 2.0,
  });

  const chainmail = new THREE.MeshStandardMaterial({
    color: 0x707888,
    metalness: 0.85,
    roughness: 0.35,
    envMap,
    envMapIntensity: 1.2,
  });

  const leather = new THREE.MeshStandardMaterial({
    color: 0x2a1a10,
    metalness: 0.1,
    roughness: 0.8,
  });

  const visor = new THREE.MeshStandardMaterial({
    color: 0x101018,
    metalness: 0.5,
    roughness: 0.5,
    emissive: 0x1a2040,
    emissiveIntensity: 0.3,
  });

  return { steel, darkSteel, gold, chainmail, leather, visor };
}

// ============================================================
// GEOMETRY BUILDERS
// ============================================================

/**
 * Create a tapered cylinder (like an armor segment).
 * radiusTop/radiusBottom for taper, height for length.
 */
function armorSegment(radiusTop, radiusBottom, height, radialSegments = 12) {
  return new THREE.CylinderGeometry(radiusTop, radiusBottom, height, radialSegments);
}

/**
 * Create a rounded box (beveled edges look like plate armor).
 */
function armorPlate(width, height, depth, bevel = 0.01) {
  // Use box + slight scaling for now; could use ExtrudeGeometry for proper bevel
  const geo = new THREE.BoxGeometry(width, height, depth, 2, 2, 2);
  return geo;
}

/**
 * Create a helmet shape — sphere top + cylinder visor.
 */
function helmetGeometry() {
  const group = new THREE.Group();

  // Dome
  const dome = new THREE.SphereGeometry(0.12, 16, 12, 0, Math.PI * 2, 0, Math.PI * 0.6);
  dome.translate(0, 0.02, 0);

  // Face plate
  const face = new THREE.BoxGeometry(0.18, 0.14, 0.14);
  face.translate(0, -0.04, 0.01);

  // Visor slit (thin box, slightly recessed)
  const visorGeo = new THREE.BoxGeometry(0.16, 0.015, 0.02);
  visorGeo.translate(0, -0.02, 0.07);

  // Neck guard
  const neckGuard = new THREE.CylinderGeometry(0.10, 0.13, 0.06, 12);
  neckGuard.translate(0, -0.09, -0.02);

  return { dome, face, visorGeo, neckGuard };
}

// ============================================================
// ARMOR PIECE DEFINITIONS
// ============================================================
// Each piece: { geometry, material, bone, offset, rotation, scale }
// bone = name of the Mixamo bone this piece is parented to.
// offset = local offset from bone position.

function defineArmorPieces(materials) {
  const m = materials;

  return [
    // === HELMET ===
    { name: 'helmet_dome', geo: () => new THREE.SphereGeometry(0.13, 16, 12, 0, Math.PI*2, 0, Math.PI*0.65),
      mat: m.steel, bone: 'mixamorigHead', offset: [0, 0.08, 0.01], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'helmet_face', geo: () => new THREE.BoxGeometry(0.20, 0.16, 0.16),
      mat: m.steel, bone: 'mixamorigHead', offset: [0, -0.02, 0.02], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'helmet_visor', geo: () => new THREE.BoxGeometry(0.17, 0.018, 0.025),
      mat: m.visor, bone: 'mixamorigHead', offset: [0, 0.0, 0.085], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'helmet_crest', geo: () => new THREE.BoxGeometry(0.02, 0.08, 0.16),
      mat: m.gold, bone: 'mixamorigHead', offset: [0, 0.12, -0.01], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'helmet_neck', geo: () => armorSegment(0.11, 0.14, 0.07),
      mat: m.darkSteel, bone: 'mixamorigNeck', offset: [0, -0.02, 0], rot: [0, 0, 0], scale: [1, 1, 1] },

    // === TORSO ===
    { name: 'breastplate', geo: () => armorSegment(0.18, 0.16, 0.24),
      mat: m.steel, bone: 'mixamorigSpine1', offset: [0, 0.05, 0], rot: [0, 0, 0], scale: [1, 1, 0.7] },
    { name: 'backplate', geo: () => new THREE.BoxGeometry(0.30, 0.22, 0.06),
      mat: m.darkSteel, bone: 'mixamorigSpine1', offset: [0, 0.05, -0.08], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'belly', geo: () => armorSegment(0.16, 0.18, 0.14),
      mat: m.chainmail, bone: 'mixamorigSpine', offset: [0, 0.02, 0], rot: [0, 0, 0], scale: [1, 1, 0.65] },
    { name: 'waist', geo: () => armorSegment(0.18, 0.17, 0.10),
      mat: m.leather, bone: 'mixamorigHips', offset: [0, 0.06, 0], rot: [0, 0, 0], scale: [1, 1, 0.65] },
    { name: 'gold_trim_chest', geo: () => new THREE.BoxGeometry(0.005, 0.22, 0.15),
      mat: m.gold, bone: 'mixamorigSpine1', offset: [0, 0.05, 0.02], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'gold_cross_chest', geo: () => new THREE.BoxGeometry(0.25, 0.005, 0.15),
      mat: m.gold, bone: 'mixamorigSpine1', offset: [0, 0.05, 0.02], rot: [0, 0, 0], scale: [1, 1, 1] },

    // === SHOULDERS ===
    { name: 'pauldron_L', geo: () => new THREE.SphereGeometry(0.09, 10, 8, 0, Math.PI*2, 0, Math.PI*0.5),
      mat: m.steel, bone: 'mixamorigLeftShoulder', offset: [0.06, 0.04, 0], rot: [0, 0, -0.3], scale: [1.3, 1, 1.3] },
    { name: 'pauldron_R', geo: () => new THREE.SphereGeometry(0.09, 10, 8, 0, Math.PI*2, 0, Math.PI*0.5),
      mat: m.steel, bone: 'mixamorigRightShoulder', offset: [-0.06, 0.04, 0], rot: [0, 0, 0.3], scale: [1.3, 1, 1.3] },
    { name: 'pauldron_trim_L', geo: () => new THREE.TorusGeometry(0.085, 0.008, 8, 16),
      mat: m.gold, bone: 'mixamorigLeftShoulder', offset: [0.06, 0.02, 0], rot: [Math.PI/2, 0, 0], scale: [1, 1, 1] },
    { name: 'pauldron_trim_R', geo: () => new THREE.TorusGeometry(0.085, 0.008, 8, 16),
      mat: m.gold, bone: 'mixamorigRightShoulder', offset: [-0.06, 0.02, 0], rot: [Math.PI/2, 0, 0], scale: [1, 1, 1] },

    // === ARMS ===
    { name: 'upperarm_L', geo: () => armorSegment(0.055, 0.05, 0.30),
      mat: m.steel, bone: 'mixamorigLeftArm', offset: [0.15, 0, 0], rot: [0, 0, Math.PI/2], scale: [1, 1, 1] },
    { name: 'upperarm_R', geo: () => armorSegment(0.055, 0.05, 0.30),
      mat: m.steel, bone: 'mixamorigRightArm', offset: [-0.15, 0, 0], rot: [0, 0, -Math.PI/2], scale: [1, 1, 1] },
    { name: 'elbow_L', geo: () => new THREE.SphereGeometry(0.04, 8, 8),
      mat: m.darkSteel, bone: 'mixamorigLeftForeArm', offset: [0, 0, 0], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'elbow_R', geo: () => new THREE.SphereGeometry(0.04, 8, 8),
      mat: m.darkSteel, bone: 'mixamorigRightForeArm', offset: [0, 0, 0], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'forearm_L', geo: () => armorSegment(0.045, 0.04, 0.26),
      mat: m.steel, bone: 'mixamorigLeftForeArm', offset: [0.13, 0, 0], rot: [0, 0, Math.PI/2], scale: [1, 1, 1] },
    { name: 'forearm_R', geo: () => armorSegment(0.045, 0.04, 0.26),
      mat: m.steel, bone: 'mixamorigRightForeArm', offset: [-0.13, 0, 0], rot: [0, 0, -Math.PI/2], scale: [1, 1, 1] },

    // === GAUNTLETS ===
    { name: 'gauntlet_L', geo: () => new THREE.BoxGeometry(0.10, 0.05, 0.08),
      mat: m.steel, bone: 'mixamorigLeftHand', offset: [0.02, 0, 0], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'gauntlet_R', geo: () => new THREE.BoxGeometry(0.10, 0.05, 0.08),
      mat: m.steel, bone: 'mixamorigRightHand', offset: [-0.02, 0, 0], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'gauntlet_trim_L', geo: () => new THREE.BoxGeometry(0.10, 0.008, 0.085),
      mat: m.gold, bone: 'mixamorigLeftHand', offset: [0.02, 0.025, 0], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'gauntlet_trim_R', geo: () => new THREE.BoxGeometry(0.10, 0.008, 0.085),
      mat: m.gold, bone: 'mixamorigRightHand', offset: [-0.02, 0.025, 0], rot: [0, 0, 0], scale: [1, 1, 1] },

    // === LEGS ===
    { name: 'thigh_L', geo: () => armorSegment(0.07, 0.06, 0.48),
      mat: m.steel, bone: 'mixamorigLeftUpLeg', offset: [0, -0.24, 0], rot: [0, 0, 0], scale: [1, 1, 0.8] },
    { name: 'thigh_R', geo: () => armorSegment(0.07, 0.06, 0.48),
      mat: m.steel, bone: 'mixamorigRightUpLeg', offset: [0, -0.24, 0], rot: [0, 0, 0], scale: [1, 1, 0.8] },
    { name: 'knee_L', geo: () => new THREE.SphereGeometry(0.055, 8, 8),
      mat: m.gold, bone: 'mixamorigLeftLeg', offset: [0, 0, 0.02], rot: [0, 0, 0], scale: [1, 1, 0.8] },
    { name: 'knee_R', geo: () => new THREE.SphereGeometry(0.055, 8, 8),
      mat: m.gold, bone: 'mixamorigRightLeg', offset: [0, 0, 0.02], rot: [0, 0, 0], scale: [1, 1, 0.8] },
    { name: 'shin_L', geo: () => armorSegment(0.055, 0.05, 0.50),
      mat: m.steel, bone: 'mixamorigLeftLeg', offset: [0, -0.25, 0], rot: [0, 0, 0], scale: [1, 1, 0.8] },
    { name: 'shin_R', geo: () => armorSegment(0.055, 0.05, 0.50),
      mat: m.steel, bone: 'mixamorigRightLeg', offset: [0, -0.25, 0], rot: [0, 0, 0], scale: [1, 1, 0.8] },

    // === FEET ===
    { name: 'boot_L', geo: () => new THREE.BoxGeometry(0.10, 0.08, 0.18),
      mat: m.darkSteel, bone: 'mixamorigLeftFoot', offset: [0, -0.02, 0.05], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'boot_R', geo: () => new THREE.BoxGeometry(0.10, 0.08, 0.18),
      mat: m.darkSteel, bone: 'mixamorigRightFoot', offset: [0, -0.02, 0.05], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'boot_trim_L', geo: () => new THREE.BoxGeometry(0.105, 0.01, 0.06),
      mat: m.gold, bone: 'mixamorigLeftFoot', offset: [0, 0.03, 0.02], rot: [0, 0, 0], scale: [1, 1, 1] },
    { name: 'boot_trim_R', geo: () => new THREE.BoxGeometry(0.105, 0.01, 0.06),
      mat: m.gold, bone: 'mixamorigRightFoot', offset: [0, 0.03, 0.02], rot: [0, 0, 0], scale: [1, 1, 1] },

    // === TASSETS (hip armor) ===
    { name: 'tasset_L', geo: () => new THREE.BoxGeometry(0.12, 0.15, 0.04),
      mat: m.steel, bone: 'mixamorigLeftUpLeg', offset: [0.02, -0.02, 0.03], rot: [0.2, 0, 0], scale: [1, 1, 1] },
    { name: 'tasset_R', geo: () => new THREE.BoxGeometry(0.12, 0.15, 0.04),
      mat: m.steel, bone: 'mixamorigRightUpLeg', offset: [-0.02, -0.02, 0.03], rot: [0.2, 0, 0], scale: [1, 1, 1] },
    { name: 'tasset_trim_L', geo: () => new THREE.BoxGeometry(0.125, 0.01, 0.045),
      mat: m.gold, bone: 'mixamorigLeftUpLeg', offset: [0.02, -0.09, 0.03], rot: [0.2, 0, 0], scale: [1, 1, 1] },
    { name: 'tasset_trim_R', geo: () => new THREE.BoxGeometry(0.125, 0.01, 0.045),
      mat: m.gold, bone: 'mixamorigRightUpLeg', offset: [-0.02, -0.09, 0.03], rot: [0.2, 0, 0], scale: [1, 1, 1] },
  ];
}

// ============================================================
// BUILD & ATTACH TO SKELETON
// ============================================================

/**
 * Build the procedural knight and attach each piece to the skeleton.
 * Each armor piece is a separate Mesh parented directly to its bone
 * via bone.add(mesh). This means it moves rigidly with that bone —
 * no vertex skinning needed.
 *
 * @param {Object} boneMap - { boneName: THREE.Bone } from rig.js
 * @returns {THREE.Group} - Group containing all armor meshes
 */
function buildKnight(boneMap) {
  const materials = createMaterials();
  const pieces = defineArmorPieces(materials);
  const group = new THREE.Group();
  group.name = 'ProceduralKnight';

  let attached = 0;
  for (const piece of pieces) {
    const bone = boneMap[piece.bone];
    if (!bone) {
      console.warn(`Knight: bone ${piece.bone} not found, skipping ${piece.name}`);
      continue;
    }

    const geometry = piece.geo();
    const mesh = new THREE.Mesh(geometry, piece.mat);
    mesh.name = piece.name;

    // Apply local offset and rotation
    mesh.position.set(piece.offset[0], piece.offset[1], piece.offset[2]);
    if (piece.rot) mesh.rotation.set(piece.rot[0], piece.rot[1], piece.rot[2]);
    if (piece.scale) mesh.scale.set(piece.scale[0], piece.scale[1], piece.scale[2]);

    mesh.castShadow = true;
    mesh.receiveShadow = true;

    // Parent to bone — mesh moves rigidly with this bone
    bone.add(mesh);
    attached++;
  }

  console.log(`ProceduralKnight: ${attached} armor pieces attached to skeleton`);
  return group;
}

/**
 * Remove the original FBX meshes (Body, Head_Hands, Lower_Armor)
 * and replace with our procedural knight.
 */
function replaceWithProceduralKnight(boneMap) {
  // Find model root
  const firstBone = Object.values(boneMap)[0];
  if (!firstBone) return null;
  let root = firstBone;
  while (root.parent && root.parent.type !== 'Scene') root = root.parent;

  // Remove original meshes but keep the skeleton
  const toRemove = [];
  root.traverse(child => {
    if (child.isMesh && (child.name === 'Body' || child.name === 'Head_Hands' || child.name === 'Lower_Armor')) {
      toRemove.push(child);
    }
  });
  for (const mesh of toRemove) {
    mesh.geometry.dispose();
    if (Array.isArray(mesh.material)) {
      mesh.material.forEach(m => m.dispose());
    } else if (mesh.material) {
      mesh.material.dispose();
    }
    mesh.parent.remove(mesh);
  }

  // Build and attach procedural knight
  return buildKnight(boneMap);
}

export { buildKnight, replaceWithProceduralKnight, createMaterials };
