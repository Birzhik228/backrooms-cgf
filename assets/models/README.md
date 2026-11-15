# Project creature assets

The game now uses the original **Threshold Sentinel**, a procedural plated
quadruped generated from mathematical primitives, with seven animation clips
and analytic color/normal textures. The generated asset payloads are dedicated
under CC0 1.0 Universal. No imported creature model is required for building or
playing the submission.

See [the model documentation](aranea/README.md), [asset license](aranea/LICENSE.txt)
and [editable generator](../../tools/generate_sentinel.py). The `aranea` directory
and BRARAN3 cache name are retained for runtime compatibility only.

The previously supplied unlicensed GLB, source textures and converted caches
were moved to an ignored local backup outside the submission package. The
former import and rig-mapping tools are archived there too. They are not used
by the current generator or game.
