/**
 * weapon-system.js — Procedural sword + shield, bone attachment
 */
import { THREE } from './core.js';
import { steelMat, goldMat, darkSteelMat } from './materials.js';
import { getBone } from './rig.js';

let sword = null;
let shield = null;

function createSword() {
  const group = new THREE.Group();
  group.name = 'Sword';

  // Blade — tapered
  const bladeGeo = new THREE.BoxGeometry(3.5, 65, 0.4); // in cm (model space)
  const pos = bladeGeo.attributes.position;
  for (let i = 0; i < pos.count; i++) {
    const y = pos.getY(i);
    if (y > 0) {
      const taper = 1 - (y / 32.5) * 0.6;
      pos.setX(i, pos.getX(i) * taper);
      pos.setZ(i, pos.getZ(i) * taper);
    }
  }
  pos.needsUpdate = true;
  bladeGeo.computeVertexNormals();
  const blade = new THREE.Mesh(bladeGeo, steelMat.clone());
  blade.position.y = 42;
  group.add(blade);

  // Fuller (groove in blade center)
  const fullerGeo = new THREE.BoxGeometry(1.2, 45, 0.6);
  const fuller = new THREE.Mesh(fullerGeo, darkSteelMat.clone());
  fuller.position.y = 38;
  group.add(fuller);

  // Crossguard
  const guardGeo = new THREE.BoxGeometry(14, 1.8, 1.8);
  const guard = new THREE.Mesh(guardGeo, goldMat.clone());
  guard.position.y = 9.5;
  group.add(guard);

  // Grip — leather wrapped
  const gripGeo = new THREE.CylinderGeometry(1.1, 1.3, 10, 8);
  const gripMat = new THREE.MeshStandardMaterial({ color: 0x3a2a1a, roughness: 0.8, metalness: 0.1 });
  const grip = new THREE.Mesh(gripGeo, gripMat);
  grip.position.y = 4;
  group.add(grip);

  // Pommel
  const pommelGeo = new THREE.SphereGeometry(1.6, 8, 8);
  const pommel = new THREE.Mesh(pommelGeo, goldMat.clone());
  pommel.position.y = -1.5;
  group.add(pommel);

  // Pommel gem
  const gemGeo = new THREE.SphereGeometry(0.6, 8, 8);
  const gemMat = new THREE.MeshStandardMaterial({ color: 0x2060ff, emissive: 0x1040cc, emissiveIntensity: 0.5 });
  const gem = new THREE.Mesh(gemGeo, gemMat);
  gem.position.y = -1.5;
  gem.position.z = 1.6;
  group.add(gem);

  return group;
}

function createDragonRampantTexture() {
  // Paint a red dragon rampant (heraldic) on a canvas
  const canvas = document.createElement('canvas');
  canvas.width = 256;
  canvas.height = 512;
  const ctx = canvas.getContext('2d');

  // Shield background — dark steel blue
  ctx.fillStyle = '#1a1a3a';
  ctx.fillRect(0, 0, 256, 512);

  // Gold border band
  ctx.strokeStyle = '#daa520';
  ctx.lineWidth = 8;
  ctx.strokeRect(12, 12, 232, 488);

  // Red dragon rampant — simplified heraldic silhouette
  ctx.save();
  ctx.translate(128, 240);
  ctx.scale(0.85, 0.85);

  ctx.fillStyle = '#cc2222';
  ctx.strokeStyle = '#881111';
  ctx.lineWidth = 2;

  ctx.beginPath();
  // Body — standing on left hind leg
  ctx.moveTo(-10, 80);      // left foot
  ctx.lineTo(-15, 40);      // left shin
  ctx.lineTo(-20, 10);      // left thigh
  ctx.lineTo(-25, -20);     // hip
  ctx.lineTo(-20, -60);     // belly
  ctx.lineTo(-15, -90);     // chest
  ctx.lineTo(-10, -110);    // neck base

  // Head — facing right, jaws open
  ctx.lineTo(5, -130);      // throat
  ctx.lineTo(15, -145);     // lower jaw tip
  ctx.lineTo(25, -140);     // jaw corner
  ctx.lineTo(20, -150);     // snout
  ctx.lineTo(30, -155);     // upper jaw/horn
  ctx.lineTo(15, -155);     // top of head
  ctx.lineTo(5, -148);      // crown/frill
  ctx.lineTo(-5, -140);     // back of head
  ctx.lineTo(-15, -130);    // nape

  // Wing — large, spread upward-right
  ctx.lineTo(-20, -120);    // wing root
  ctx.lineTo(10, -170);     // wing tip top
  ctx.lineTo(40, -150);     // wing upper edge
  ctx.lineTo(65, -120);     // wing mid
  ctx.lineTo(80, -80);      // wing outer point
  ctx.lineTo(55, -70);      // wing scallop 1
  ctx.lineTo(70, -50);      // wing point 2
  ctx.lineTo(45, -45);      // wing scallop 2
  ctx.lineTo(55, -25);      // wing point 3
  ctx.lineTo(30, -25);      // wing scallop 3
  ctx.lineTo(35, -5);       // wing bottom point
  ctx.lineTo(10, -15);      // wing membrane bottom
  ctx.lineTo(-10, -30);     // wing back to body

  // Back and tail
  ctx.lineTo(-15, 0);       // mid back
  ctx.lineTo(-10, 30);      // lower back
  ctx.lineTo(-5, 60);       // tail root
  ctx.lineTo(10, 90);       // tail mid
  ctx.lineTo(25, 110);      // tail curve
  ctx.lineTo(40, 120);      // tail tip
  ctx.lineTo(35, 125);      // tail spade top
  ctx.lineTo(45, 130);      // tail spade point
  ctx.lineTo(30, 130);      // tail spade bottom
  ctx.lineTo(20, 120);      // tail return
  ctx.lineTo(5, 100);       // tail inner
  ctx.lineTo(-5, 70);       // back to body

  // Right leg (raised, clawing)
  ctx.lineTo(5, -10);       // right hip
  ctx.lineTo(20, -20);      // right thigh
  ctx.lineTo(35, -5);       // right knee
  ctx.lineTo(25, 10);       // right shin
  ctx.lineTo(30, 20);       // right foot
  ctx.lineTo(20, 15);       // right claw
  ctx.lineTo(10, 5);        // back

  // Right arm (raised forward, clawing)
  ctx.moveTo(-5, -85);      // shoulder
  ctx.lineTo(15, -105);     // upper arm
  ctx.lineTo(30, -110);     // elbow
  ctx.lineTo(40, -100);     // forearm
  ctx.lineTo(50, -105);     // claw tip 1
  ctx.lineTo(42, -95);      // claw return
  ctx.lineTo(48, -92);      // claw tip 2
  ctx.lineTo(38, -88);      // claw return
  ctx.lineTo(25, -90);      // wrist
  ctx.lineTo(10, -80);      // back to body

  ctx.closePath();
  ctx.fill();
  ctx.stroke();

  // Eye
  ctx.fillStyle = '#ffcc00';
  ctx.beginPath();
  ctx.arc(18, -148, 3, 0, Math.PI * 2);
  ctx.fill();

  ctx.restore();

  const texture = new THREE.CanvasTexture(canvas);
  texture.flipY = false;
  return texture;
}

function createShield() {
  const group = new THREE.Group();
  group.name = 'KiteShield';

  // Kite shield body with dragon rampant texture
  const shape = new THREE.Shape();
  shape.moveTo(0, 20);
  shape.quadraticCurveTo(14, 16, 14, 5);
  shape.quadraticCurveTo(14, -5, 0, -22);
  shape.quadraticCurveTo(-14, -5, -14, 5);
  shape.quadraticCurveTo(-14, 16, 0, 20);

  const extrudeSettings = { depth: 1.5, bevelEnabled: true, bevelThickness: 0.3, bevelSize: 0.3, bevelSegments: 2 };
  const shieldGeo = new THREE.ExtrudeGeometry(shape, extrudeSettings);

  // Apply dragon texture to shield face
  const dragonTex = createDragonRampantTexture();
  const shieldMat = new THREE.MeshStandardMaterial({
    map: dragonTex,
    metalness: 0.3,
    roughness: 0.5,
  });
  // Steel for the sides/back
  const shieldMaterials = [shieldMat, steelMat.clone()];
  // ExtrudeGeometry has 2 material groups: face (0) and sides (1)
  shieldGeo.groups.forEach(g => {
    // Front and back faces get the dragon texture, sides get steel
    if (g.materialIndex === 0) g.materialIndex = 0;
    else g.materialIndex = 1;
  });
  const shieldMesh = new THREE.Mesh(shieldGeo, shieldMaterials);
  group.add(shieldMesh);

  // Gold rim
  const rimShape = new THREE.Shape();
  rimShape.moveTo(0, 21);
  rimShape.quadraticCurveTo(15, 17, 15, 5);
  rimShape.quadraticCurveTo(15, -5, 0, -23);
  rimShape.quadraticCurveTo(-15, -5, -15, 5);
  rimShape.quadraticCurveTo(-15, 17, 0, 21);
  const rimHole = new THREE.Path();
  rimHole.moveTo(0, 19);
  rimHole.quadraticCurveTo(13, 15, 13, 5);
  rimHole.quadraticCurveTo(13, -4, 0, -21);
  rimHole.quadraticCurveTo(-13, -4, -13, 5);
  rimHole.quadraticCurveTo(-13, 15, 0, 19);
  rimShape.holes.push(rimHole);
  const rimGeo = new THREE.ExtrudeGeometry(rimShape, { depth: 2, bevelEnabled: false });
  const rim = new THREE.Mesh(rimGeo, goldMat.clone());
  rim.position.z = -0.2;
  group.add(rim);

  // Central boss
  const bossGeo = new THREE.SphereGeometry(3, 12, 12);
  const boss = new THREE.Mesh(bossGeo, goldMat.clone());
  boss.position.set(0, 2, 2);
  group.add(boss);

  return group;
}

// Default offsets (tunable via API)
let swordOffset = { x: 2, y: 0, z: 0, rx: -90, ry: 0, rz: 0 };
let shieldOffset = { x: 0, y: 10, z: 8, rx: 0, ry: 0, rz: 0 };

function attachWeapons() {
  const rightHand = getBone('mixamorigRightHand');
  const leftForeArm = getBone('mixamorigLeftForeArm');

  if (rightHand && !sword) {
    sword = createSword();
    sword.position.set(swordOffset.x, swordOffset.y, swordOffset.z);
    sword.rotation.set(
      THREE.MathUtils.degToRad(swordOffset.rx),
      THREE.MathUtils.degToRad(swordOffset.ry),
      THREE.MathUtils.degToRad(swordOffset.rz)
    );
    rightHand.add(sword);
  }

  if (leftForeArm && !shield) {
    shield = createShield();
    shield.position.set(shieldOffset.x, shieldOffset.y, shieldOffset.z);
    shield.rotation.set(
      THREE.MathUtils.degToRad(shieldOffset.rx),
      THREE.MathUtils.degToRad(shieldOffset.ry),
      THREE.MathUtils.degToRad(shieldOffset.rz)
    );
    shield.scale.setScalar(1.2);
    leftForeArm.add(shield);
  }

  return { sword: !!sword, shield: !!shield };
}

function detachWeapons() {
  if (sword && sword.parent) { sword.parent.remove(sword); sword = null; }
  if (shield && shield.parent) { shield.parent.remove(shield); shield = null; }
}

function setSwordOffset(x, y, z, rx, ry, rz) {
  swordOffset = { x, y, z, rx, ry, rz };
  if (sword) {
    sword.position.set(x, y, z);
    sword.rotation.set(THREE.MathUtils.degToRad(rx), THREE.MathUtils.degToRad(ry), THREE.MathUtils.degToRad(rz));
  }
}

function setShieldOffset(x, y, z, rx, ry, rz) {
  shieldOffset = { x, y, z, rx, ry, rz };
  if (shield) {
    shield.position.set(x, y, z);
    shield.rotation.set(THREE.MathUtils.degToRad(rx), THREE.MathUtils.degToRad(ry), THREE.MathUtils.degToRad(rz));
  }
}

function setSwordVisible(v) { if (sword) sword.visible = v; }
function setShieldVisible(v) { if (shield) shield.visible = v; }

export {
  attachWeapons, detachWeapons,
  setSwordOffset, setShieldOffset,
  setSwordVisible, setShieldVisible,
};
