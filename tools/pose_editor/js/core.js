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
renderer.toneMappingExposure = 1.6;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x0a0a1a);

const camera = new THREE.PerspectiveCamera(35, canvas.clientWidth / canvas.clientHeight, 0.1, 100);
camera.position.set(0, 1.2, 3.5);
camera.lookAt(0, 1.0, 0);

const controls = new OrbitControls(camera, canvas);
controls.target.set(0, 1.0, 0);
controls.enableDamping = true;
controls.dampingFactor = 0.05;

// Lighting — tuned for bare-metal SDF knight
const ambientLight = new THREE.AmbientLight(0x8090b0, 1.5);
scene.add(ambientLight);

const keyLight = new THREE.DirectionalLight(0xfff0d0, 4.0);
keyLight.position.set(3, 5, 4);
keyLight.castShadow = true;
keyLight.shadow.mapSize.set(2048, 2048);
scene.add(keyLight);

// Strong back light for rim highlights on metallic surfaces
const backLight = new THREE.DirectionalLight(0xffe8c0, 2.5);
backLight.position.set(-2, 4, -3);
scene.add(backLight);

// Cool fill to show form in shadows
const fillLight = new THREE.DirectionalLight(0x6080c0, 1.2);
fillLight.position.set(-3, 2, 2);
scene.add(fillLight);

// Warm rim light — golden edge on Aurelith armor
const rimLight = new THREE.DirectionalLight(0xffc040, 2.0);
rimLight.position.set(-1, 3, -4);
scene.add(rimLight);

// Low front fill to brighten the chest/face
const frontFill = new THREE.DirectionalLight(0xdde0f0, 1.5);
frontFill.position.set(0, 2, 5);
scene.add(frontFill);

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
envScene.background = new THREE.Color(0x303848);
[
  [0xfff0d0, 20, [3, 5, 4]],
  [0x6080c0, 12, [-3, 2, 2]],
  [0xffc040, 15, [-1, 3, -4]],
  [0xdde0f0, 10, [0, 2, 5]],
  [0xffe8c0, 12, [-2, 4, -3]],
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
