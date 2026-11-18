#!/usr/bin/env python3
"""Verify an extracted Mac package and exercise the packaged/native-built game."""
import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import struct
import subprocess
import sys
import time


def sha256(path):
    with path.open("rb") as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def contained_file(root, relative):
    path = PurePosixPath(relative)
    require(bool(relative) and not path.is_absolute() and ".." not in path.parts and "\\" not in relative,
            "Invalid relative manifest path: " + relative)
    candidate = root.joinpath(*path.parts)
    require(root in candidate.resolve().parents, "Manifest path leaves the package: " + relative)
    require(not candidate.is_symlink() and candidate.is_file(), "Missing or linked file: " + relative)
    # Reject a linked directory component as well as a linked leaf file.
    parent = candidate.parent
    while parent != root:
        require(not parent.is_symlink(), "Linked manifest directory: " + str(parent))
        parent = parent.parent
    return candidate


def check_files(root, entries, provenance=False):
    require(isinstance(entries, list) and bool(entries), "Manifest file list is empty")
    seen = set()
    for entry in entries:
        relative = entry["path"]
        require(relative not in seen, "Duplicate manifest entry: " + relative)
        seen.add(relative)
        path = contained_file(root, relative)
        require(path.stat().st_size == entry["bytes"], "Size mismatch: " + relative)
        require(sha256(path).lower() == entry["sha256"].lower(), "SHA-256 mismatch: " + relative)
        if provenance:
            for key in ("license", "source", "creator"):
                require(bool(str(entry.get(key, "")).strip()), "Missing " + key + " provenance: " + relative)
    return seen


CASES = [
    ("menu", 8, [], ["Paused: yes"]),
    ("presentation-ao-on", 12, ["--presentation", "--presentation-check", "ao-on"],
     ["Presentation verified: ao-on", "AO amount 0.850000"]),
    ("presentation-ao-off", 12, ["--presentation", "--presentation-check", "ao-off"],
     ["Presentation verified: ao-off", "AO amount 0.000000"]),
    ("presentation-inspection", 12, ["--presentation", "--presentation-check", "camera"],
     ["Presentation verified: camera"]),
    ("walking", 180, ["--demo", "--auto-walk", "--auto-sprint", "--no-threat"], ["Paused: no", "Encounter: exploring"]),
    ("creature-capture", 12, ["--demo", "--auto-jumpscare"], ["Catches: 1", "Encounter: jumpscare"]),
    ("creature-restart", 165, ["--demo", "--auto-jumpscare", "--auto-restart"], ["Restarts: 1", "Encounter: exploring", "Key: missing"]),
    ("level0-to-fun", 120, ["--demo", "--auto-key", "--auto-exit", "--auto-walk", "--no-threat"], ["Level: Fun", "Exit used: yes"]),
    ("fun-to-parking", 200, ["--demo", "--level-fun", "--auto-key", "--auto-exit", "--auto-walk", "--no-threat"], ["Transition: Level Fun -> Parking.", "Level: Parking"]),
    ("parking-to-ending", 130, ["--demo", "--parking", "--auto-breakers", "--auto-exit", "--auto-walk", "--no-threat"], ["Parking completed.", "Ending started: parking exit", "Circuits: 3/3"]),
    ("city-ending", 8, ["--demo", "--ending-time", "32"], ["Ending phase: City life", "Ending city: 12 walking pedestrians / 6 moving cars."]),
    ("verity-pickup", 300, ["--demo", "--seed", "3", "--auto-verity", "--auto-walk", "--auto-sprint", "--no-threat"], ["Verity collected.", "Verity: active"]),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--executable", type=Path, help="Override packaged bin/Backrooms, for source-build verification")
    parser.add_argument("--manifest-only", action="store_true", help="Validate packaged files on any OS without running the game")
    parser.add_argument("--case-timeout", type=int, default=120,
                        help="Seconds allowed per scenario (default 120; increase for software-rendered CI)")
    args = parser.parse_args()
    require(1 <= args.case_timeout <= 3600, "Case timeout must be 1..3600 seconds")
    root = args.root.expanduser().resolve()
    manifest_path = root / "package-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    require(manifest.get("schema") == 1 and manifest.get("platform") == "macOS", "Expected a Mac package manifest")
    package_files = check_files(root, manifest["files"])
    required = ("CMakeLists.txt", "Run Mac.command", "Presentation Mac.command", "Build Mac.command",
                "docs/macOS.md", "THIRD_PARTY.md", "fshader_backrooms.glsl", "vshader_backrooms.glsl",
                "fshader_lens.glsl", "vshader_lens.glsl", "fshader_shadow.glsl", "vshader_shadow.glsl",
                "fshader_ao.glsl", "balance.cfg", "jumpscare.cfg",
                "assets/models/aranea/aranea.brm", "assets/models/aranea/creature-color.rgb",
                "assets/models/aranea/creature-normal.rgb", "assets/audio/music/threshold-ambient.wav")
    for relative in required:
        require(relative in package_files, "Required package file not listed: " + relative)
    if not manifest["sourceOnly"]:
        require("bin/Backrooms" in package_files, "Prebuilt package executable is absent")
    license_manifest = json.loads((root / "assets/asset-license-manifest.json").read_text(encoding="utf-8-sig"))
    asset_files = check_files(root / "assets", license_manifest["files"], provenance=True)
    actual_assets = {path.relative_to(root / "assets").as_posix() for path in (root / "assets").rglob("*")
                     if path.is_file() and path.name != ".DS_Store" and not path.name.startswith("._")}
    require(actual_assets == asset_files | {"asset-license-manifest.json"}, "Unlisted or missing asset in the extracted package")
    print("PASS package hashes ({} files)".format(len(package_files)), flush=True)
    print("PASS asset hashes and provenance ({} files)".format(len(asset_files)), flush=True)
    if args.manifest_only:
        print("PASS manifest verification; game execution was not requested.")
        return
    require(platform.system() == "Darwin", "Run the graphics checks on a Mac; use --manifest-only on this OS")
    executable = args.executable
    if executable:
        executable = executable.expanduser().resolve()
    elif (root / "bin/Backrooms").is_file():
        executable = root / "bin/Backrooms"
    else:
        executable = root / "build/macos/bin/Backrooms"
    require(executable.is_file() and os.access(executable, os.X_OK), "Build first using Build Mac.command; no executable is available")
    runtime_dir = executable.parent if (executable.parent / "vshader_backrooms.glsl").is_file() else root
    for name in ("Run Mac.command", "Presentation Mac.command", "Build Mac.command"):
        require(os.access(root / name, os.X_OK), "Launcher executable permission is missing: " + name)
    evidence = root / "tests/artifacts/macos" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    evidence.mkdir(parents=True)
    fixture = evidence / "gameplay.cfg"
    fixture.write_text("ambientOcclusion 0.65\nbrightness 1\nblur 0\nfov 78\nheadBob 1\nvsync 0\nmaxFps 0\nmusicEnabled 1\n", encoding="ascii")
    presentation_fixture = evidence / "presentation.cfg"
    presentation_fixture.write_text("ambientOcclusion 0\nbrightness 0.7\nblur 1.2\nfov 92\nheadBob 0.8\nvsync 0\nmaxFps 0\nmusicEnabled 0\n", encoding="ascii")
    results = {"schema": 1, "startedUtc": datetime.now(timezone.utc).isoformat(),
               "system": platform.platform(), "machine": platform.machine(),
               "executable": str(executable), "executableSha256": sha256(executable),
               "packageManifestSha256": sha256(manifest_path), "verifiedPackageFiles": len(package_files),
               "verifiedAssetFiles": len(asset_files), "cases": [],
               "note": "Hidden checks validate rendering/game flow, not audible playback, keyboard/mouse interaction or defence-machine performance."}
    results_path = evidence / "results.json"
    for name, frames, flags, expected in CASES:
        settings = presentation_fixture if "--presentation-check" in flags else fixture
        capture = evidence / (name + ".bmp")
        profile = evidence / (name + ".csv")
        arguments = [str(executable), "--hidden", "--seed", "12071998", "--width", "1280", "--height", "720",
                     "--fps-limit", "0", "--frames", str(frames), "--settings-file", str(settings),
                     "--capture", str(capture), "--profile", str(profile)] + flags
        print("RUN " + name, flush=True)
        started = time.monotonic()
        try:
            completed = subprocess.run(arguments, cwd=root, capture_output=True, timeout=args.case_timeout, check=False)
            (evidence / (name + "-stdout.txt")).write_bytes(completed.stdout)
            (evidence / (name + "-stderr.txt")).write_bytes(completed.stderr)
            log_path = runtime_dir / "backrooms.log"
            log = log_path.read_text(encoding="utf-8", errors="replace")
            shutil.copyfile(log_path, evidence / (name + ".log"))
            require(completed.returncode == 0, name + " exited with " + str(completed.returncode))
            for phrase in ["GL errors: 0", "GL resources after Game: 0", "Background music loaded.", "Completed {} frames.".format(frames)] + expected:
                require(phrase in log, name + " did not report: " + phrase)
            bitmap = capture.read_bytes()
            require(len(bitmap) >= 54 and bitmap[:2] == b"BM", name + " did not save a BMP screenshot")
            width, height = struct.unpack_from("<ii", bitmap, 18)
            require((width, abs(height)) == (1280, 720), name + " framebuffer is not 1280x720")
            if name == "walking":
                with profile.open(newline="", encoding="utf-8-sig") as stream:
                    positions = list(csv.DictReader(stream))
                require(bool(positions), "Walking produced no frame profile")
                last = positions[-1]
                require(math.hypot(float(last["x"]) - 3.75, float(last["z"]) - 3.75) > 1.0,
                        "Walking fixture did not move at least one metre")
            renderer = re.search(r"^Renderer: (.+)$", log, re.MULTILINE)
            item = {"name": name, "passed": True, "exitCode": completed.returncode,
                    "seconds": round(time.monotonic() - started, 3), "width": width, "height": abs(height),
                    "renderer": renderer.group(1).strip() if renderer else "not reported",
                    "glErrors": 0, "retainedGlObjects": 0, "musicLoaded": True, "args": arguments[1:]}
        except (OSError, ValueError, subprocess.TimeoutExpired) as error:
            results["cases"].append({"name": name, "passed": False, "error": str(error)})
            results_path.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
            raise
        results["cases"].append(item)
        results_path.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
        print("PASS " + name, flush=True)
    results["completedUtc"] = datetime.now(timezone.utc).isoformat()
    results_path.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print("PASS all {} Mac package smoke cases".format(len(CASES)))
    print("EVIDENCE " + str(evidence))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, struct.error, subprocess.TimeoutExpired) as error:
        print("Mac verification failed: " + str(error), file=sys.stderr)
        sys.exit(1)
