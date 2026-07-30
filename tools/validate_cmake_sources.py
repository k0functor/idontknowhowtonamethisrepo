#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_PATTERN = re.compile(r"\bsrc/[^\s)#]+\.(?:cpp|hpp)\b")


def strip_comment(line: str) -> str:
    # CMake '#' starts a comment for the simple source-list format used here.
    # Paths in this project do not contain '#', because civilisation has not fallen that far yet.
    return line.split("#", 1)[0]


def cmake_manifest_files() -> list[Path]:
    manifests = [ROOT / "CMakeLists.txt"]
    cmake_directory = ROOT / "cmake"
    if cmake_directory.is_dir():
        manifests.extend(sorted(cmake_directory.rglob("*.cmake")))
    return manifests


def collect_declared_sources() -> set[Path]:
    sources: set[Path] = set()
    for manifest in cmake_manifest_files():
        for line in manifest.read_text(encoding="utf-8").splitlines():
            clean = strip_comment(line)
            for match in SOURCE_PATTERN.findall(clean):
                sources.add(Path(match))
    return sources


def collect_filesystem_sources() -> set[Path]:
    return {
        path.relative_to(ROOT)
        for path in (ROOT / "src").rglob("*")
        if path.suffix in {".cpp", ".hpp"}
    }


def main() -> int:
    try:
        cmake_sources = collect_declared_sources()
    except Exception as exc:  # noqa: BLE001 - validator output should be user friendly.
        print(f"CMake source validation failed: {exc}")
        return 1

    filesystem_sources = collect_filesystem_sources()
    missing_files = sorted(cmake_sources - filesystem_sources, key=lambda path: path.as_posix())
    missing_from_cmake = sorted(filesystem_sources - cmake_sources, key=lambda path: path.as_posix())

    if missing_files or missing_from_cmake:
        print("CMake source validation failed:")
        if missing_files:
            print(" - listed in CMake manifests but missing on disk:")
            for path in missing_files:
                print(f"   * {path.as_posix()}")
        if missing_from_cmake:
            print(" - present under src/ but not listed in CMake manifests:")
            for path in missing_from_cmake:
                print(f"   * {path.as_posix()}")
        return 1

    print(
        f"CMake sources OK: {len(cmake_sources)} src files declared across "
        f"{len(cmake_manifest_files())} manifest(s)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
