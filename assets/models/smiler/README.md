# Smiler model

This is the current runtime creature. The user supplied `smiler-backrooms.zip`, which contained only `source/Zombie Scream.fbx` (872,240 bytes). The original FBX is preserved unchanged here. Its author and license were not included in the archive, so no authorship or license is inferred.

- `smiler.brm`: game-ready geometry and animation cache; required beside the executable in `assets/models/smiler`.
- `source/Zombie Scream.fbx`: supplied mesh, skin weights, 65-bone Mixamo-named armature and original 85-frame action.
- `smiler-manifest.json`: source SHA-256, geometry counts, animation durations and measured bounds.
- `preview.png`: rendered view of the imported creature in its idle pose.

The original 4,466 vertices and 8,728 triangles are retained, including the long claws, volumetric white eyes and grinning teeth. The two supplied materials map to game IDs 20 (black body) and 21 (white eyes/teeth). There are no embedded or external image textures in this archive. Triangulation is fixed before evaluating poses so all animation frames retain identical topology. The mesh is oriented Y-up, facing -Z, and scaled to metres; gameplay poses stay below 2.84 m.

## Animation

`tools/bake_smiler.py` authors idle breathing, alternating walking, and a longer/faster running stride on the supplied rig, plus separately bent-knee and bent-spine versions for low doorways. The full crouch reaches at most 2.234 m. These are articulated limb animations, not a translation of the whole model. Original skin weights deform the mesh when baking.

The capture clip samples frames 1–56 of the supplied `Armature|mixamo.com|Layer0` action into a 1.05-second scream. Its eye geometry is stabilized around local `(0, 2.60, 0)` only for the cinematic closeup, which keeps the face in view as the original action crouches and opens its arms.

Seven baked clips use 148 frames in total: 12 idle, 24 walk, 24 run, their three crouched counterparts, and 28 capture frames. The game interpolates adjacent frames, blends locomotion by actual movement speed, and blends crouching separately. A continuous distance-based stride phase prevents resets when changing behavior or pausing. Normals are interpolated and renormalized. The one-time validated cache is about 7.70 MiB on disk and in memory; rendering expands its indexed triangles into the existing vertex format.

## Rebuild

Blender 4.3 is an **offline development tool only**. Players and contributors compiling the game do not need Blender or an FBX library. Run from the project root:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 4.3\blender.exe' --background --disable-autoexec --python tools/bake_smiler.py
```

This rebuilds the cache and manifest from the original FBX. The script resets Blender's scene and imports the file with automatic script execution disabled. Source inspection helpers are `tools/inspect_smiler.py` and `tools/preview_smiler_source.py`.

## Cache format and validation

`BRSMIL1` version 1 is little-endian, beginning with an eight-byte magic, version/vertex/triangle/clip counts and position/normal quantization units. Seven 28-byte clip descriptors contain padded names, frame counts, durations and loop flags. Each triangle stores three uint32 indices, a material byte and three reserved zero bytes. Frame vertices contain six signed int16 values: position in 1/8192 m units and normal in 1/32767 units.

The loader rejects missing, truncated, oversized or unsupported files, invalid counts or clips, bad indices/materials/normals, nonzero reserved bytes, and mismatched/trailing data before rendering. The executable preloads the cache so the first appearance causes no disk-loading hitch. `tests/threat_model_tests.cpp` checks retained geometry, distinct articulated gaits, smooth loops/transitions, pause determinism, doorway clearance, animation bounds and malformed-file rejection.
