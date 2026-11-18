#!/usr/bin/env python3
"""Capture a bounded Mac startup and a stack sample if rendering is slow."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import platform
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args()
    if platform.system() != "Darwin" or not 60 <= args.timeout <= 1200:
        raise ValueError("Run on macOS with a 60..1200 second timeout")
    root = args.root.resolve()
    evidence = root / "tests/artifacts/mac-startup"
    evidence.mkdir(parents=True, exist_ok=True)
    fixture = evidence / "settings.cfg"
    fixture.write_text("ambientOcclusion 0.65\nbrightness 1\nblur 0\nfov 78\nheadBob 1\nvsync 0\nmaxFps 0\nmusicEnabled 1\n")
    command = [str(root / "bin/Backrooms"), "--hidden", "--seed", "12071998",
               "--width", "1280", "--height", "720", "--fps-limit", "0", "--frames", "8",
               "--settings-file", str(fixture), "--capture", str(evidence / "menu.bmp"),
               "--profile", str(evidence / "menu.csv")]
    result = {"startedUtc": datetime.now(timezone.utc).isoformat(), "command": command,
              "system": platform.platform(), "machine": platform.machine(), "timeoutSeconds": args.timeout}
    started = time.monotonic()
    with (evidence / "stdout.txt").open("wb") as stdout, (evidence / "stderr.txt").open("wb") as stderr:
        process = subprocess.Popen(command, cwd=root, stdout=stdout, stderr=stderr)
        try:
            process.wait(timeout=60)
        except subprocess.TimeoutExpired:
            print("Startup still running at 60 seconds; collecting stack sample", flush=True)
            sample = subprocess.run(["/usr/bin/sample", str(process.pid), "5", "-file", str(evidence / "stack.txt")],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
            (evidence / "sample-output.txt").write_bytes(sample.stdout)
            if (root / "backrooms.log").is_file():
                shutil.copyfile(root / "backrooms.log", evidence / "log-at-60s.txt")
            try:
                process.wait(timeout=max(1, args.timeout-(time.monotonic()-started)))
            except subprocess.TimeoutExpired:
                result["timedOut"] = True
                process.kill()
                process.wait()
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            if (root / "backrooms.log").is_file():
                shutil.copyfile(root / "backrooms.log", evidence / "backrooms.log")
            result["exitCode"] = process.returncode
            result["seconds"] = round(time.monotonic()-started, 3)
            result["completedUtc"] = datetime.now(timezone.utc).isoformat()
            (evidence / "result.json").write_text(json.dumps(result, indent=2)+"\n")
    print(json.dumps(result, indent=2), flush=True)
    return 0 if process.returncode == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
