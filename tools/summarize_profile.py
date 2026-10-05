"""Summarize --profile CSVs. Python standard library only; no game dependency.

Example: python tools/summarize_profile.py tests/artifacts/sprint.csv
The first 30 frames and final frame are reported/excluded explicitly so initial
GPU uploads and optional final screenshot readback cannot masquerade as travel.
"""
import argparse
import csv
import json
from pathlib import Path
import statistics


def summarize(path, warmup):
    with path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    if len(rows) <= warmup + 1:
        raise ValueError(f"{path}: need more than {warmup + 1} frames")
    steady = rows[warmup:-1]
    result = {
        "file": str(path),
        "frames": len(rows),
        "excluded_initial_frames": warmup,
        "excluded_final_frame": True,
        "first_frame_ms": float(rows[0]["frame_ms"]),
        "phases": {},
    }
    for key in rows[0]:
        if not key.endswith("_ms"):
            continue
        values = sorted(float(row[key]) for row in steady)
        result["phases"][key] = {
            "median": statistics.median(values),
            "p95": values[min(len(values) - 1, int(len(values) * .95))],
            "maximum": max(values),
        }
    result["frames_above_100ms"] = sum(float(row["frame_ms"]) > 100 for row in steady)
    crossings = [row for previous, row in zip(rows, rows[1:])
                 if (row["chunk_x"], row["chunk_z"]) != (previous["chunk_x"], previous["chunk_z"])]
    result["chunk_crossings"] = [
        {key: row[key] for key in ("frame", "chunk_x", "chunk_z", "world_ms", "upload_ms", "uploads", "frame_ms")}
        for row in crossings
    ]
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", nargs="+", type=Path)
    parser.add_argument("--warmup", type=int, default=30)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.warmup < 0:
        parser.error("warmup must be nonnegative")
    report = json.dumps([summarize(path, args.warmup) for path in args.csv], indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report + "\n", encoding="utf-8")
    print(report)
