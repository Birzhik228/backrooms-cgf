BACKROOMS CGF SUBMISSION

Team: Rukhanov Birzhan, Zhamenov Gani, Rinatuly Miras

1. EXTRACT THE ZIP
Extract the complete BackroomsCGF folder before running anything. Keep the EXE,
assets, shaders, configuration files and tools together. Do not run inside the
ZIP viewer and do not copy the EXE by itself.

2. RUN
Double-click Run.cmd for the game, or Presentation.cmd for the fixed graphics
demonstration. A Windows console displays errors and writes backrooms.log.
Keep that console open while playing; closing it also closes the game.
The prebuilt Windows package needs an OpenGL 3.3-capable graphics driver.
MSYS2, Python, Blender and Git are not needed to play the prebuilt package.

3. DEMONSTRATE
G toggles ambient occlusion. F4 enters/leaves the inspection camera.
WASD and mouse move/look; Shift moves faster, Ctrl crouches in normal play.
In inspection mode Space moves up and Ctrl moves down.
H opens help, Escape pauses, E interacts, and F toggles the flashlight.
Enter leaves the frozen presentation scene and starts normal gameplay.

4. CHECK ON THE LAB COMPUTER
Close the game and double-click Lab Check.cmd. It verifies package hashes,
required files and asset provenance, runs automated rendering/gameplay checks,
then measures 1280x720 AO-on and AO-off performance. Allow several minutes.
It needs Windows PowerShell and a working OpenGL 3.3 graphics driver, but no
compiler or Git installation. It does not upload or submit anything.

Results are saved under:
  tests\artifacts\submission\<timestamp>\results.json
  tests\artifacts\ao-performance\<timestamp>\results.json

The AO benchmark measures a fixed hidden presentation view. It does not replace
a visible keyboard/mouse walkthrough. Rehearse all three levels and the ending,
check speaker/headphone audio, and record the lab machine's specifications.
Do not describe a check on another computer as a completed lab-computer test.

5. REBUILD FROM THE INCLUDED SOURCE
Install the documented CMake/MSYS2 GCC/GLFW prerequisites, then use Build.cmd or:
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Test -GraphicsTest
See docs\build-and-tests.md for setup details. A source-only ZIP has no EXE;
build it first. Rebuilding changes the executable hash from a prebuilt package.

6. REPORT PRESENTATION AND VIDEO
If final files were supplied when packaging, they are in Deliverables\ and are
listed in package-manifest.json. Otherwise they remain separate submissions.
The technical notes in docs\ are development records, not the team's final
report or presentation. Submit the final documents/video or accessible video
link through the course's required channel and verify access after uploading.

7. ASSET CREDITS AND CHANGES
See THIRD_PARTY.md, assets\asset-license-manifest.json, docs\licenses\ and
docs\asset-permissions-2026-11-15.md. The release uses a project-generated
Threshold Sentinel model and cleared/generated audio. Earlier recordings or
screenshots may show assets replaced during the permissions audit; ensure the
submitted presentation, report and video describe this packaged build.

package-manifest.json lists every packaged file except itself with size and
SHA-256. Verification results/logs/settings generated after extraction are not
part of that original manifest. Local backups, Git history and build caches
are deliberately excluded from the ZIP.
