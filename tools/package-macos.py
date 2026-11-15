#!/usr/bin/env python3
"""Package Mac source/runtime files; optionally include a universal executable."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import stat
import struct
import sys
import zipfile


ROOT = Path(__file__).resolve().parent.parent
SOURCE_DIRS = ("app", "common", "include", "tools", "docs", ".github", "tests")
TEXT_SUFFIXES = {".c", ".cc", ".cpp", ".m", ".mm", ".h", ".hpp", ".glsl",
                 ".py", ".ps1", ".cmd", ".command", ".sh", ".json", ".md",
                 ".txt", ".cmake", ".in", ".yml", ".yaml", ".xml"}
ROOT_SUFFIXES = TEXT_SUFFIXES | {".cfg"}
ROOT_NAMES = {"CMakeLists.txt", "CMakePresets.json", "Makefile", "README.md",
              "README_FIRST.txt", "THIRD_PARTY.md", ".gitattributes", ".gitignore"}
SKIP_DIRS = {"__pycache__", ".qa", "artifacts", "node_modules", ".git", "build", "backups", "dist"}
SKIP_NAMES = {"report-draft.md", "presentation.html", ".DS_Store", "settings.cfg"}
REQUIRED = ("CMakeLists.txt", "Run Mac.command", "Build Mac.command",
            "Presentation Mac.command", "docs/macOS.md", "THIRD_PARTY.md",
            "tools/verify-macos.py", "assets/asset-license-manifest.json",
            "assets/models/aranea/aranea.brm", "assets/audio/music/threshold-ambient.wav",
            "vshader_backrooms.glsl", "fshader_backrooms.glsl", "fshader_ao.glsl",
            "vshader_lens.glsl", "fshader_lens.glsl", "vshader_shadow.glsl",
            "fshader_shadow.glsl", "balance.cfg", "jumpscare.cfg")
ARCH_NAMES = {0x01000007: "x86_64", 0x0100000C: "arm64"}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def is_link(path):
    return path.is_symlink() or getattr(path, "is_junction", lambda: False)()


def walk_files(directory):
    if is_link(directory):
        raise ValueError("Linked input directory is not accepted: " + str(directory))
    for path in sorted(directory.iterdir()):
        if is_link(path):
            raise ValueError("Linked input is not accepted: " + str(path))
        if path.is_dir():
            if path.name not in SKIP_DIRS:
                yield from walk_files(path)
        elif path.is_file() and path.name not in SKIP_NAMES and not path.name.startswith("._"):
            yield path


def macho_slices(data):
    """Validate Mach-O slices and collect dynamic library load commands."""
    magic = data[:4]
    fat = {b"\xca\xfe\xba\xbe": (">", False), b"\xbe\xba\xfe\xca": ("<", False),
           b"\xca\xfe\xba\xbf": (">", True), b"\xbf\xba\xfe\xca": ("<", True)}
    if magic in fat:
        endian, fat64 = fat[magic]
        if len(data) < 8:
            raise ValueError("Truncated universal executable header")
        count, = struct.unpack_from(endian + "I", data, 4)
        entry_size = 32 if fat64 else 20
        if not 1 <= count <= 16 or 8 + count * entry_size > len(data):
            raise ValueError("Invalid universal executable slice table")
        ranges = []
        slices = []
        for index in range(count):
            entry = 8 + index * entry_size
            cpu, = struct.unpack_from(endian + "I", data, entry)
            offset, size = struct.unpack_from(endian + ("QQ" if fat64 else "II"), data, entry + 8)
            if offset < 8 + count * entry_size or size < 32 or offset + size > len(data):
                raise ValueError("Invalid Mach-O slice boundaries")
            if any(offset < end and start < offset + size for start, end in ranges):
                raise ValueError("Overlapping Mach-O slices")
            ranges.append((offset, offset + size))
            slices.append(read_macho_slice(data[offset:offset + size], cpu))
        return slices
    return [read_macho_slice(data)]


def read_macho_slice(data, expected_cpu=None):
    magic = data[:4]
    formats = {b"\xcf\xfa\xed\xfe": "<", b"\xfe\xed\xfa\xcf": ">"}
    if magic not in formats or len(data) < 32:
        raise ValueError("Expected a 64-bit Mac Mach-O executable, not a Windows EXE")
    endian = formats[magic]
    cpu, _subtype, file_type, command_count, command_bytes = struct.unpack_from(endian + "5I", data, 4)
    if cpu not in ARCH_NAMES or (expected_cpu is not None and cpu != expected_cpu):
        raise ValueError("Unsupported or inconsistent Mach-O CPU type")
    if file_type != 2 or 32 + command_bytes > len(data):
        raise ValueError("Mach-O slice is not a valid executable")
    libraries = []
    cursor = 32
    command_end = cursor + command_bytes
    for _ in range(command_count):
        if cursor + 8 > command_end:
            raise ValueError("Truncated Mach-O load command")
        command, size = struct.unpack_from(endian + "II", data, cursor)
        if size < 8 or cursor + size > command_end:
            raise ValueError("Invalid Mach-O load command size")
        # LC_LOAD_DYLIB, WEAK, REEXPORT, LAZY and UPWARD, ignoring LC_REQ_DYLD.
        if (command & 0x7FFFFFFF) in {0xC, 0x18, 0x1F, 0x20, 0x23}:
            if size < 24:
                raise ValueError("Truncated Mach-O dependency command")
            name_offset, = struct.unpack_from(endian + "I", data, cursor + 8)
            if not 24 <= name_offset < size:
                raise ValueError("Invalid Mach-O dependency name")
            name_data = data[cursor + name_offset:cursor + size]
            if b"\0" not in name_data:
                raise ValueError("Unterminated Mach-O dependency name")
            name = name_data.split(b"\0", 1)[0].decode("utf-8")
            if not name.startswith(("/System/Library/", "/usr/lib/")):
                raise ValueError("Non-system dynamic dependency; use static GLFW/SDL: " + name)
            libraries.append(name)
        cursor += size
    if cursor != command_end:
        raise ValueError("Mach-O load command lengths do not match the header")
    return {"architecture": ARCH_NAMES[cpu], "systemLibraries": sorted(set(libraries))}


def collect_source():
    selected = {}
    for path in sorted(ROOT.iterdir()):
        if path.is_file() and path.name not in SKIP_NAMES and (path.name in ROOT_NAMES or path.suffix in ROOT_SUFFIXES):
            if is_link(path):
                raise ValueError("Linked root input is not accepted: " + str(path))
            selected[path.name] = path
    for folder in SOURCE_DIRS:
        directory = ROOT / folder
        if not directory.is_dir():
            continue
        for path in walk_files(directory):
            if path.suffix in TEXT_SUFFIXES:
                selected[path.relative_to(ROOT).as_posix()] = path
    for path in walk_files(ROOT / "assets"):
        selected[path.relative_to(ROOT).as_posix()] = path
    for required in REQUIRED:
        if required not in selected:
            raise ValueError("Required Mac package file is missing: " + required)
    return selected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, help="Universal static-dependency Mach-O executable; omit for source-only")
    parser.add_argument("--output", type=Path, required=True, help="New output ZIP; existing files are never overwritten")
    parser.add_argument("--deliverable", action="append", type=Path, default=[], help="Final report/slides/video file; repeatable")
    args = parser.parse_args()
    output = args.output.expanduser().resolve()
    if output.suffix.lower() != ".zip":
        raise ValueError("Output must end in .zip")
    if output.exists():
        raise ValueError("Output already exists: " + str(output))
    for folder in (*SOURCE_DIRS, "assets"):
        if output == ROOT / folder or ROOT / folder in output.parents:
            raise ValueError("Choose an output location outside packaged source/asset directories")
    selected = collect_source()
    contents = {}
    for relative, path in selected.items():
        data = path.read_bytes()
        # Shell scripts must remain directly executable after Windows packaging.
        if path.suffix in {".command", ".sh"}:
            data = data.replace(b"\r\n", b"\n")
            if data.startswith(b"\xef\xbb\xbf"):
                data = data[3:]
        contents[relative] = data
    executable_info = []
    if args.executable:
        if is_link(args.executable.expanduser()):
            raise ValueError("Executable must not be a symbolic link")
        executable = args.executable.expanduser().resolve()
        if not executable.is_file() or is_link(executable):
            raise ValueError("Executable must be an ordinary file")
        data = executable.read_bytes()
        executable_info = macho_slices(data)
        if sorted(info["architecture"] for info in executable_info) != ["arm64", "x86_64"]:
            raise ValueError("Release executable must contain both arm64 and x86_64 slices")
        contents["bin/Backrooms"] = data
    deliverables = []
    for item in args.deliverable:
        if not item.is_file() or is_link(item):
            raise ValueError("Deliverable must be an ordinary file: " + str(item))
        relative = "Deliverables/" + item.name
        if relative in contents:
            raise ValueError("Duplicate deliverable filename: " + item.name)
        contents[relative] = item.read_bytes()
        deliverables.append(relative)
    source_only = not bool(args.executable)
    status = ("SOURCE-ONLY: build on a Mac before playing. No native executable is included."
              if source_only else "PREBUILT: universal Mac executable included for Apple Silicon and Intel.")
    contents["START_HERE_MAC.txt"] = ("BACKROOMS CGF - MAC PACKAGE\n\n" + status + "\n\n"
        "1. Extract this entire ZIP into a writable folder.\n"
        "2. Read docs/macOS.md for setup, launch and verification instructions.\n"
        "3. Open Run Mac.command to play, or Presentation Mac.command for the demo.\n"
        "A source-only first launch needs Xcode Command Line Tools, CMake and Internet.\n"
        "A prebuilt launch needs no compiler or Homebrew libraries.\n\n"
        "All project source code, runtime assets and licenses are included.\n"
        "The package targets macOS 11 or newer. Verify it on the actual Mac before submission.\n"
        "Use python3 tools/verify-macos.py for package/rendering checks.\n"
        "The manifest records packaged files; verification outputs are created after extraction.\n\n"
        "Final report, slides and video: " + (", ".join(deliverables) if deliverables else "not included; supply them separately/later") + ".\n"
        "See THIRD_PARTY.md for asset and library credits.\n").encode("utf-8")
    entries = []
    folded = set()
    for relative, data in sorted(contents.items()):
        if relative.casefold() in folded:
            raise ValueError("Case-insensitive package filename collision: " + relative)
        folded.add(relative.casefold())
        mode = 0o755 if relative.endswith((".command", ".sh")) or relative == "bin/Backrooms" else 0o644
        entries.append({"path": relative, "bytes": len(data), "sha256": digest(data), "mode": oct(mode)})
    manifest = {"schema": 1, "package": "BackroomsCGF", "platform": "macOS",
                "createdUtc": datetime.now(timezone.utc).isoformat(), "sourceOnly": source_only,
                "targetArchitectures": ["arm64", "x86_64"], "minimumMacOS": "11.0",
                "executable": executable_info, "deliverables": deliverables, "files": entries}
    contents["package-manifest.json"] = (json.dumps(manifest, indent=2) + "\n").encode("utf-8")
    output.parent.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().timetuple()[:6]
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for relative, data in sorted(contents.items()):
            entry = zipfile.ZipInfo("BackroomsCGF/" + relative, timestamp)
            entry.create_system = 3
            mode = 0o755 if relative.endswith((".command", ".sh")) or relative == "bin/Backrooms" else 0o644
            entry.external_attr = (stat.S_IFREG | mode) << 16
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data)
    result = {"zip": str(output), "sha256": digest(output.read_bytes()),
              "zipBytes": output.stat().st_size, "fileCount": len(contents),
              "sourceOnly": source_only, "targetArchitectures": manifest["targetArchitectures"],
              "deliverables": deliverables}
    output.with_suffix(".package.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    output.with_suffix(".sha256.txt").write_text(result["sha256"] + "  " + output.name + "\n", encoding="ascii")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, struct.error, zipfile.BadZipFile) as error:
        print("Mac packaging failed: " + str(error), file=sys.stderr)
        sys.exit(1)
