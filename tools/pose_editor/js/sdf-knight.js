/**
 * sdf-knight.js — Procedural knight via Signed Distance Fields + Marching Cubes
 *
 * Defines the knight's body and armor as mathematical field functions,
 * extracts the isosurface with marching cubes, computes skinning weights
 * from bone distances, and produces a single continuous skinned mesh.
 *
 * Math:
 *   Capsule SDF:    d(p) = |closest_on_segment(p, A, B) - p| - radius
 *   Smooth union:   smin(a, b, k) = -ln(e^{-ka} + e^{-kb}) / k
 *   Box SDF:        d(p) = |max(|p - center| - half, 0)| - bevel
 *   Sphere SDF:     d(p) = |p - center| - radius
 *   Skinning:       weight_i(v) = softmax(-distance(v, bone_i) / sigma)
 */
import { THREE, scene, envMap } from './core.js';

// ============================================================
// SDF PRIMITIVES
// ============================================================

function sdCapsule(p, a, b, r) {
  const pa = [p[0]-a[0], p[1]-a[1], p[2]-a[2]];
  const ba = [b[0]-a[0], b[1]-a[1], b[2]-a[2]];
  const baDot = ba[0]*ba[0] + ba[1]*ba[1] + ba[2]*ba[2];
  let h = (pa[0]*ba[0] + pa[1]*ba[1] + pa[2]*ba[2]) / (baDot || 1e-8);
  h = Math.max(0, Math.min(1, h));
  const dx = pa[0] - ba[0]*h, dy = pa[1] - ba[1]*h, dz = pa[2] - ba[2]*h;
  return Math.sqrt(dx*dx + dy*dy + dz*dz) - r;
}

function sdSphere(p, center, r) {
  const dx = p[0]-center[0], dy = p[1]-center[1], dz = p[2]-center[2];
  return Math.sqrt(dx*dx + dy*dy + dz*dz) - r;
}

function sdBox(p, center, half, bevel) {
  const dx = Math.abs(p[0]-center[0]) - half[0];
  const dy = Math.abs(p[1]-center[1]) - half[1];
  const dz = Math.abs(p[2]-center[2]) - half[2];
  const mx = Math.max(dx, 0), my = Math.max(dy, 0), mz = Math.max(dz, 0);
  return Math.sqrt(mx*mx + my*my + mz*mz) + Math.min(Math.max(dx, dy, dz), 0) - (bevel || 0);
}

function sdTorus(p, center, R, r, axis) {
  // Torus in XZ plane by default (axis='y'), shifted to center
  const px = p[0]-center[0], py = p[1]-center[1], pz = p[2]-center[2];
  let q0, q1;
  if (axis === 'x') {
    q0 = Math.sqrt(py*py + pz*pz) - R;
    q1 = px;
  } else if (axis === 'z') {
    q0 = Math.sqrt(px*px + py*py) - R;
    q1 = pz;
  } else {
    q0 = Math.sqrt(px*px + pz*pz) - R;
    q1 = py;
  }
  return Math.sqrt(q0*q0 + q1*q1) - r;
}

// Polynomial smooth minimum — blends two SDFs with controllable radius.
// k = blend radius (in same units as the SDF). Numerically stable at any scale.
// From Inigo Quilez: https://iquilezles.org/articles/smin/
function smin(a, b, k) {
  if (k <= 0) return Math.min(a, b);
  const h = Math.max(k - Math.abs(a - b), 0) / k;
  return Math.min(a, b) - h * h * h * k * (1 / 6);
}

// Sharp union
function opUnion(a, b) { return Math.min(a, b); }

// Subtraction
function opSubtract(a, b) { return Math.max(a, -b); }

// ============================================================
// KNIGHT BODY DEFINITION
// ============================================================

/**
 * Define the knight's SDF from skeleton bone positions.
 *
 * @param {Object} bones - { boneName: [x, y, z] } world positions
 * @returns {Function} sdf(p) -> distance
 */
function defineKnightSDF(bones) {
  const B = bones;
  const K = 8; // smooth blend radius in cm (higher = smoother joint transitions)

  // All dimensions in CENTIMETERS to match Mixamo model space
  return function sdf(p) {
    // === BODY (smooth union of capsules along skeleton) ===
    let body = 1e6;

    // Torso
    body = smin(body, sdCapsule(p, B.hips, B.spine, 10), K);
    body = smin(body, sdCapsule(p, B.spine, B.spine1, 9.5), K);
    body = smin(body, sdCapsule(p, B.spine1, B.spine2, 10), K);
    body = smin(body, sdCapsule(p, B.spine2, B.neck, 8), K);
    body = smin(body, sdCapsule(p, B.neck, B.head, 6), K);

    // Left arm
    body = smin(body, sdCapsule(p, B.lShoulder, B.lArm, 5.5), K);
    body = smin(body, sdCapsule(p, B.lArm, B.lForeArm, 4.2), K);
    body = smin(body, sdCapsule(p, B.lForeArm, B.lHand, 3.5), K);

    // Right arm
    body = smin(body, sdCapsule(p, B.rShoulder, B.rArm, 5.5), K);
    body = smin(body, sdCapsule(p, B.rArm, B.rForeArm, 4.2), K);
    body = smin(body, sdCapsule(p, B.rForeArm, B.rHand, 3.5), K);

    // Left leg
    body = smin(body, sdCapsule(p, B.lUpLeg, B.lLeg, 5.8), K);
    body = smin(body, sdCapsule(p, B.lLeg, B.lFoot, 4.8), K);
    body = smin(body, sdCapsule(p, B.lFoot, B.lToe, 3.8), K);

    // Right leg
    body = smin(body, sdCapsule(p, B.rUpLeg, B.rLeg, 5.8), K);
    body = smin(body, sdCapsule(p, B.rLeg, B.rFoot, 4.8), K);
    body = smin(body, sdCapsule(p, B.rFoot, B.rToe, 3.8), K);

    // Head sphere
    const headCenter = [(B.head[0]+B.headTop[0])/2, (B.head[1]+B.headTop[1])/2, (B.head[2]+B.headTop[2])/2];
    body = smin(body, sdSphere(p, headCenter, 10), K);

    // === ARMOR (sharper unions — plate edges) ===
    let armor = 1e6;
    const KA = 3; // sharper blend radius for armor edges (cm)

    // Breastplate — thick slab over chest
    const chestCenter = [(B.spine1[0]+B.spine2[0])/2, (B.spine1[1]+B.spine2[1])/2, (B.spine1[2]+B.spine2[2])/2 + 4];
    armor = opUnion(armor, sdBox(p, chestCenter, [12, 11, 4], 1));

    // Pauldrons — spheres on shoulders
    armor = opUnion(armor, sdSphere(p, [B.lArm[0]+3, B.lArm[1]+4, B.lArm[2]], 7));
    armor = opUnion(armor, sdSphere(p, [B.rArm[0]-3, B.rArm[1]+4, B.rArm[2]], 7));

    // Helmet — sphere + box visor
    armor = opUnion(armor, sdSphere(p, headCenter, 12.5));
    // Visor slit subtraction
    const visorCenter = [headCenter[0], headCenter[1]-2, headCenter[2]+12];
    armor = opSubtract(armor, sdBox(p, visorCenter, [6, 0.8, 3], 0));

    // Tassets — plate skirts at hips
    const lTasset = [(B.hips[0]+B.lUpLeg[0])/2 + 2, B.hips[1]-6, B.hips[2]+4];
    const rTasset = [(B.hips[0]+B.rUpLeg[0])/2 - 2, B.hips[1]-6, B.hips[2]+4];
    armor = opUnion(armor, sdBox(p, lTasset, [6, 7, 2], 0.5));
    armor = opUnion(armor, sdBox(p, rTasset, [6, 7, 2], 0.5));

    // Knee guards
    armor = opUnion(armor, sdSphere(p, [B.lLeg[0], B.lLeg[1], B.lLeg[2]+2], 4.5));
    armor = opUnion(armor, sdSphere(p, [B.rLeg[0], B.rLeg[1], B.rLeg[2]+2], 4.5));

    // Greaves — slight bulge on shins
    const lShin = [(B.lLeg[0]+B.lFoot[0])/2, (B.lLeg[1]+B.lFoot[1])/2, (B.lLeg[2]+B.lFoot[2])/2+1];
    const rShin = [(B.rLeg[0]+B.rFoot[0])/2, (B.rLeg[1]+B.rFoot[1])/2, (B.rLeg[2]+B.rFoot[2])/2+1];
    armor = opUnion(armor, sdBox(p, lShin, [4, 20, 3], 1));
    armor = opUnion(armor, sdBox(p, rShin, [4, 20, 3], 1));

    // Boots — boxes at feet
    armor = opUnion(armor, sdBox(p, [B.lFoot[0], B.lFoot[1]-1, B.lFoot[2]+4], [4.5, 4, 8], 1));
    armor = opUnion(armor, sdBox(p, [B.rFoot[0], B.rFoot[1]-1, B.rFoot[2]+4], [4.5, 4, 8], 1));

    // Combine body + armor with smooth blend
    return smin(body, armor, KA);
  };
}

/**
 * Define a gold trim SDF — thin features on edges of armor pieces.
 */
function defineGoldSDF(bones) {
  const B = bones;
  return function goldSdf(p) {
    let d = 1e6;
    const headCenter = [(B.head[0]+B.headTop[0])/2, (B.head[1]+B.headTop[1])/2, (B.head[2]+B.headTop[2])/2];

    // All dimensions in CENTIMETERS

    // Helmet crest
    d = opUnion(d, sdBox(p, [headCenter[0], headCenter[1]+10, headCenter[2]], [0.8, 4, 8], 0.3));

    // Chest cross
    const chestCenter = [(B.spine1[0]+B.spine2[0])/2, (B.spine1[1]+B.spine2[1])/2, (B.spine1[2]+B.spine2[2])/2 + 8.5];
    d = opUnion(d, sdBox(p, chestCenter, [0.3, 9, 0.3], 0.2));  // vertical
    d = opUnion(d, sdBox(p, chestCenter, [10, 0.3, 0.3], 0.2));  // horizontal

    // Pauldron rims
    d = opUnion(d, sdTorus(p, [B.lArm[0]+3, B.lArm[1]+2, B.lArm[2]], 6.5, 0.7, 'y'));
    d = opUnion(d, sdTorus(p, [B.rArm[0]-3, B.rArm[1]+2, B.rArm[2]], 6.5, 0.7, 'y'));

    // Waist belt
    const waist = [B.hips[0], B.hips[1]+4, B.hips[2]];
    d = opUnion(d, sdTorus(p, waist, 11, 0.8, 'y'));

    // Knee caps
    d = opUnion(d, sdSphere(p, [B.lLeg[0], B.lLeg[1], B.lLeg[2]+3.5], 2.5));
    d = opUnion(d, sdSphere(p, [B.rLeg[0], B.rLeg[1], B.rLeg[2]+3.5], 2.5));

    // Boot rims
    d = opUnion(d, sdTorus(p, [B.lFoot[0], B.lFoot[1]+2, B.lFoot[2]+2], 4, 0.6, 'y'));
    d = opUnion(d, sdTorus(p, [B.rFoot[0], B.rFoot[1]+2, B.rFoot[2]+2], 4, 0.6, 'y'));

    // Gauntlet rims
    d = opUnion(d, sdTorus(p, B.lHand, 3, 0.5, 'x'));
    d = opUnion(d, sdTorus(p, B.rHand, 3, 0.5, 'x'));

    return d;
  };
}

// ============================================================
// MARCHING CUBES
// ============================================================

// Edge table and triangle table for marching cubes
// (standard 256-entry lookup tables — included inline for self-containment)
const EDGE_TABLE = new Int32Array([0x0,0x109,0x203,0x30a,0x406,0x50f,0x605,0x70c,0x80c,0x905,0xa0f,0xb06,0xc0a,0xd03,0xe09,0xf00,0x190,0x99,0x393,0x29a,0x596,0x49f,0x795,0x69c,0x99c,0x895,0xb9f,0xa96,0xd9a,0xc93,0xf99,0xe90,0x230,0x339,0x33,0x13a,0x636,0x73f,0x435,0x53c,0xa3c,0xb35,0x83f,0x936,0xe3a,0xf33,0xc39,0xd30,0x3a0,0x2a9,0x1a3,0xaa,0x7a6,0x6af,0x5a5,0x4ac,0xbac,0xaa5,0x9af,0x8a6,0xfaa,0xea3,0xda9,0xca0,0x460,0x569,0x663,0x76a,0x66,0x16f,0x265,0x36c,0xc6c,0xd65,0xe6f,0xf66,0x86a,0x963,0xa69,0xb60,0x5f0,0x4f9,0x7f3,0x6fa,0x1f6,0xff,0x3f5,0x2fc,0xdfc,0xcf5,0xfff,0xef6,0x9fa,0x8f3,0xbf9,0xaf0,0x650,0x759,0x453,0x55a,0x256,0x35f,0x55,0x15c,0xe5c,0xf55,0xc5f,0xd56,0xa5a,0xb53,0x859,0x950,0x7c0,0x6c9,0x5c3,0x4ca,0x3c6,0x2cf,0x1c5,0xcc,0xfcc,0xec5,0xdcf,0xcc6,0xbca,0xac3,0x9c9,0x8c0,0x8c0,0x9c9,0xac3,0xbca,0xcc6,0xdcf,0xec5,0xfcc,0xcc,0x1c5,0x2cf,0x3c6,0x4ca,0x5c3,0x6c9,0x7c0,0x950,0x859,0xb53,0xa5a,0xd56,0xc5f,0xf55,0xe5c,0x15c,0x55,0x35f,0x256,0x55a,0x453,0x759,0x650,0xaf0,0xbf9,0x8f3,0x9fa,0xef6,0xfff,0xcf5,0xdfc,0x2fc,0x3f5,0xff,0x1f6,0x6fa,0x7f3,0x4f9,0x5f0,0xb60,0xa69,0x963,0x86a,0xf66,0xe6f,0xd65,0xc6c,0x36c,0x265,0x16f,0x66,0x76a,0x663,0x569,0x460,0xca0,0xda9,0xea3,0xfaa,0x8a6,0x9af,0xaa5,0xbac,0x4ac,0x5a5,0x6af,0x7a6,0xaa,0x1a3,0x2a9,0x3a0,0xd30,0xc39,0xf33,0xe3a,0x936,0x83f,0xb35,0xa3c,0x53c,0x435,0x73f,0x636,0x13a,0x33,0x339,0x230,0xe90,0xf99,0xc93,0xd9a,0xa96,0xb9f,0x895,0x99c,0x69c,0x795,0x49f,0x596,0x29a,0x393,0x99,0x190,0xf00,0xe09,0xd03,0xc0a,0xb06,0xa0f,0x905,0x80c,0x70c,0x605,0x50f,0x406,0x30a,0x203,0x109,0x0]);

// Compressed tri table — each cube config has up to 15 vertex indices (-1 terminated)
// Using the standard marching cubes triangle table
const TRI_TABLE = [[-1],
[0,8,3,-1],[0,1,9,-1],[1,8,3,9,8,1,-1],[1,2,10,-1],[0,8,3,1,2,10,-1],[9,2,10,0,2,9,-1],[2,8,3,2,10,8,10,9,8,-1],[3,11,2,-1],[0,11,2,8,11,0,-1],[1,9,0,2,3,11,-1],[1,11,2,1,9,11,9,8,11,-1],[3,10,1,11,10,3,-1],[0,10,1,0,8,10,8,11,10,-1],[3,9,0,3,11,9,11,10,9,-1],[9,8,10,10,8,11,-1],
[4,7,8,-1],[4,3,0,7,3,4,-1],[0,1,9,8,4,7,-1],[4,1,9,4,7,1,7,3,1,-1],[1,2,10,8,4,7,-1],[3,4,7,3,0,4,1,2,10,-1],[9,2,10,9,0,2,8,4,7,-1],[2,10,9,2,9,7,2,7,3,7,9,4,-1],[8,4,7,3,11,2,-1],[11,4,7,11,2,4,2,0,4,-1],[9,0,1,8,4,7,2,3,11,-1],[4,7,11,9,4,11,9,11,2,9,2,1,-1],[3,10,1,3,11,10,7,8,4,-1],[1,11,10,1,4,11,1,0,4,7,11,4,-1],[4,7,8,9,0,11,9,11,10,11,0,3,-1],[4,7,11,4,11,9,9,11,10,-1],
[9,5,4,-1],[9,5,4,0,8,3,-1],[0,5,4,1,5,0,-1],[8,5,4,8,3,5,3,1,5,-1],[1,2,10,9,5,4,-1],[3,0,8,1,2,10,4,9,5,-1],[5,2,10,5,4,2,4,0,2,-1],[2,10,5,3,2,5,3,5,4,3,4,8,-1],[9,5,4,2,3,11,-1],[0,11,2,0,8,11,4,9,5,-1],[0,5,4,0,1,5,2,3,11,-1],[2,1,5,2,5,8,2,8,11,4,8,5,-1],[10,3,11,10,1,3,9,5,4,-1],[4,9,5,0,8,1,8,10,1,8,11,10,-1],[5,4,0,5,0,11,5,11,10,11,0,3,-1],[5,4,8,5,8,10,10,8,11,-1],
[9,7,8,5,7,9,-1],[9,3,0,9,5,3,5,7,3,-1],[0,7,8,0,1,7,1,5,7,-1],[1,5,3,3,5,7,-1],[9,7,8,9,5,7,10,1,2,-1],[10,1,2,9,5,3,5,7,3,9,3,0,-1],[8,0,2,8,2,5,8,5,7,10,5,2,-1],[2,10,5,2,5,3,3,5,7,-1],[7,9,5,7,8,9,3,11,2,-1],[9,5,7,9,7,2,9,2,0,2,7,11,-1],[2,3,11,0,1,8,1,7,8,1,5,7,-1],[11,2,1,11,1,7,7,1,5,-1],[9,5,8,8,5,7,10,1,3,10,3,11,-1],[5,7,0,5,0,9,7,11,0,1,0,10,11,10,0,-1],[11,10,0,11,0,3,10,5,0,8,0,7,5,7,0,-1],[11,10,5,7,11,5,-1],
[10,6,5,-1],[0,8,3,5,10,6,-1],[9,0,1,5,10,6,-1],[1,8,3,1,9,8,5,10,6,-1],[1,6,5,2,6,1,-1],[1,6,5,1,2,6,3,0,8,-1],[9,6,5,9,0,6,0,2,6,-1],[5,9,8,5,8,2,5,2,6,3,2,8,-1],[2,3,11,10,6,5,-1],[11,0,8,11,2,0,10,6,5,-1],[0,1,9,2,3,11,5,10,6,-1],[5,10,6,1,9,2,9,11,2,9,8,11,-1],[6,3,11,6,5,3,5,1,3,-1],[0,8,11,0,11,5,0,5,1,5,11,6,-1],[3,11,6,0,3,6,0,6,5,0,5,9,-1],[6,5,9,6,9,11,11,9,8,-1],
[5,10,6,4,7,8,-1],[4,3,0,4,7,3,6,5,10,-1],[1,9,0,5,10,6,8,4,7,-1],[10,6,5,1,9,7,1,7,3,7,9,4,-1],[6,1,2,6,5,1,4,7,8,-1],[1,2,5,5,2,6,3,0,4,3,4,7,-1],[8,4,7,9,0,5,0,6,5,0,2,6,-1],[7,3,9,7,9,4,3,2,9,5,9,6,2,6,9,-1],[3,11,2,7,8,4,10,6,5,-1],[5,10,6,4,7,2,4,2,0,2,7,11,-1],[0,1,9,4,7,8,2,3,11,5,10,6,-1],[9,2,1,9,11,2,9,4,11,7,11,4,5,10,6,-1],[8,4,7,3,11,5,3,5,1,5,11,6,-1],[5,1,11,5,11,6,1,0,11,7,11,4,0,4,11,-1],[0,5,9,0,6,5,0,3,6,11,6,3,8,4,7,-1],[6,5,9,6,9,11,4,7,9,7,11,9,-1],
[10,4,9,6,4,10,-1],[4,10,6,4,9,10,0,8,3,-1],[10,0,1,10,6,0,6,4,0,-1],[8,3,1,8,1,6,8,6,4,6,1,10,-1],[1,4,9,1,2,4,2,6,4,-1],[3,0,8,1,2,4,2,6,4,1,4,9,-1],[0,2,4,4,2,6,-1],[8,3,2,8,2,4,4,2,6,-1],[10,4,9,10,6,4,11,2,3,-1],[0,8,2,2,8,11,4,9,10,4,10,6,-1],[3,11,2,0,1,6,0,6,4,6,1,10,-1],[6,4,1,6,1,10,4,8,1,2,1,11,8,11,1,-1],[9,6,4,9,3,6,9,1,3,11,6,3,-1],[8,11,1,8,1,0,11,6,1,9,1,4,6,4,1,-1],[3,11,6,3,6,0,0,6,4,-1],[6,4,8,11,6,8,-1],
[7,10,6,7,8,10,8,9,10,-1],[0,7,3,0,10,7,0,9,10,6,7,10,-1],[10,6,7,1,10,7,1,7,8,1,8,0,-1],[10,6,7,10,7,1,1,7,3,-1],[1,2,6,1,6,8,1,8,9,8,6,7,-1],[2,6,9,2,9,1,6,7,9,0,9,3,7,3,9,-1],[7,8,0,7,0,6,6,0,2,-1],[7,3,2,6,7,2,-1],[2,3,11,10,6,8,10,8,9,8,6,7,-1],[2,0,7,2,7,11,0,9,7,6,7,10,9,10,7,-1],[1,8,0,1,7,8,1,10,7,6,7,10,2,3,11,-1],[11,2,1,11,1,7,10,6,1,6,7,1,-1],[8,9,6,8,6,7,9,1,6,11,6,3,1,3,6,-1],[0,9,1,11,6,7,-1],[7,8,0,7,0,6,3,11,0,11,6,0,-1],[7,11,6,-1],
[7,6,11,-1],[3,0,8,11,7,6,-1],[0,1,9,11,7,6,-1],[8,1,9,8,3,1,11,7,6,-1],[10,1,2,6,11,7,-1],[1,2,10,3,0,8,6,11,7,-1],[2,9,0,2,10,9,6,11,7,-1],[6,11,7,2,10,3,10,8,3,10,9,8,-1],[7,2,3,6,2,7,-1],[7,0,8,7,6,0,6,2,0,-1],[2,7,6,2,3,7,0,1,9,-1],[1,6,2,1,8,6,1,9,8,8,7,6,-1],[10,7,6,10,1,7,1,3,7,-1],[10,7,6,1,7,10,1,8,7,1,0,8,-1],[0,3,7,0,7,10,0,10,9,6,10,7,-1],[7,6,10,7,10,8,8,10,9,-1],
[6,8,4,11,8,6,-1],[3,6,11,3,0,6,0,4,6,-1],[8,6,11,8,4,6,9,0,1,-1],[9,4,6,9,6,3,9,3,1,11,3,6,-1],[6,8,4,6,11,8,2,10,1,-1],[1,2,10,3,0,11,0,6,11,0,4,6,-1],[4,11,8,4,6,11,0,2,9,2,10,9,-1],[10,9,3,10,3,2,9,4,3,11,3,6,4,6,3,-1],[8,2,3,8,4,2,4,6,2,-1],[0,4,2,4,6,2,-1],[1,9,0,2,3,4,2,4,6,4,3,8,-1],[1,9,4,1,4,2,2,4,6,-1],[8,1,3,8,6,1,8,4,6,6,10,1,-1],[10,1,0,10,0,6,6,0,4,-1],[4,6,3,4,3,8,6,10,3,0,3,9,10,9,3,-1],[10,9,4,6,10,4,-1],
[4,9,5,7,6,11,-1],[0,8,3,4,9,5,11,7,6,-1],[5,0,1,5,4,0,7,6,11,-1],[11,7,6,8,3,4,3,5,4,3,1,5,-1],[9,5,4,10,1,2,7,6,11,-1],[6,11,7,1,2,10,0,8,3,4,9,5,-1],[7,6,11,5,4,10,4,2,10,4,0,2,-1],[3,4,8,3,5,4,3,2,5,10,5,2,11,7,6,-1],[7,2,3,7,6,2,5,4,9,-1],[9,5,4,0,8,6,0,6,2,6,8,7,-1],[3,6,2,3,7,6,1,5,0,5,4,0,-1],[6,2,8,6,8,7,2,1,8,4,8,5,1,5,8,-1],[9,5,4,10,1,6,1,7,6,1,3,7,-1],[1,6,10,1,7,6,1,0,7,8,7,0,9,5,4,-1],[4,0,10,4,10,5,0,3,10,6,10,7,3,7,10,-1],[7,6,10,7,10,8,5,4,10,4,8,10,-1],
[6,9,5,6,11,9,11,8,9,-1],[3,6,11,0,6,3,0,5,6,0,9,5,-1],[0,11,8,0,5,11,0,1,5,5,6,11,-1],[6,11,3,6,3,5,5,3,1,-1],[1,2,10,9,5,11,9,11,8,11,5,6,-1],[0,11,3,0,6,11,0,9,6,5,6,9,1,2,10,-1],[11,8,5,11,5,6,8,0,5,10,5,2,0,2,5,-1],[6,11,3,6,3,5,2,10,3,10,5,3,-1],[5,8,9,5,2,8,5,6,2,3,8,2,-1],[9,5,6,9,6,0,0,6,2,-1],[1,5,8,1,8,0,5,6,8,3,8,2,6,2,8,-1],[1,5,6,2,1,6,-1],[1,3,6,1,6,10,3,8,6,5,6,9,8,9,6,-1],[10,1,0,10,0,6,9,5,0,5,6,0,-1],[0,3,8,5,6,10,-1],[10,5,6,-1],
[11,5,10,7,5,11,-1],[11,5,10,11,7,5,8,3,0,-1],[5,11,7,5,10,11,1,9,0,-1],[10,7,5,10,11,7,9,8,1,8,3,1,-1],[11,1,2,11,7,1,7,5,1,-1],[0,8,3,1,2,7,1,7,5,7,2,11,-1],[9,7,5,9,2,7,9,0,2,2,11,7,-1],[7,5,2,7,2,11,5,9,2,3,2,8,9,8,2,-1],[2,5,10,2,3,5,3,7,5,-1],[8,2,0,8,5,2,8,7,5,10,2,5,-1],[9,0,1,5,10,3,5,3,7,3,10,2,-1],[9,8,2,9,2,1,8,7,2,10,2,5,7,5,2,-1],[1,3,5,3,7,5,-1],[0,8,7,0,7,1,1,7,5,-1],[9,0,3,9,3,5,5,3,7,-1],[9,8,7,5,9,7,-1],
[5,8,4,5,10,8,10,11,8,-1],[5,0,4,5,11,0,5,10,11,11,3,0,-1],[0,1,9,8,4,10,8,10,11,10,4,5,-1],[10,11,4,10,4,5,11,3,4,9,4,1,3,1,4,-1],[2,5,1,2,8,5,2,11,8,4,5,8,-1],[0,4,11,0,11,3,4,5,11,2,11,1,5,1,11,-1],[0,2,5,0,5,9,2,11,5,4,5,8,11,8,5,-1],[9,4,5,2,11,3,-1],[2,5,10,3,5,2,3,4,5,3,8,4,-1],[5,10,2,5,2,4,4,2,0,-1],[3,10,2,3,5,10,3,8,5,4,5,8,0,1,9,-1],[5,10,2,5,2,4,1,9,2,9,4,2,-1],[8,4,5,8,5,3,3,5,1,-1],[0,4,5,1,0,5,-1],[8,4,5,8,5,3,9,0,5,0,3,5,-1],[9,4,5,-1],
[4,11,7,4,9,11,9,10,11,-1],[0,8,3,4,9,7,9,11,7,9,10,11,-1],[1,10,11,1,11,4,1,4,0,7,4,11,-1],[3,1,4,3,4,8,1,10,4,7,4,11,10,11,4,-1],[4,11,7,9,11,4,9,2,11,9,1,2,-1],[9,7,4,9,11,7,9,1,11,2,11,1,0,8,3,-1],[11,7,4,11,4,2,2,4,0,-1],[11,7,4,11,4,2,8,3,4,3,2,4,-1],[2,9,10,2,7,9,2,3,7,7,4,9,-1],[9,10,7,9,7,4,10,2,7,8,7,0,2,0,7,-1],[3,7,10,3,10,2,7,4,10,1,10,0,4,0,10,-1],[1,10,2,8,7,4,-1],[4,9,1,4,1,7,7,1,3,-1],[4,9,1,4,1,7,0,8,1,8,7,1,-1],[4,0,3,7,4,3,-1],[4,8,7,-1],
[9,10,8,10,11,8,-1],[3,0,9,3,9,11,11,9,10,-1],[0,1,10,0,10,8,8,10,11,-1],[3,1,10,11,3,10,-1],[1,2,11,1,11,9,9,11,8,-1],[3,0,9,3,9,11,1,2,9,2,11,9,-1],[0,2,11,8,0,11,-1],[3,2,11,-1],[2,3,8,2,8,10,10,8,9,-1],[9,10,2,0,9,2,-1],[2,3,8,2,8,10,0,1,8,1,10,8,-1],[1,10,2,-1],[1,3,8,9,1,8,-1],[0,9,1,-1],[0,3,8,-1],[-1]];

/**
 * Run marching cubes on an SDF to extract a triangle mesh.
 *
 * @param {Function} sdf - sdf(p) -> distance
 * @param {Array} min - [x,y,z] bounding box minimum
 * @param {Array} max - [x,y,z] bounding box maximum
 * @param {number} res - grid resolution per axis
 * @returns {{ positions: Float32Array, normals: Float32Array }}
 */
function marchingCubes(sdf, min, max, res) {
  const dx = (max[0]-min[0])/res;
  const dy = (max[1]-min[1])/res;
  const dz = (max[2]-min[2])/res;

  // Evaluate SDF on grid
  const grid = new Float32Array((res+1)*(res+1)*(res+1));
  const idx = (ix, iy, iz) => ix + (res+1) * (iy + (res+1) * iz);

  for (let iz = 0; iz <= res; iz++)
    for (let iy = 0; iy <= res; iy++)
      for (let ix = 0; ix <= res; ix++) {
        const p = [min[0]+ix*dx, min[1]+iy*dy, min[2]+iz*dz];
        grid[idx(ix,iy,iz)] = sdf(p);
      }

  const verts = [];

  // Interpolate vertex on edge between two grid points
  function interp(ix1,iy1,iz1, ix2,iy2,iz2) {
    const v1 = grid[idx(ix1,iy1,iz1)];
    const v2 = grid[idx(ix2,iy2,iz2)];
    const t = v1 / (v1 - v2);
    return [
      min[0] + (ix1 + t*(ix2-ix1)) * dx,
      min[1] + (iy1 + t*(iy2-iy1)) * dy,
      min[2] + (iz1 + t*(iz2-iz1)) * dz,
    ];
  }

  for (let iz = 0; iz < res; iz++)
    for (let iy = 0; iy < res; iy++)
      for (let ix = 0; ix < res; ix++) {
        // Determine cube index from 8 corner signs
        let cubeIdx = 0;
        if (grid[idx(ix,   iy,   iz  )] < 0) cubeIdx |= 1;
        if (grid[idx(ix+1, iy,   iz  )] < 0) cubeIdx |= 2;
        if (grid[idx(ix+1, iy,   iz+1)] < 0) cubeIdx |= 4;
        if (grid[idx(ix,   iy,   iz+1)] < 0) cubeIdx |= 8;
        if (grid[idx(ix,   iy+1, iz  )] < 0) cubeIdx |= 16;
        if (grid[idx(ix+1, iy+1, iz  )] < 0) cubeIdx |= 32;
        if (grid[idx(ix+1, iy+1, iz+1)] < 0) cubeIdx |= 64;
        if (grid[idx(ix,   iy+1, iz+1)] < 0) cubeIdx |= 128;

        if (cubeIdx === 0 || cubeIdx === 255) continue;

        const edges = EDGE_TABLE[cubeIdx];
        const edgeVerts = new Array(12);

        if (edges & 1)    edgeVerts[0]  = interp(ix,iy,iz, ix+1,iy,iz);
        if (edges & 2)    edgeVerts[1]  = interp(ix+1,iy,iz, ix+1,iy,iz+1);
        if (edges & 4)    edgeVerts[2]  = interp(ix+1,iy,iz+1, ix,iy,iz+1);
        if (edges & 8)    edgeVerts[3]  = interp(ix,iy,iz, ix,iy,iz+1);
        if (edges & 16)   edgeVerts[4]  = interp(ix,iy+1,iz, ix+1,iy+1,iz);
        if (edges & 32)   edgeVerts[5]  = interp(ix+1,iy+1,iz, ix+1,iy+1,iz+1);
        if (edges & 64)   edgeVerts[6]  = interp(ix+1,iy+1,iz+1, ix,iy+1,iz+1);
        if (edges & 128)  edgeVerts[7]  = interp(ix,iy+1,iz, ix,iy+1,iz+1);
        if (edges & 256)  edgeVerts[8]  = interp(ix,iy,iz, ix,iy+1,iz);
        if (edges & 512)  edgeVerts[9]  = interp(ix+1,iy,iz, ix+1,iy+1,iz);
        if (edges & 1024) edgeVerts[10] = interp(ix+1,iy,iz+1, ix+1,iy+1,iz+1);
        if (edges & 2048) edgeVerts[11] = interp(ix,iy,iz+1, ix,iy+1,iz+1);

        const tri = TRI_TABLE[cubeIdx];
        for (let t = 0; tri[t] !== -1; t += 3) {
          verts.push(...edgeVerts[tri[t]]);
          verts.push(...edgeVerts[tri[t+1]]);
          verts.push(...edgeVerts[tri[t+2]]);
        }
      }

  // Build positions array
  const positions = new Float32Array(verts);

  // Compute normals via central difference on SDF
  const eps = dx * 0.5;
  const normals = new Float32Array(positions.length);
  for (let i = 0; i < positions.length; i += 3) {
    const p = [positions[i], positions[i+1], positions[i+2]];
    const nx = sdf([p[0]+eps,p[1],p[2]]) - sdf([p[0]-eps,p[1],p[2]]);
    const ny = sdf([p[0],p[1]+eps,p[2]]) - sdf([p[0],p[1]-eps,p[2]]);
    const nz = sdf([p[0],p[1],p[2]+eps]) - sdf([p[0],p[1],p[2]-eps]);
    const len = Math.sqrt(nx*nx+ny*ny+nz*nz) || 1;
    normals[i] = nx/len;
    normals[i+1] = ny/len;
    normals[i+2] = nz/len;
  }

  return { positions, normals, vertexCount: positions.length / 3 };
}


// ============================================================
// SKINNING WEIGHTS
// ============================================================

/**
 * Compute per-vertex skinning weights from bone distances.
 * Each vertex gets weighted to the 4 nearest bones using softmax.
 *
 * @param {Float32Array} positions - vertex positions (N*3)
 * @param {Array} boneSegments - [{a: [x,y,z], b: [x,y,z], boneIndex: int}, ...]
 * @param {number} sigma - falloff distance (smaller = tighter binding)
 * @returns {{ skinIndices: Float32Array, skinWeights: Float32Array }}
 */
function computeSkinWeights(positions, boneSegments, sigma = 8) {
  const n = positions.length / 3;
  const skinIndices = new Float32Array(n * 4);
  const skinWeights = new Float32Array(n * 4);

  for (let i = 0; i < n; i++) {
    const p = [positions[i*3], positions[i*3+1], positions[i*3+2]];

    // Distance to each bone segment
    const dists = boneSegments.map(seg => {
      const pa = [p[0]-seg.a[0], p[1]-seg.a[1], p[2]-seg.a[2]];
      const ba = [seg.b[0]-seg.a[0], seg.b[1]-seg.a[1], seg.b[2]-seg.a[2]];
      const baDot = ba[0]*ba[0] + ba[1]*ba[1] + ba[2]*ba[2];
      let h = (pa[0]*ba[0] + pa[1]*ba[1] + pa[2]*ba[2]) / (baDot || 1e-8);
      h = Math.max(0, Math.min(1, h));
      const dx = pa[0]-ba[0]*h, dy = pa[1]-ba[1]*h, dz = pa[2]-ba[2]*h;
      return { dist: Math.sqrt(dx*dx+dy*dy+dz*dz), idx: seg.boneIndex };
    });

    // Sort by distance, take top 4
    dists.sort((a, b) => a.dist - b.dist);
    const top4 = dists.slice(0, 4);

    // Softmax weights
    const exps = top4.map(d => Math.exp(-d.dist / sigma));
    const sumExp = exps.reduce((a, b) => a + b, 0) || 1;

    for (let j = 0; j < 4; j++) {
      skinIndices[i*4+j] = top4[j] ? top4[j].idx : 0;
      skinWeights[i*4+j] = top4[j] ? exps[j] / sumExp : 0;
    }
  }

  return { skinIndices, skinWeights };
}


// ============================================================
// BUILD COMPLETE KNIGHT
// ============================================================

/**
 * Build an SDF knight mesh skinned to the Mixamo skeleton.
 *
 * @param {Object} boneMap - { boneName: THREE.Bone } from rig.js
 * @param {Object} opts - { resolution: 80, steelColor: 0x9aa5b4, goldColor: 0xdaa520 }
 * @returns {THREE.SkinnedMesh}
 */
function buildSDFKnight(boneMap, opts = {}) {
  const resolution = opts.resolution || 80;
  console.time('SDF Knight');

  // Extract bone positions in MODEL SPACE (centimeters).
  // The bones' world positions are in meters (after Group scale 0.01).
  // The original mesh vertices are in cm, so we need bone positions in cm too.
  // We get world position (meters) and multiply by 100 to get cm.
  const wp = (name) => {
    const bone = boneMap[name];
    if (!bone) return [0, 0, 0];
    const v = new THREE.Vector3();
    bone.getWorldPosition(v);
    return [v.x * 100, v.y * 100, v.z * 100];
  };

  const bones = {
    hips: wp('mixamorigHips'),
    spine: wp('mixamorigSpine'),
    spine1: wp('mixamorigSpine1'),
    spine2: wp('mixamorigSpine2'),
    neck: wp('mixamorigNeck'),
    head: wp('mixamorigHead'),
    headTop: wp('mixamorigHeadTop_End'),
    lShoulder: wp('mixamorigLeftShoulder'),
    lArm: wp('mixamorigLeftArm'),
    lForeArm: wp('mixamorigLeftForeArm'),
    lHand: wp('mixamorigLeftHand'),
    rShoulder: wp('mixamorigRightShoulder'),
    rArm: wp('mixamorigRightArm'),
    rForeArm: wp('mixamorigRightForeArm'),
    rHand: wp('mixamorigRightHand'),
    lUpLeg: wp('mixamorigLeftUpLeg'),
    lLeg: wp('mixamorigLeftLeg'),
    lFoot: wp('mixamorigLeftFoot'),
    lToe: wp('mixamorigLeftToeBase'),
    rUpLeg: wp('mixamorigRightUpLeg'),
    rLeg: wp('mixamorigRightLeg'),
    rFoot: wp('mixamorigRightFoot'),
    rToe: wp('mixamorigRightToeBase'),
  };

  // 1. Define SDF
  const bodySDF = defineKnightSDF(bones);
  const goldSDF = defineGoldSDF(bones);

  // 2. Compute bounding box (in cm)
  const allPts = Object.values(bones);
  let bbMin = [Infinity, Infinity, Infinity];
  let bbMax = [-Infinity, -Infinity, -Infinity];
  for (const pt of allPts) {
    for (let i = 0; i < 3; i++) {
      bbMin[i] = Math.min(bbMin[i], pt[i]);
      bbMax[i] = Math.max(bbMax[i], pt[i]);
    }
  }
  // Expand by armor padding (in cm)
  const pad = 20;
  bbMin = bbMin.map(v => v - pad);
  bbMax = bbMax.map(v => v + pad);

  // 3. Marching cubes on body SDF
  console.log(`  Marching cubes: res=${resolution}, bounds=[${bbMin.map(v=>v.toFixed(2))}, ${bbMax.map(v=>v.toFixed(2))}]`);
  const bodyMesh = marchingCubes(bodySDF, bbMin, bbMax, resolution);
  console.log(`  Body: ${bodyMesh.vertexCount} vertices`);

  // 4. Marching cubes on gold trim SDF (lower res for thin features)
  const goldMesh = marchingCubes(goldSDF, bbMin, bbMax, Math.floor(resolution * 0.8));
  console.log(`  Gold: ${goldMesh.vertexCount} vertices`);

  // 5. Build bone segments for skinning.
  // CRITICAL: skinIndex values must be indices into skeleton.bones[],
  // NOT indices into boneMap keys. Build a lookup from bone name → skeleton index.
  const skeletonRef = opts._skeleton || null;
  const skeletonBoneIndex = {};
  if (skeletonRef) {
    skeletonRef.bones.forEach((bone, i) => {
      skeletonBoneIndex[bone.name] = i;
    });
  } else {
    // Fallback: use boneMap key order (will be wrong but at least won't crash)
    Object.keys(boneMap).forEach((name, i) => { skeletonBoneIndex[name] = i; });
  }

  const segments = [
    { a: bones.hips, b: bones.spine, bone: 'mixamorigHips' },
    { a: bones.spine, b: bones.spine1, bone: 'mixamorigSpine' },
    { a: bones.spine1, b: bones.spine2, bone: 'mixamorigSpine1' },
    { a: bones.spine2, b: bones.neck, bone: 'mixamorigSpine2' },
    { a: bones.neck, b: bones.head, bone: 'mixamorigNeck' },
    { a: bones.head, b: bones.headTop, bone: 'mixamorigHead' },
    { a: bones.lShoulder, b: bones.lArm, bone: 'mixamorigLeftShoulder' },
    { a: bones.lArm, b: bones.lForeArm, bone: 'mixamorigLeftArm' },
    { a: bones.lForeArm, b: bones.lHand, bone: 'mixamorigLeftForeArm' },
    { a: bones.rShoulder, b: bones.rArm, bone: 'mixamorigRightShoulder' },
    { a: bones.rArm, b: bones.rForeArm, bone: 'mixamorigRightArm' },
    { a: bones.rForeArm, b: bones.rHand, bone: 'mixamorigRightForeArm' },
    { a: bones.lUpLeg, b: bones.lLeg, bone: 'mixamorigLeftUpLeg' },
    { a: bones.lLeg, b: bones.lFoot, bone: 'mixamorigLeftLeg' },
    { a: bones.lFoot, b: bones.lToe, bone: 'mixamorigLeftFoot' },
    { a: bones.rUpLeg, b: bones.rLeg, bone: 'mixamorigRightUpLeg' },
    { a: bones.rLeg, b: bones.rFoot, bone: 'mixamorigRightLeg' },
    { a: bones.rFoot, b: bones.rToe, bone: 'mixamorigRightFoot' },
  ].map(s => ({ a: s.a, b: s.b, boneIndex: skeletonBoneIndex[s.bone] ?? 0 }));

  // 6. Compute skinning weights for body mesh
  console.log('  Computing skinning weights...');
  const { skinIndices, skinWeights } = computeSkinWeights(bodyMesh.positions, segments);

  // 7. Build Three.js SkinnedMesh for body
  const bodyGeo = new THREE.BufferGeometry();
  bodyGeo.setAttribute('position', new THREE.BufferAttribute(bodyMesh.positions, 3));
  bodyGeo.setAttribute('normal', new THREE.BufferAttribute(bodyMesh.normals, 3));
  bodyGeo.setAttribute('skinIndex', new THREE.BufferAttribute(new Uint16Array(skinIndices), 4));
  bodyGeo.setAttribute('skinWeight', new THREE.BufferAttribute(skinWeights, 4));

  const steelMat = new THREE.MeshStandardMaterial({
    color: opts.steelColor || 0x9aa5b4,
    metalness: 0.92,
    roughness: 0.15,
    envMap,
    envMapIntensity: 2.0,
    flatShading: false,
  });

  // 8. Build gold trim mesh (also skinned)
  const goldGeo = new THREE.BufferGeometry();
  goldGeo.setAttribute('position', new THREE.BufferAttribute(goldMesh.positions, 3));
  goldGeo.setAttribute('normal', new THREE.BufferAttribute(goldMesh.normals, 3));
  const goldSkin = computeSkinWeights(goldMesh.positions, segments);
  goldGeo.setAttribute('skinIndex', new THREE.BufferAttribute(new Uint16Array(goldSkin.skinIndices), 4));
  goldGeo.setAttribute('skinWeight', new THREE.BufferAttribute(goldSkin.skinWeights, 4));

  const goldMat = new THREE.MeshStandardMaterial({
    color: opts.goldColor || 0xdaa520,
    metalness: 0.95,
    roughness: 0.1,
    emissive: 0x332200,
    emissiveIntensity: 0.2,
    envMap,
    envMapIntensity: 2.0,
  });

  // 9. Create SkinnedMesh instances using the skeleton captured earlier
  const skeleton = skeletonRef;

  if (!skeleton) {
    console.warn('SDF Knight: No skeleton found, mesh will be static');
    const bodyMeshObj = new THREE.Mesh(bodyGeo, steelMat);
    const goldMeshObj = new THREE.Mesh(goldGeo, goldMat);
    scene.add(bodyMeshObj);
    scene.add(goldMeshObj);
    console.timeEnd('SDF Knight');
    return bodyMeshObj;
  }

  // Mesh vertices are already in cm (model space) — no scaling needed.

  const bodySkinnedMesh = new THREE.SkinnedMesh(bodyGeo, steelMat);
  bodySkinnedMesh.name = 'SDFKnight_Body';
  bodySkinnedMesh.bind(skeleton);
  bodySkinnedMesh.castShadow = true;
  bodySkinnedMesh.frustumCulled = false;

  const goldSkinnedMesh = new THREE.SkinnedMesh(goldGeo, goldMat);
  goldSkinnedMesh.name = 'SDFKnight_Gold';
  goldSkinnedMesh.bind(skeleton);
  goldSkinnedMesh.castShadow = true;
  goldSkinnedMesh.frustumCulled = false;

  // Add to the model root (Group with scale 0.01)
  let modelRoot = Object.values(boneMap)[0];
  while (modelRoot.parent && modelRoot.parent.type !== 'Scene') modelRoot = modelRoot.parent;
  modelRoot.add(bodySkinnedMesh);
  modelRoot.add(goldSkinnedMesh);

  // Hide bone helper spheres
  scene.traverse(child => {
    if (child.name === 'boneHelperGroup' || (child.isGroup && child.children.some(c => c.userData?.boneName))) {
      child.visible = false;
    }
  });

  console.timeEnd('SDF Knight');
  console.log(`  SDF Knight complete: ${bodyMesh.vertexCount + goldMesh.vertexCount} total vertices`);

  return bodySkinnedMesh;
}

/**
 * Replace the original FBX meshes with the SDF knight.
 */
function replaceWithSDFKnight(boneMap, opts) {
  const firstBone = Object.values(boneMap)[0];
  if (!firstBone) return null;
  let root = firstBone;
  while (root.parent && root.parent.type !== 'Scene') root = root.parent;

  // IMPORTANT: Capture skeleton BEFORE removing original skinned meshes
  let skeleton = null;
  root.traverse(child => {
    if (child.isSkinnedMesh && child.skeleton && !skeleton) {
      skeleton = child.skeleton;
    }
  });

  // Remove original meshes
  const toRemove = [];
  root.traverse(child => {
    if (child.isMesh && ['Body', 'Head_Hands', 'Lower_Armor'].includes(child.name)) {
      toRemove.push(child);
    }
  });
  for (const mesh of toRemove) {
    mesh.geometry.dispose();
    if (mesh.material) {
      (Array.isArray(mesh.material) ? mesh.material : [mesh.material]).forEach(m => m.dispose());
    }
    mesh.parent.remove(mesh);
  }

  return buildSDFKnight(boneMap, { ...opts, _skeleton: skeleton });
}

export { buildSDFKnight, replaceWithSDFKnight, defineKnightSDF, defineGoldSDF, marchingCubes };
