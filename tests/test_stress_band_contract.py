from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def load_json(path: str) -> dict[str, str]:
    return json.loads(read(path))


def main() -> None:
    rules = read("src/run/StressRules.hpp")
    psychopath = read("src/run/StressPsychopathRules.hpp")
    player_turn = read("src/combat/PlayerTurnSystem.cpp")
    player_view = read("src/ui/PlayerView.cpp")

    for threshold in (40, 80, 120, 160, 200):
        assert str(threshold) in rules, f"stress threshold {threshold} is missing"

    for band in ("Calm", "Tense", "Pressured", "Panicked", "Breaking", "Collapsed"):
        assert band in rules, f"stress band {band} is missing"

    assert "startTurnEnergyBonus" in psychopath
    assert "startTurnDiscardCount" in psychopath
    assert "gainEnergy(player.id, energyBonus)" in player_turn
    assert "StressBreakdownDiscard" in player_turn
    assert "stressBandColor" in player_view
    assert "thresholdRatio" in player_view

    for locale in ("ru", "en"):
        core = load_json(f"data/localization/{locale}/core.json")
        statuses = load_json(f"data/localization/{locale}/statuses.json")
        archetypes = load_json(f"data/localization/{locale}/archetypes.json")

        for suffix in ("calm", "tense", "pressured", "panicked", "breaking", "collapsed"):
            key = f"ui.stress_band.{suffix}.name"
            assert core.get(key), f"missing {locale} localization key {key}"

        assert "40" in statuses["inspect.player.lost_psychopath_stress_power.value"]
        assert "160" in statuses["inspect.player.lost_psychopath_stress_power.value"]
        assert "40/80/120/160" in archetypes["archetype.lost_psychopath.strength.stress_scaling"]

    print("Stress band contract passed")


if __name__ == "__main__":
    main()
