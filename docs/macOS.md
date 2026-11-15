# macOS build and launch

The Mac package includes the complete C++ source, shaders, configuration, assets,
licenses and tests. The target is macOS 11 or newer on Apple Silicon (arm64) and
Intel (x86_64). The ZIP manifest states whether a native executable is included.
A source-only package needs to be built on a Mac before playing; it is not a
tested, ready-to-play Mac binary.

## Open the game

Extract the whole ZIP into a writable folder. Keep the shaders and `assets`
directory beside the launchers. Open `Run Mac.command`, or
`Presentation Mac.command` for the fixed demonstration scene. A prebuilt package
uses `bin/Backrooms`; it does not require CMake, Homebrew or Python to play.
The launcher opens Terminal so errors are visible and writes `backrooms.log` in
the extracted project folder. Closing that Terminal session can close the game.

macOS may require confirmation before opening a downloaded, unsigned program.
For a package you trust, follow Apple's normal **Open Anyway** procedure in
System Settings > Privacy & Security. Do not disable Gatekeeper globally.
Apple's instructions are at
<https://support.apple.com/en-us/102445>.

If an extraction tool loses script permissions, open Terminal in the extracted
folder and run:

```sh
chmod +x "Run Mac.command" "Presentation Mac.command" "Build Mac.command"
./"Run Mac.command"
```

The packager preserves these permissions and Unix line endings. `Backrooms.exe`,
`.cmd` and PowerShell utilities are Windows files; use the Mac launchers here.

## Build from the included source

Install Apple's Command Line Tools once:

```sh
xcode-select --install
```

Install CMake 3.21 or newer. If Homebrew is already installed:

```sh
brew install cmake
```

Then open `Build Mac.command`. The first build needs an Internet connection to
download the pinned GLFW and SDL source archives. It compiles both architectures
and links those libraries statically. No package manager installs are performed
by the launcher. Subsequent builds reuse the downloads in `build/macos`.

Equivalent terminal commands from the project folder:

```sh
cmake -S . -B build/macos \
  -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DBACKROOMS_FETCH_MAC_DEPS=ON \
  -DBUILD_TESTING=OFF
cmake --build build/macos --config Release --target Backrooms --parallel 2
./"Run Mac.command"
```

For a developer-only build using installed libraries instead, install
`brew install cmake glfw sdl2`, configure `BACKROOMS_FETCH_MAC_DEPS=OFF`, and build
for the current machine architecture. Such an executable can depend on Homebrew
libraries and is not accepted as a self-contained universal release by the Mac
packager.

## Controls and audio

WASD and the mouse move and look. Escape pauses, H opens help, E interacts and F
toggles the flashlight. G toggles ambient occlusion. F4 toggles the independent
inspection camera; Space and Control move up and down in that mode. Enter leaves
the presentation scene for normal play. On keyboards where function keys control
system features, hold Fn/Globe while pressing F4, or change the keyboard setting.

Mac sound effects and background music use SDL's audio output. The bundled music
is a WAV file. Test speakers or headphones during a visible walkthrough: hidden
graphics checks deliberately suppress audio output. The packaged game still
requires a working OpenGL 3.3-capable driver/context.

## Validate the package on a Mac

Python 3 is only needed for the optional verification tools. From the extracted
folder, after building if the package is source-only:

```sh
python3 tools/verify-macos.py
```

The verifier checks the archive manifest and asset inventory, then runs the
fixed presentation with AO on/off, inspection, movement, capture/restart and
level/ending checks at 1280x720. Results, screenshots and logs are written under
`tests/artifacts/macos/`. `--manifest-only` checks packaged files on any OS without
launching the game. The result explicitly identifies the computer used.

For native and graphics unit tests, configure another build with
`BUILD_TESTING=ON` and `BACKROOMS_GRAPHICS_TESTS=ON`, build the default target,
then run `ctest --test-dir <build-directory> --output-on-failure`.
An automated or Windows-hosted check is not a completed Mac listening, keyboard,
mouse or performance test. Rehearse on the actual defence computer.

## Create a Mac submission ZIP

From the source folder, using Python 3:

```sh
python3 tools/package-macos.py \
  --executable build/macos/bin/Backrooms \
  --output "$HOME/Desktop/BackroomsCGF-Mac-Full-Project.zip"
```

The packager requires an executable with both arm64 and x86_64 Mach-O slices and
rejects dependencies outside macOS system libraries/frameworks. It includes all
project code, tests, tools, runtime assets, credits and build instructions. It
excludes caches, Git history, backups, development captures and Windows binaries.
The ZIP and its manifest record SHA-256 hashes; the original folder is unchanged.

Omit `--executable` to make an explicitly labeled source-only Mac package. This
does not turn a Windows executable into a Mac executable. The final report,
presentation and video are separate until supplied and passed using repeatable
`--deliverable` arguments. Their screenshots and credits should describe the
current packaged model/audio, and performance figures should name the tested OS
and machine.
