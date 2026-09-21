#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, *needles: str) -> str:
    text = (ROOT / path).read_text(encoding="utf-8")
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError(f"{path} is missing: {', '.join(missing)}")
    return text


def main() -> int:
    theme = require(
        "src/ui/UiTheme.hpp",
        "enum class Tone",
        "toneFill",
        "toneBorder",
        "statusFill",
        "statusBorder",
        "intentFill",
        "intentBorder",
        "stressBand",
    )
    if theme.count("inline constexpr Color") < 8:
        raise AssertionError("UiTheme must own the shared neutral palette")

    require(
        "src/ui/BasicUi.hpp",
        "ButtonStyle buttonStyle(UiTheme::Tone tone)",
        "void drawPanel(",
        "void drawChip(",
        "void drawProgressBar(",
    )
    require(
        "src/ui/BasicUi.cpp",
        "UiTheme::toneFill",
        "UiTheme::controlRoundness",
        "UiTheme::borderThickness",
    )

    for path in (
        "src/ui/CombatView.cpp",
        "src/ui/EnemyView.cpp",
        "src/ui/PlayerView.cpp",
    ):
        require(path, '#include "ui/UiTheme.hpp"')

    require(
        "src/ui/CombatViewModel.hpp",
        "incomingDamageMin",
        "incomingDamageMax",
        "attackingEnemyCount",
        "partyWideThreatCount",
        "incomingDamageLabel",
        "partyWideThreatLabel",
    )
    require("src/ui/EnemyViewModel.hpp", "intentDangerLevel")
    require(
        "src/ui/CombatViewModelBuilder.cpp",
        "intentDangerLevel(",
        "intent.hitCount",
        "ui.threat.incoming_damage",
        "ui.threat.no_attacks",
        "ui.threat.party_wide",
    )
    require(
        "src/ui/CombatView.cpp",
        "renderThreatSummary",
        "UiTheme::Tone::Danger",
        "UiTheme::Tone::Warning",
    )
    require(
        "src/ui/EnemyView.cpp",
        "model_.intentDangerLevel",
        "intentBorderThickness",
        "dangerStrip",
    )

    localization_keys = {
        "ui.threat.incoming_damage",
        "ui.threat.no_attacks",
        "ui.threat.party_wide",
    }
    for locale in ("en", "ru"):
        data = json.loads((ROOT / f"data/localization/{locale}/core.json").read_text(encoding="utf-8"))
        missing = sorted(localization_keys.difference(data))
        if missing:
            raise AssertionError(f"{locale} localization is missing: {', '.join(missing)}")

    cmake = require(
        "CMakeLists.txt",
        "src/ui/UiTheme.hpp",
        "test_ui_theme_and_combat_feedback_contract.py",
    )
    if cmake.count("ui_theme_and_combat_feedback_contract") < 1:
        raise AssertionError("The UI contract must be registered with CTest")

    print("UI theme and combat feedback contract validated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
