# Aranea Membri model

The current runtime creature comes from the user-supplied `rec-room-backrooms-entity-62-aranea-membri.zip`. The original GLB and PNG files are preserved byte for byte. No author or license document accompanied the supplied archive, so no authorship, license or affiliation is inferred from its name.

| Supplied file | Bytes | SHA-256 |
| --- | ---: | --- |
| `source/Scene.glb` | 4,665,152 | `62524b8b492de1b222c3c57700ef55eda6ed80e1e118b2e0d85c12aad2214d17` |
| `textures/UGC_Carpet_Berber_tex_1.png` | 1,913,282 | `653d0a38d5f3dee8ce63e1ce4f8e4f62267e7ea601874a26fb0c57d36550090b` |
| `textures/Carpet_LowPile_norm_0.png` | 1,483,374 | `c8bf8453e960fcaa42f7a78acc23c20d722d30ec3cb39430ddfd06d41f43d550` |

All **135 mesh parts and 15,220 triangles** are retained. Splitting vertices where source UVs or normals differ gives **11,657 runtime vertices**. No geometry decimation is applied. The source has no rig or animation; the project creates the joint mapping and motion. Runtime coordinates are metres, Y-up, facing -Z.

## Runtime files

- `aranea.brm`: version-3 geometry and animation cache, 31,171,529 bytes (about 29.73 MiB).
- `creature-color.rgb` and `creature-normal.rgb`: 512×512 RGB texture caches, 786,448 bytes each.
- `aranea-manifest.json`: source/cache hashes, geometry counts, clip durations and baked bounds.
- `source/` and `textures/`: unchanged supplied originals for provenance and rebaking.

Install the executable, animation bank and both RGB caches together. The old Smiler bank is incompatible and is not a fallback. The model is loaded before gameplay; no Blender, glTF parser, Python or image library is required at runtime.

The source's `Mist_Carpet`, `White_Carpet` and `Black_Carpet` map to game material IDs 20, 21 and 22. All three use the original color and normal maps and their original linear base-color factors. Source normal strength is 0.635. The color map is decoded from sRGB for lighting; normal data remains linear. Default glTF OPAQUE alpha mode is preserved. No constant Smiler eye/teeth emission is carried over; a restrained face fill is used during capture.

## Animation

`tools/aranea-rig.json` maps the original mesh parts to four hip/knee/tip chains plus head and jaw pivots. `tools/bake_aranea.py` solves the two-segment legs analytically and transforms those source parts without changing their triangles or UVs. The source assembly's body and skull point along different diagonal axes. Baking corrects the body from the front/rear hip centers, then aligns the skull independently from its paired orbital rims. The resulting eyes are level and the face and walking direction both point along local −Z. `restAlignment` in the manifest records the source landmarks and corrections; the original GLB remains unchanged.

The foot order is front-left, front-right, rear-left, rear-right. Walking uses staggered phases `[0, 0.5, 0.75, 0.25]`, a **78% support phase** and **1.30 m full-cycle stride**. Three feet support the body while the fourth lifts and advances, with brief four-foot overlap. Running uses diagonal phases `[0, 0.5, 0.5, 0]`, **58% support** and a **2.00 m stride**. Support tips move backward relative to the body to cancel forward root travel. Smooth leg lift, body weight transfer and the separate faster gait make individual steps visible. Four contact channels drive the existing heavy footstep recordings.

There are seven clips and 220 frames:

| Clip | Frames | Nominal duration |
| --- | ---: | ---: |
| idle | 16 | 3.20 s |
| walk | 40 | 1.20 s |
| run | 40 | 0.65 s |
| idle_duck | 16 | 3.20 s |
| walk_duck | 40 | 1.20 s |
| run_duck | 40 | 0.65 s |
| scare | 28 | 2.00 s |

Locomotion uses actual distance traveled and a continuous phase, blending walk/run by speed. Navigation turns toward a route segment before moving along it; rendering uses the actual movement direction to keep the body facing forward. Folded clips draw the legs inward for doors: the corrected baked full-fold envelope is approximately **1.15 m wide and 2.15 m high**, while normal locomotion reaches approximately **2.37 m**. Consult the manifest for exact per-clip bounds. The capture has a separate cinematic envelope and a stabilized **1.75 m** eye anchor.

The head can turn independently around its neck pivot, up to **±0.8 rad** (about ±46°) in yaw and **−0.25/+0.20 rad** in pitch. While moving it stays centered in yaw, with subtle nods and vertical tracking. At rest it follows a visible player or scans gently. The jaw has restrained idle movement and opens more strongly during capture. The authored capture thrusts the body and front claws forward with head jitter; no source animation was present. Pause freezes animation and contacts; reset starts a new gait state.

The runtime capture presentation aims the camera at the animated head over **0.20 seconds**, then brings the eye anchor to **0.90 m** with an anchored scale increase, decaying random camera shake/roll and a **70°→95°** FOV punch. Its default **2.0-second** duration and effect values are configurable in `jumpscare.cfg` or a file selected by `--jumpscare-config`. See [jumpscare implementation](../../../docs/jumpscare.md). Changing runtime timing does not require rebaking the normalized capture clip.

## Offline rebuild

Blender 4.3 and Python with Pillow are development tools only. From the project root:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 4.3\blender.exe' --background --disable-autoexec --python tools/bake_aranea.py
python tools/prepare_aranea_textures.py
```

The baker resets its Blender scene and imports `Scene.glb` with automatic script execution disabled, then writes the bank and manifest. The texture converter reads the supplied PNGs, resizes to 512 square and flips rows for the Blender/OpenGL UV convention. It renormalizes the downsampled normal vectors and leaves the original files untouched.

## Cache format and validation

`BRARAN3\0` version 3 uses little-endian fields: a bounded geometry/clip header, seven clip descriptors, indexed triangles, five per-vertex byte masks (head plus four feet) and two float UV coordinates. Each frame stores an int16 head pivot, four contact bits and a reserved byte, followed by quantized position/normal data. Positions use 1/8192 m units; normals use 1/32767 units.

The model loader validates byte/count budgets, version, materials, indices, head/foot masks, finite UVs, head pivots, contact flags, reserved bytes and normals. The texture format is `ARATEX1\0`, two little-endian uint32 dimensions and RGB bytes. Its loader requires exactly 512×512 pixels and an exact byte count; color and normal caches are validated before texture allocation.

`build.ps1 -Test` and `tools/verify-aranea.ps1` cover model behavior and game rendering. See [Aranea update notes](../../../docs/aranea-motion.md) for validation scope and recorded results. Earlier Smiler results apply only to that historical model.
