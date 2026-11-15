#!/bin/bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
cd "$project_dir"
if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "Run Mac.command requires macOS. On Windows, use Run.cmd."
    exit 1
fi
if [[ -x "$project_dir/bin/Backrooms" ]]; then
    game="$project_dir/bin/Backrooms"
elif [[ -x "$project_dir/build/macos/bin/Backrooms" ]]; then
    game="$project_dir/build/macos/bin/Backrooms"
else
    /bin/bash "$project_dir/Build Mac.command"
    game="$project_dir/bin/Backrooms"
fi
# Keep the project root as the working directory: shaders, settings and assets
# are relative to it. Terminal displays diagnostics and backrooms.log records them.
exec "$game" "$@"
