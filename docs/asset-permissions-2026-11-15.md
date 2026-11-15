# Asset permission audit and submission changes

The 15 November 2026 submission uses an original procedural creature, original
generated music/pickup cue and documented CC0 recordings. The audit distinguishes
an available download from permission to redistribute it. This record describes
the evidence used for this package, not a general opinion about every possible
use of the older files.

| Earlier asset | Evidence and decision | Submission replacement |
| --- | --- | --- |
| Aranea GLB and carpet textures | The matching [Sketchfab listing](https://sketchfab.com/3d-models/rec-room-backrooms-entity-62-aranea-membri-714f9831e49249ceb8aa14252fc530b0) shows CC Attribution, but uploader Lol_o9 says they did not make the model. Original creator/authority is unresolved. | Original Threshold Sentinel mesh, textures and animation generated without imported assets. |
| Temporex Numbers recording | User-supplied commercial recording; no redistribution grant was established. | Original 48-second ambient loop generated from oscillators and seeded noise. |
| Horror Skunx Verity excerpt | [Apple Music](https://music.apple.com/us/album/its-me-its-verity/6799279365?i=6799279367) identifies the recording and its copyright; the [Myinstants clip](https://www.myinstants.com/en/instant/hey-its-me-its-verity-7411/) provides no established redistribution grant. | Original two-second pickup chime. |
| YouTube Aranea calls and mixed derivatives | [Video source](https://www.youtube.com/watch?v=wzUCsTD23rg) credits other creators, with no verified reuse grant. | New processing using only the verified CC0 ZeroKingFull roar. |
| Player footsteps 6385 | [Pixabay source](https://pixabay.com/sound-effects/household-indoor-footsteps-6385/) credits dkiller2204 from Freesound under the Pixabay Content License. | Existing GboxMikeFozzy CC0 recordings and documented derivatives. |
| Heavy footsteps 352771 | [Pixabay source](https://pixabay.com/sound-effects/film-special-effects-heavy-walking-footsteps-352771/) credits Universfield under the Pixabay Content License. | Documented Fantozzi CC0 sand contacts. |
| Partygoer jumpscare 39630 | [Pixabay source](https://pixabay.com/sound-effects/film-special-effects-jumpscare-sound-39630/) credits BrotherMster from Freesound under the Pixabay Content License. | CC0 roar-based cue. |
| ZeroKingFull Monster Roar 02 | [Freesound primary page](https://freesound.org/people/zerokingfull/sounds/347410/) explicitly states CC0. | Retained original and capture derivative; license documentation corrected. |

[Pixabay's terms](https://pixabay.com/service/terms/) distinguish integrated
creative use from standalone redistribution. The three Pixabay recordings were
replaced for simpler source-package terms, not because no license exists.

Retained third-party sources are [GboxMikeFozzy Footsteps](https://opengameart.org/content/footsteps-0),
[Fantozzi sand contacts](https://opengameart.org/content/fantozzis-footsteps-grasssand-stone)
and the [ZeroKingFull roar](https://freesound.org/people/zerokingfull/sounds/347410/).
Their license is [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).
The project retains source URLs, creator names, processing descriptions and
SHA-256 hashes. All runtime audio is indexed in `assets/audio/manifest.json`.
The full asset inventory is `assets/asset-license-manifest.json`.

Superseded original files and derivatives were preserved locally in
`backups/asset-permissions-2026-11-15/` and excluded from the ZIP. They also remain
in earlier Git history; this work does not rewrite that history. Old cached
builds and captures are excluded too. The package includes the relevant software
license notices in `docs/licenses/`.

The final report, slides and video must describe or show the replacement assets
where relevant. The 14 November performance report identifies the older binary;
the new package must be measured separately. Tests on this computer do not
establish performance on the professor's separate computer.
