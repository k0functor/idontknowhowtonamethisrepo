#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CMAKE_FILE = ROOT / "CMakeLists.txt"
TARGET_NAME = "i_dont_know_how_to_name_this_game"
SOURCE_PATTERN = re.compile(r"\bsrc/[^\s)#]+\.(?:cpp|hpp)\b")


def strip_comment(line: str) -> str:
    # CMake '#' starts a comment for the simple source-list format used here.
    # Paths in this project do not contain '#', because civilisation has not fallen that far yet.
    return line.split("#", 1)[0]


def collect_target_sources(cmake_text: str) -> set[Path]:
    lines = cmake_text.splitlines()
    start_index = None
    start_pattern = re.compile(rf"^\s*add_executable\s*\(\s*{re.escape(TARGET_NAME)}\b")
    for index, line in enumerate(lines):
        if start_pattern.search(line):
            start_index = index
            break

    if start_index is None:
        raise RuntimeError(f"cannot find add_executable({TARGET_NAME} ...) block")

    block_lines: list[str] = []
    depth = 0
    opened = False
    for line in lines[start_index:]:
        block_lines.append(line)
        clean = strip_comment(line)
        depth += clean.count("(")
        depth -= clean.count(")")
        if "(" in clean:
            opened = True
        if opened and depth <= 0:
            break
    else:
        raise RuntimeError(f"add_executable({TARGET_NAME} ...) block is not closed")

    sources: set[Path] = set()
    for line in block_lines:
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
        cmake_sources = collect_target_sources(CMAKE_FILE.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001 - validator output should be user friendly.
        print(f"CMake source validation failed: {exc}")
        return 1

    filesystem_sources = collect_filesystem_sources()
    missing_files = sorted(cmake_sources - filesystem_sources, key=lambda path: path.as_posix())
    missing_from_cmake = sorted(filesystem_sources - cmake_sources, key=lambda path: path.as_posix())

    if missing_files or missing_from_cmake:
        print("CMake source validation failed:")
        if missing_files:
            print(" - listed in CMakeLists.txt but missing on disk:")
            for path in missing_files:
                print(f"   * {path.as_posix()}")
        if missing_from_cmake:
            print(" - present under src/ but not listed in CMakeLists.txt:")
            for path in missing_from_cmake:
                print(f"   * {path.as_posix()}")
        return 1

    print(f"CMake sources OK: {len(cmake_sources)} src files listed for {TARGET_NAME}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
