#!/usr/bin/env python3
"""Validate release-critical source-tree invariants before a contest build."""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ContestReadinessError(RuntimeError):
    pass


def fail(message: str) -> None:
    raise ContestReadinessError(message)


def load_json(relative: str):
    path = ROOT / relative
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"Cannot read {relative}: {error}")


def require_contains(relative: str, needles: list[str]) -> None:
    path = ROOT / relative
    try:
        text = path.read_text(encoding="utf-8-sig")
    except OSError as error:
        fail(f"Cannot read {relative}: {error}")
    for needle in needles:
        if needle not in text:
            fail(f"{relative} is missing release-critical marker: {needle}")


def validate_config() -> None:
    root = load_json("config/app.json")
    debug = root.get("debug")
    if not isinstance(debug, dict) or debug.get("enabled") is not False:
        fail("config/app.json must disable debug tools by default")

    window = root.get("window")
    if not isinstance(window, dict):
        fail("config/app.json must define a window object")
    if int(window.get("width", 0)) < 960 or int(window.get("height", 0)) < 540:
        fail("default window is too small for the contest UI")
    if int(window.get("framerate_limit", 0)) <= 0:
        fail("default frame-rate limit must be positive")

    paths = root.get("paths")
    if not isinstance(paths, dict):
        fail("config/app.json must define runtime paths")
    for key in ("assets", "data", "saves"):
        value = paths.get(key)
        if not isinstance(value, str) or not value or Path(value).is_absolute() or ".." in Path(value).parts:
            fail(f"runtime path '{key}' must be a safe relative path")


def validate_runtime_state_is_ignored() -> None:
    gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8-sig")
    if "saves/**" not in gitignore or "!saves/.gitkeep" not in gitignore:
        fail(".gitignore must exclude runtime saves while retaining saves/.gitkeep")
    if "logs/" not in gitignore:
        fail(".gitignore must exclude runtime logs")


def validate_main_menu() -> None:
    menu = (ROOT / "src/scenes/MainMenuScene.cpp").read_text(encoding="utf-8-sig")
    if "ui.credits_later" in menu:
        fail("main menu still exposes the non-functional credits placeholder")

    for locale in ("ru", "en"):
        core = load_json(f"data/localization/{locale}/core.json")
        if "ui.credits_later" in core:
            fail(f"{locale} localization still contains the credits placeholder")


def validate_release_configuration() -> None:
    presets = load_json("CMakePresets.json")
    release = next(
        (preset for preset in presets.get("configurePresets", []) if preset.get("name") == "gcc-release"),
        None,
    )
    if release is None:
        fail("gcc-release configure preset is missing")
    cache = release.get("cacheVariables", {})
    if cache.get("CMAKE_BUILD_TYPE") != "Release":
        fail("gcc-release must configure a Release build")
    if str(cache.get("GAME_STATIC_LINK_LIBRARIES", "")).upper() != "ON":
        fail("gcc-release must request static runtime linkage")
    if str(cache.get("GAME_BUILD_APP", "")).upper() != "ON":
        fail("gcc-release must build the application")

    require_contains(
        "src/main.cpp",
        ["--smoke-test", "Application app;"],
    )
    require_contains(
        "tools/package_windows_release.ps1",
        [
            "validate_contest_readiness.py",
            "Invoke-PackagedSmokeTest",
            'Arguments @("--smoke-test")',
            "validate_release_package.py",
        ],
    )


def validate_finale_variety() -> None:
    root = load_json("data/encounters/act5_encounters.json")
    if not isinstance(root, dict):
        fail("act5 encounter file must contain an object")
    bosses = root.get("pools", {}).get("boss")
    if bosses is None:
        # Current data shape stores the pools below the first top-level value.
        for value in root.values():
            if isinstance(value, dict) and isinstance(value.get("boss"), list):
                bosses = value["boss"]
                break
    if not isinstance(bosses, list) or len(bosses) < 3:
        fail("fifth floor must expose at least three final boss encounters")
    if any(int(entry.get("weight", 0)) <= 0 for entry in bosses if isinstance(entry, dict)):
        fail("every final boss encounter must have positive selection weight")


def validate_playable_roster() -> None:
    archetypes = load_json("data/archetypes/playable_archetypes.json")
    available = [entry for entry in archetypes if entry.get("is_available")]
    if len(available) < 7:
        fail(f"expected at least seven available archetypes, found {len(available)}")
    for archetype in available:
        if not archetype.get("starting_deck"):
            fail(f"archetype {archetype.get('id', '<unknown>')} has an empty starting deck")
        if not archetype.get("reward_card_pools"):
            fail(f"archetype {archetype.get('id', '<unknown>')} has no reward card pool")


def main() -> int:
    try:
        validate_config()
        validate_runtime_state_is_ignored()
        validate_main_menu()
        validate_release_configuration()
        validate_finale_variety()
        validate_playable_roster()
    except ContestReadinessError as error:
        print(f"Contest readiness validation failed: {error}", file=sys.stderr)
        return 1

    print("Contest readiness validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
