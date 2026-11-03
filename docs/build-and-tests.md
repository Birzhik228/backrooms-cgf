# Build and test workflow

## One build graph

`CMakeLists.txt` defines the application's sources, shared libraries, compiler
settings, runtime files and test registration. `build.ps1` and `Makefile` only
select a configuration and invoke CMake/CTest. `Build.cmd` and `Run.cmd` preserve
the existing Windows double-click workflow. No additional library is linked into
the game by this build change.

The game and test executables reuse static libraries for world generation,
creature navigation/model animation, audio/music, GL loading, shaders and render
support. A world or audio source file is no longer compiled separately for every
test executable. CMake tracks headers and newly added `app/*.cpp` files. A second
build skips unchanged compilation; `-IfNeeded` relies on this graph instead of a
separate timestamp/file list.

Release retains the existing `-O2` optimization for GCC. All native tests keep
assertions enabled even in Release. `-Wall -Wextra` remains enabled. Builds use
two parallel compiler jobs by default; `build.ps1 -Jobs N` or `make JOBS=N` changes
that limit. This is a build setting, not the game's frame-rate cap.

## Windows contributors

Install MSYS2 GCC, GLFW, make and CMake 3.21+ for the same toolchain prefix. The
README gives the MinGW64 package command. UCRT64 is also supported when its
matching packages are installed. `BACKROOMS_CXX` may point at the desired
`g++.exe`; the wrapper discovers GCC, make and GLFW beside it. Its PATH change is
local to that PowerShell process.

```powershell
.\build.ps1 -IfNeeded
.\build.ps1 -Test
.\build.ps1 -GraphicsTest
.\build.ps1 -Test -GraphicsTest
.\build.ps1 -DebugBuild
```

Close the game before replacing its executable. Release intermediates go into
`build/windows-release/`; Debug uses `build/windows-debug/`. The game is built
under `bin/` and CMake's `backrooms_deploy` target copies it to the root, where
`Run.cmd` launches it. Settings are not part of the runtime-copy target.

The MinGW configuration explicitly selects `libglfw3.a` instead of the GLFW
package's DLL import target and statically links GCC's runtime. A player still
needs Windows and a compatible graphics driver, but does not need MSYS2 on PATH.
The PowerShell wrapper checks and propagates configure, compile and test failures.

## Direct CMake and Make

Use a shell with the intended compiler/dependencies available:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset native
```

For explicit paths or a different generator:

```sh
cmake -S . -B build/local -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/local --parallel 2 --target Backrooms native_tests
ctest --test-dir build/local -C Release -L '^native$' --output-on-failure
```

`build/local/bin/` contains the executable and its shaders, configuration and
runtime audio/models. The runtime-copy target also updates data-only edits during
an incremental build. It never copies `settings.cfg`, logs or captures. To update
the familiar root executable, build `backrooms_deploy` instead of `Backrooms`.

`make` calls that deployment target; `make test` builds/runs native tests;
`make graphics-test` enables and runs the graphics group. Make accepts
`BUILD_DIR`, `CONFIG`, `JOBS` and additional `CMAKE_ARGS`. Configuration is cached
per build directory: use a different directory when changing compiler/generator.

## Test groups

All test registration lives in CMake. Existing behavioral tests remain included,
alongside configuration validation, performance statistics and GL ownership tests.

- `native`: no display/context required. Includes generation/streaming, player and
  creature behavior, progression, cinematic sampling, recorded/synthesized audio,
  configuration, performance statistics and fake-entrypoint GL ownership tests.
- `audio-device`: an additional label on Windows `music_lifecycle`. This check
  exercises a real MCI/WAV device. Local `build.ps1 -Test` runs it; hosted Windows
  CI explicitly excludes it because an audio endpoint is not guaranteed.
- `graphics`: requires an actual OpenGL 3.3 context. Includes shadow/AO images,
  lens noise, city lighting and exceptional resource-lifetime cleanup.

Graphics checks are opt-in, so ordinary native runs do not attempt to create a
window on a headless computer:

```sh
cmake --preset graphics
cmake --build --preset graphics --target graphics_tests_build
ctest --preset graphics
```

On Linux without a physical display, use Mesa and Xvfb:

```sh
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1280x800x24' ctest --preset graphics
```

CTest runs from the source asset directory, gives every case a finite timeout,
and writes failure details to `build/<configuration>/Testing/Temporary/`.
Existing full-game verification scripts remain under `tools/`; they complement
native and rendered tests with menus, interactions and transitions.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes, pull requests and manual dispatch:

1. Ubuntu builds the game and runs all native tests without a display server.
2. Windows/UCRT64 builds the game and native tests, then runs the native group
   excluding the hardware audio-device case.
3. A separate Ubuntu job provides Mesa software OpenGL and Xvfb for the graphics
   group. It does not assume an OpenGL context is available in the native jobs.

The workflow has read-only repository permissions, does not persist checkout
credentials, cancels superseded runs and uploads diagnostic logs/artifacts after
failures. It does not publish releases or change branches. Hosted results become
available only after this workflow is committed/pushed to GitHub; a locally
validated workflow file is not a completed hosted run.

Action usage follows the official [checkout](https://github.com/actions/checkout),
[upload-artifact](https://github.com/actions/upload-artifact) and
[MSYS2 setup](https://github.com/msys2/setup-msys2) documentation.
