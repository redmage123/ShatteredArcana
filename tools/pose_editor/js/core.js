/**
 * core.js — Scene, renderer, camera, lighting, render loop
 */
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const canvas = document.getElementById('viewport');

const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  alpha: true,
  preserveDrawingBuffer: true, // Required for screenshot capture in headless
});
renderer.setPixelRatio(window.devicePixelRatio);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.2;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x0a0a1a);

const camera = new THREE.PerspectiveCamera(35, canvas.clientWidth / canvas.clientHeight, 0.1, 100);
camera.position.set(0, 1.2, 3.5);
camera.lookAt(0, 1.0, 0);

const controls = new OrbitControls(camera, canvas);
controls.target.set(0, 1.0, 0);
controls.enableDamping = true;
controls.dampingFactor = 0.05;

// Lighting — bright enough to pass confidence gate on all angles
const ambientLight = new THREE.AmbientLight(0x4a4a6a, 0.8);
scene.add(ambientLight);

const keyLight = new THREE.DirectionalLight(0xfff0d0, 2.5);
keyLight.position.set(3, 5, 4);
keyLight.castShadow = true;
keyLight.shadow.mapSize.set(2048, 2048);
scene.add(keyLight);

// Extra back/top light to illuminate armor from behind
const backLight = new THREE.DirectionalLight(0xffe8c0, 1.2);
backLight.position.set(-2, 4, -3);
scene.add(backLight);

const fillLight = new THREE.DirectionalLight(0x4060a0, 0.6);
fillLight.position.set(-3, 2, -2);
scene.add(fillLight);

const rimLight = new THREE.DirectionalLight(0xffc040, 1.0);
rimLight.position.set(-1, 3, -4);
scene.add(rimLight);

// Ground
const ground = new THREE.Mesh(
  new THREE.PlaneGeometry(10, 10),
  new THREE.MeshStandardMaterial({ color: 0x1a1a2e, roughness: 0.8 })
);
ground.rotation.x = -Math.PI / 2;
ground.receiveShadow = true;
scene.add(ground);

// Environment map for metallic reflections
const pmremGen = new THREE.PMREMGenerator(renderer);
const envScene = new THREE.Scene();
envScene.background = new THREE.Color(0x1a1a2e);
[
  [0xfff0d0, 10, [3, 5, 4]],
  [0x4060a0, 5, [-3, 2, -2]],
  [0xffc040, 8, [-1, 3, -4]],
].forEach(([color, intensity, pos]) => {
  const l = new THREE.PointLight(color, intensity, 20);
  l.position.set(...pos);
  envScene.add(l);
});
const envMap = pmremGen.fromScene(envScene).texture;

// Bone helper group (green spheres on joints)
const boneHelperGroup = new THREE.Group();
scene.add(boneHelperGroup);

// Updatables — systems register here to get called each frame
const updatables = [];
function registerUpdatable(fn) { updatables.push(fn); }

// Clock for delta time
const clock = new THREE.Clock();

// Resize
function resize() {
  const w = window.innerWidth, h = window.innerHeight - 120;
  renderer.setSize(w, h);
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
}
window.addEventListener('resize', resize);
resize();

// Render loop
function animate() {
  requestAnimationFrame(animate);
  const dt = clock.getDelta();
  updatables.forEach(fn => fn(dt));
  controls.update();
  renderer.render(scene, camera);
}

// Synchronous single render (for frame capture)
function forceRender() {
  renderer.render(scene, camera);
}

// Set render size for export
function setRenderSize(w, h) {
  renderer.setSize(w, h);
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
}

export {
  THREE,
  canvas,
  renderer,
  scene,
  camera,
  controls,
  envMap,
  boneHelperGroup,
  clock,
  registerUpdatable,
  resize,
  animate,
  forceRender,
  setRenderSize,
};
