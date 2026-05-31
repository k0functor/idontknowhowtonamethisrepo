#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
LOCALE_DIR = DATA_DIR / "localization"

TEXT_ID_KEY_RE = re.compile(r"(^|_)(text_id|text)$|(_text_id|_text_ids)$")
TEXT_ID_VALUE_RE = re.compile(r"^[a-z0-9_.-]+\.[a-z0-9_.-]+$")
CPP_STRING_RE = re.compile(r'"([^"\\]*(?:\\.[^"\\]*)*)"')
# Hardcoded C++ string scanning is intentionally scoped to code that can render
# text in the game UI. Data loading, parsers, serializers, validation and other
# infrastructure may keep English diagnostics for developers. Mark exceptional
# developer-facing literals in scanned files with `NOL10N`.
PLAYER_FACING_CPP_PREFIXES = (
    "src/flow/",
    "src/inspect/",
    "src/scenes/",
    "src/ui/",
)
PLAYER_FACING_CPP_FILES = {
    "src/cards/CardUpgrade.cpp",
    "src/combat/ModifierSystem.cpp",
    "src/relics/RelicSystem.cpp",
}
NOL10N_MARKER = "NOL10N"
ALLOWED_CPP_SUBSTRINGS = (
    "data/", "config/", ".json", ".png", ".ttf", "debug", "Debug", "ERROR", "INFO",
    "WARN", "Missing", "Cannot", "failed", "Loaded", "validation", "Validation"
)


def load_json(path: Path):
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def load_locale(locale: str) -> tuple[dict[str, str], list[str]]:
    directory = LOCALE_DIR / locale
    texts: dict[str, str] = {}
    errors: list[str] = []

    if not directory.is_dir():
        return texts, [f"{directory}: locale directory is missing"]

    for path in sorted(directory.glob("*.json")):
        try:
            root = load_json(path)
        except Exception as exc:
            errors.append(f"{path}: invalid JSON: {exc}")
            continue

        if not isinstance(root, dict):
            errors.append(f"{path}: root must be an object")
            continue

        for text_id, value in root.items():
            if text_id in texts:
                errors.append(f"{path}: duplicate text id '{text_id}' in locale '{locale}'")
            if not isinstance(value, str):
                errors.append(f"{path}: value for '{text_id}' must be a string")
            texts[text_id] = value

    return texts, errors


def collect_text_refs(value, refs: set[str]) -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            key_lower = key.lower()
            if TEXT_ID_KEY_RE.search(key_lower):
                if isinstance(child, str):
                    refs.add(child)
                elif isinstance(child, list):
                    refs.update(item for item in child if isinstance(item, str))
            else:
                collect_text_refs(child, refs)
    elif isinstance(value, list):
        for child in value:
            collect_text_refs(child, refs)


def collect_data_text_refs() -> set[str]:
    refs: set[str] = set()
    for path in DATA_DIR.rglob("*.json"):
        if LOCALE_DIR in path.parents:
            continue
        try:
            collect_text_refs(load_json(path), refs)
        except Exception:
            continue
    return {ref for ref in refs if TEXT_ID_VALUE_RE.match(ref)}


def is_player_facing_cpp(relative: str) -> bool:
    return relative in PLAYER_FACING_CPP_FILES or relative.startswith(PLAYER_FACING_CPP_PREFIXES)


def is_localized_line(line: str) -> bool:
    return (
        "TextId(" in line or
        ".get(" in line or
        ".format(" in line or
        "rawText(" in line or
        "formatRawText(" in line
    )


def scan_cpp_strings() -> list[str]:
    warnings: list[str] = []
    for path in (ROOT / "src").rglob("*.cpp"):
        relative = path.relative_to(ROOT).as_posix()
        if not is_player_facing_cpp(relative):
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        for line_number, line in enumerate(text.splitlines(), start=1):
            stripped = line.strip()
            if NOL10N_MARKER in line:
                continue
            if stripped.startswith("#include"):
                continue
            if is_localized_line(line):
                continue
            for match in CPP_STRING_RE.finditer(line):
                literal = match.group(1)
                if not literal or len(literal) < 5:
                    continue
                if any(part in literal for part in ALLOWED_CPP_SUBSTRINGS):
                    continue
                if not re.search(r"\s", literal):
                    continue
                if re.search(r"[A-Za-z]{3,}", literal):
                    warnings.append(f"{relative}:{line_number}: possible hardcoded player-facing text: \"{literal}\"")
    return warnings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict-hardcoded", action="store_true")
    args = parser.parse_args()

    errors: list[str] = []
    ru, ru_errors = load_locale("ru")
    en, en_errors = load_locale("en")
    errors.extend(ru_errors)
    errors.extend(en_errors)

    ru_ids = set(ru)
    en_ids = set(en)
    for text_id in sorted(ru_ids - en_ids):
        errors.append(f"en is missing text id '{text_id}'")
    for text_id in sorted(en_ids - ru_ids):
        errors.append(f"ru is missing text id '{text_id}'")

    data_refs = collect_data_text_refs()
    for text_id in sorted(data_refs - ru_ids):
        errors.append(f"ru is missing data-referenced text id '{text_id}'")
    for text_id in sorted(data_refs - en_ids):
        errors.append(f"en is missing data-referenced text id '{text_id}'")

    warnings = scan_cpp_strings()

    if errors:
        print("Localization validation failed:")
        for error in errors:
            print(f" - {error}")
    if warnings:
        print("Hardcoded-text warnings:")
        for warning in warnings:
            print(f" - {warning}")

    if errors or (args.strict_hardcoded and warnings):
        return 1

    print(f"Localization OK: {len(ru_ids)} ru ids, {len(en_ids)} en ids, {len(data_refs)} data refs")
    if warnings:
        print(f"Warnings: {len(warnings)} possible hardcoded strings")
    return 0


if __name__ == "__main__":
    sys.exit(main())
