# Shattered Arcana — Knight Denizen Animation Project History

## Timeline (2026-04-06)

### Attempt 1: Hunyuan3D GLB + Blender envelope weights
- **Approach**: Used AI-generated 3D model from Hunyuan3D 2.1, tried Blender auto-weight painting
- **Result**: FAILED — 638K vertex single fused mesh, no materials/UVs/vertex colors
- **Why it failed**: Bone Heat Weighting failed on dense mesh. Mesh is a solid sculpture — sword/arm/body all fused. No separate parts to animate independently.
- **Lesson**: AI-generated 3D models from Hunyuan3D are geometry-only sculptures, NOT game-ready assets

### Attempt 2: UniRig auto-rigging
- **Approach**: Used VAST-AI/UniRig (SIGGRAPH 2025) to auto-rig the Hunyuan3D mesh
- **Setup**: Python 3.11 venv at /tmp/UniRig, all CUDA deps (flash_attn, spconv-cu121, torch_scatter)
- **Result**: PARTIAL SUCCESS — generated 35-bone skeleton with proper skinning weights
- **Why it fell short**: Skeleton bone names are generic (bone_0 through bone_34). Skinning works but the fused mesh still distorts badly when animated. Sword doesn't move separately because it's part of the mesh.
- **Lesson**: UniRig works great for rigging, but can't fix a fundamentally un-animatable mesh topology

### Attempt 3: Projected portrait texture on Hunyuan3D mesh
- **Approach**: Project the 2D denizen portrait onto the 3D mesh using Generated texture coordinates
- **Result**: PARTIAL — gave the mesh color (gold/black) but looks like paint splatter, not real armor
- **Lesson**: Without UV unwrapping, texture projection is always approximate

### Attempt 4: Building knight from raw bmesh vertex coordinates
- **Approach**: Manually specified vertex positions for body parts (cylinders, spheres)
- **Result**: FAILED — disconnected blobs, gaps at joints, hollow rings from remove_doubles
- **Why it failed**: No human can build a humanoid by typing vertex coordinates. Wrong tool for the job.
- **Lesson**: Use Blender's higher-level tools (primitives, modifiers), not raw vertex lists

### Attempt 5: Separate mesh objects per body part with BONE parenting
- **Approach**: Created separate cylinders for each body part, parented each to a bone
- **Result**: FAILED — gaps between body parts, wrong parent offsets, parts flying off during animation
- **Why it failed**: parent_type='BONE' positions child relative to bone TAIL not HEAD. Used wrong offset calculation. Used ARMATURE_AUTO for rigid objects (should use BONE).
- **Lesson**: BONE parenting puts origin at tail, need head-tail offset. Rigid objects MUST use BONE parent, NOT ARMATURE_AUTO.

### Attempt 6: Skin modifier + Rigify metarig (CURRENT APPROACH)
- **Approach**: 
  1. Rigify basic_human_metarig for 29-bone armature
  2. Skin modifier on stick figure for watertight body mesh (~3100 verts)
  3. ARMATURE_AUTO for body deformation
  4. BONE parenting for sword, shield, helmet, pauldrons, gauntlets, greaves
- **Result**: WORKING — connected body, sword moves independently, armor pieces follow bones
- **Remaining issues**: 
  - Armor is cylindrical blobs, not detailed plate armor
  - Chest plate offset wrong (hidden inside body)
  - Shield shows as oval not kite shape
  - Gold gauntlets are ball-shaped not forearm-wrapping
  - Body is still a smooth mannequin

### Key Technical Discoveries

1. **bpy parent_type='BONE'**: Object origin at bone TAIL, not HEAD. Offset = bone.head_local - bone.tail_local
2. **ARMATURE_AUTO vs BONE**: AUTO creates vertex groups + modifier for DEFORMABLE meshes. BONE makes rigid following for accessories (sword/shield/helmet)
3. **Skin modifier**: Creates watertight manifold mesh from stick figure with per-vertex radii. branch_smoothing controls junction quality
4. **Blender 4.0 crease**: Moved to generic attributes (bm.edges.layers.float.get('crease_edge')), old API removed
5. **Animation smoothness**: AUTO_CLAMPED handle types prevent overshoot. More keyframes with smaller increments = organic motion
6. **matrix_parent_inverse**: Computed automatically by bpy.ops.object.parent_set operator but NOT by direct property assignment

### Files on Dev Server (176.9.99.103)

- /tmp/knight_v6d.blend — base working scene (body + armor shell + sword + shield)
- /tmp/knight_step1b.blend — + helmet
- /tmp/knight_step2.blend — + gauntlets  
- /tmp/knight_step3b.blend — + greaves + chest plate + pauldrons
- /tmp/knight_final_v2.blend — full scene with smooth 144-frame animation
- /tmp/UniRig/ — UniRig installation with Python 3.11 venv
- /tmp/knight_pro.py — Skin modifier + Rigify metarig approach (working)
- /tmp/knight_all.py — Combined armor + animation script

### Files on Prod Server (78.47.104.139)

- /opt/ai-elevate/mythforge/website/mythforge-site/public/art/denizen-anims/aurelith-denizen-anim.webm — deployed animation
- /opt/ai-elevate/mythforge/website/mythforge-site/public/art/cinematics/aurelith-cinematic.webm — forward-only cinematic

### Tools Installed

- Blender 4.0.2 (system) on dev server
- bpy 4.2 pip package (Python 3.11 venv)
- UniRig (VAST-AI/UniRig) at /tmp/UniRig with Python 3.11 venv
- SVD-XT (Stable Video Diffusion) for cinematics
- Hunyuan3D 2.1 for image-to-3D (geometry only, no textures)

### What Would Fix the Remaining Quality Issues

1. Use a pre-rigged knight model from Mixamo/Sketchfab as the base mesh
2. Or use MakeHuman (open source) for a proper humanoid with topology/UVs
3. Or commission a game-ready model ($200-500) with exact specs
4. Use Cycles renderer instead of EEVEE for better metallic reflections
5. Use motion capture data (FreeMoCap or Mixamo animations) instead of manual keyframes
