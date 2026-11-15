#!/bin/bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
cd "$project_dir"
if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "Build Mac.command must run on a Mac. The Windows package uses Build.cmd."
    exit 1
fi

# Finder's Terminal session may not include the Homebrew binary directories.
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
if ! xcode-select -p >/dev/null 2>&1 || ! xcrun --find clang++ >/dev/null 2>&1; then
    echo "Install Apple's Command Line Tools first: xcode-select --install"
    echo "Finish the installation, then run this script again."
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake 3.21 or newer is required to compile the source."
    echo "If Homebrew is installed, run: brew install cmake"
    echo "See docs/macOS.md for setup and the prebuilt-package option."
    exit 1
fi

echo "Building for Apple Silicon and Intel (macOS 11 or newer)."
echo "The first build downloads the pinned GLFW and SDL source dependencies."
cmake -S "$project_dir" -B "$project_dir/build/macos" \
    -DCMAKE_BUILD_TYPE=Release \
    '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DBACKROOMS_FETCH_MAC_DEPS=ON \
    -DBUILD_TESTING=OFF
cmake --build "$project_dir/build/macos" --config Release --target Backrooms --parallel 2
if [[ ! -x "$project_dir/build/macos/bin/Backrooms" ]]; then
    echo "Build completed without the expected executable: build/macos/bin/Backrooms"
    exit 1
fi
mkdir -p "$project_dir/bin"
cp "$project_dir/build/macos/bin/Backrooms" "$project_dir/bin/Backrooms"
chmod +x "$project_dir/bin/Backrooms"
echo "Build complete. Open Run Mac.command or Presentation Mac.command."
