#!/usr/bin/env python3
"""Validate that a portable release contains only distributable runtime files."""

from __future__ import annotations

import argparse
import json
import stat
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Iterable


FORBIDDEN_SUFFIXES = {".dll", ".ilk", ".log", ".pdb"}
FORBIDDEN_TOP_LEVEL = {
    ".git",
    ".github",
    "build",
    "dist",
    "docs",
    "reports",
    "src",
    "tests",
    "tools",
}
FORBIDDEN_ROOT_FILES = {
    ".gitignore",
    "cmakelists.txt",
    "cmakepresets.json",
}


class PackageValidationError(RuntimeError):
    pass


@dataclass(frozen=True)
class PackageEntry:
    path: PurePosixPath
    data: bytes | None
    is_directory: bool = False
    is_symlink: bool = False


class PackageReader:
    def entries(self) -> list[PackageEntry]:
        raise NotImplementedError


class DirectoryReader(PackageReader):
    def __init__(self, root: Path) -> None:
        self.root = root

    def entries(self) -> list[PackageEntry]:
        result: list[PackageEntry] = []
        for path in sorted(self.root.rglob("*")):
            relative = PurePosixPath(path.relative_to(self.root).as_posix())
            is_symlink = path.is_symlink()
            is_directory = path.is_dir()
            data = None if is_directory or is_symlink else path.read_bytes()
            result.append(PackageEntry(relative, data, is_directory, is_symlink))
        return result


class ZipReader(PackageReader):
    def __init__(self, archive: Path) -> None:
        self.archive = archive

    def entries(self) -> list[PackageEntry]:
        result: list[PackageEntry] = []
        with zipfile.ZipFile(self.archive) as package:
            for info in package.infolist():
                raw_name = info.filename.replace("\\", "/")
                path = PurePosixPath(raw_name.rstrip("/"))
                mode = (info.external_attr >> 16) & 0xFFFF
                is_symlink = stat.S_ISLNK(mode)
                is_directory = info.is_dir()
                data = None if is_directory or is_symlink else package.read(info)
                result.append(PackageEntry(path, data, is_directory, is_symlink))
        return result


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path, help="Package directory or .zip archive")
    parser.add_argument(
        "--executable",
        default="i_dont_know_how_to_name_this_game.exe",
        help="Expected executable filename",
    )
    return parser.parse_args()


def fail(message: str) -> None:
    raise PackageValidationError(message)


def validate_entry_path(path: PurePosixPath) -> None:
    if not path.parts:
        fail("Package contains an empty path")
    if path.is_absolute() or any(part in {"", ".", ".."} for part in path.parts):
        fail(f"Unsafe package path: {path}")


def validate_paths(entries: Iterable[PackageEntry]) -> dict[str, PackageEntry]:
    files: dict[str, PackageEntry] = {}
    casefolded: dict[str, str] = {}

    for entry in entries:
        validate_entry_path(entry.path)
        rendered = entry.path.as_posix()
        normalized = rendered.casefold()

        previous = casefolded.get(normalized)
        if previous is not None and previous != rendered:
            fail(f"Windows path collision: '{previous}' and '{rendered}'")
        casefolded[normalized] = rendered

        if entry.is_symlink:
            fail(f"Symbolic links are not allowed in a portable package: {rendered}")
        if entry.is_directory:
            continue

        files[rendered] = entry

        top_level = entry.path.parts[0].casefold()
        if top_level in FORBIDDEN_TOP_LEVEL:
            fail(f"Development-only directory included in package: {rendered}")
        if len(entry.path.parts) == 1 and top_level in FORBIDDEN_ROOT_FILES:
            fail(f"Development-only file included in package: {rendered}")
        if top_level in {"logs", "saves"}:
            fail(f"Runtime-local state included in package: {rendered}")
        if entry.path.suffix.casefold() in FORBIDDEN_SUFFIXES:
            fail(f"Forbidden binary, debug, or log artifact included in package: {rendered}")

    return files


def require_file(files: dict[str, PackageEntry], path: str) -> PackageEntry:
    entry = files.get(path)
    if entry is None:
        fail(f"Required package file is missing: {path}")
    return entry


def validate_config(entry: PackageEntry) -> None:
    assert entry.data is not None
    try:
        root = json.loads(entry.data.decode("utf-8-sig"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        fail(f"config/app.json is not valid UTF-8 JSON: {error}")

    debug = root.get("debug", {})
    if not isinstance(debug, dict):
        fail("config/app.json: 'debug' must be an object")
    if debug.get("enabled", False) is not False:
        fail("config/app.json enables debug tools in a release package")


def validate_package(package: Path, executable: str) -> tuple[int, int]:
    if package.is_dir():
        reader: PackageReader = DirectoryReader(package)
    elif package.is_file() and package.suffix.casefold() == ".zip":
        if not zipfile.is_zipfile(package):
            fail(f"Not a valid ZIP archive: {package}")
        reader = ZipReader(package)
    else:
        fail(f"Package path must be a directory or .zip archive: {package}")

    files = validate_paths(reader.entries())
    executable_entry = require_file(files, executable)
    if executable_entry.data is None or not executable_entry.data:
        fail(f"Executable is empty: {executable}")

    config_entry = require_file(files, "config/app.json")
    validate_config(config_entry)

    data_files = [path for path in files if path.startswith("data/")]
    if not data_files:
        fail("Package does not contain runtime data files under data/")

    total_bytes = sum(len(entry.data or b"") for entry in files.values())
    return len(files), total_bytes


def main() -> int:
    args = parse_arguments()
    try:
        file_count, total_bytes = validate_package(args.package, args.executable)
    except PackageValidationError as error:
        print(f"Release package validation failed: {error}", file=sys.stderr)
        return 1

    print(
        "Release package validation passed: "
        f"{file_count} files, {total_bytes / (1024 * 1024):.2f} MiB"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
