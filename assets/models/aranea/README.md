# Threshold Sentinel

The current creature is an original procedural model generated for this project
with Codex assistance. Its faceted ceramic sensor mask, segmented shell, dorsal
crest and four mechanical limbs are built from mathematical surfaces and
authored coordinates. No supplied GLB, texture, rig, animation, image tracing or
third-party mesh is used. The former imported Aranea model is not included in
the distributable asset set.

The directory and cache filename retain `aranea` only for compatibility with
the existing runtime loader. The visual character is named **Threshold Sentinel**.

## Origin and permission

The complete editable source is [generate_sentinel.py](../../../tools/generate_sentinel.py).
The generated mesh, texture caches and animation bank are dedicated under
**CC0 1.0 Universal**; see [LICENSE.txt](LICENSE.txt). This dedication covers the
new generated payloads only, not the former imported assets or the rest of the
project's source code. The manifest records generator and output SHA-256 hashes.

## Geometry and materials

The bank contains **4,214 vertices, 8,120 triangles and 55 geometric parts**.
Its three material slots are an oxidized shell, pale ceramic panels and dark
sensor/joint recesses. The shared 512 by 512 color and normal textures are
analytic grain and machining marks generated from sine functions; no source
photograph or external image is required. Coordinates are metres, Y up and
negative Z forward.

The ordinary pose is about 2.40 m tall. Full doorway folding narrows the total
width to about 1.05 m and lowers the highest point to about 2.03 m. See the
manifest for exact per-clip bounds. The final capture clip has a separate
cinematic envelope and is centered on a 1.75 m sensor anchor.

## Animation and runtime

Seven clips contain 220 frames in the existing `BRARAN3` format:

| Clip | Frames | Nominal duration |
| --- | ---: | ---: |
| idle | 16 | 3.20 s |
| walk | 40 | 1.20 s |
| run | 40 | 0.65 s |
| idle_duck | 16 | 3.20 s |
| walk_duck | 40 | 1.20 s |
| run_duck | 40 | 0.65 s |
| scare | 28 | 2.00 s |

The authored walk keeps three supporting feet, while the run alternates diagonal
pairs. Support phases are 78% and 58%, and full-cycle stride lengths are 1.30 m
and 2.00 m. Two-segment inverse kinematics solves the knees. The head and lower
face plate articulate independently, and the capture raises the front legs.
Runtime motion interpolation, bounded world-space foot planting, pause behavior
and configurable capture timing continue to use the game's existing code.

`aranea.brm` stores bounded descriptors, indexed triangles, head/foot masks,
UVs, contact metadata and quantized position/normal frames. The runtime validates
counts, material IDs, indices, masks, normals, finite UVs and byte budgets before
rendering. `creature-color.rgb` and `creature-normal.rgb` use the existing
`ARATEX1` header. Keep all three payloads with the game.

## Reproduction

Install Python and NumPy for offline generation, then run from the project root:

```text
python tools/generate_sentinel.py
```

No Blender installation or asset download is required. Python and NumPy are
not runtime dependencies; the checked-in generated payloads are used directly.

`tests/threat_model_tests.cpp` checks the actual generated sensor landmarks,
all seven clips, normalized normals, fixed UVs, support contacts, foot planting,
doorway clearance, pause determinism, capture alignment and malformed-cache
rejection. Its generated-geometry assertions replace obsolete imported-mesh
counts while keeping behavior and validation checks.
