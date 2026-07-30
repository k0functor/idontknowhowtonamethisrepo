#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGETS = ROOT / "data" / "balance" / "progression_targets.json"
ARCHETYPES = ROOT / "data" / "archetypes" / "playable_archetypes.json"
ENCOUNTER_DIR = ROOT / "data" / "encounters"


def main() -> int:
    targets = json.loads(TARGETS.read_text(encoding="utf-8"))
    assert targets["schema_version"] == 1
    assert set(targets["floors"]) == {f"floor{i}" for i in range(1, 6)}
    previous_offense = 0.0
    previous_defense = 0.0
    for floor_id in sorted(targets["floors"]):
        floor = targets["floors"][floor_id]
        assert floor["offense_multiplier"] >= previous_offense
        assert floor["defense_multiplier"] >= previous_defense
        previous_offense = floor["offense_multiplier"]
        previous_defense = floor["defense_multiplier"]
        for pool in ("combat", "elite", "boss"):
            low, high = floor[f"{pool}_hp_loss_target"]
            assert 0 <= low < high

    archetypes = json.loads(ARCHETYPES.read_text(encoding="utf-8"))
    decks = {item["id"]: item["starting_deck"] for item in archetypes}
    assert "lost_psychopath_clean_cut" not in decks["lost_psychopath"], (
        "a fresh psychopath run must not start with a stress-spending card that is dead on turn one"
    )
    assert decks["herbalist"].count("herbalist_scalpel") == 1
    assert decks["herbalist"].count("herbalist_basic_defend") == 3
    assert decks["sadist_masochist"].count("masochist_endure") == 1

    tuned = {
        "act4_lens_hound_pack": (2, 6),
        "act4_final_calculation_hall": (1, 11),
        "act5_last_court_patrol": (1, 8),
        "act5_headsman_court": (1, 13),
    }
    found: dict[str, tuple[int, int]] = {}
    for path in ENCOUNTER_DIR.glob("*.json"):
        root = json.loads(path.read_text(encoding="utf-8"))
        for encounters in root["pools"].values():
            for encounter in encounters:
                if encounter["id"] in tuned:
                    found[encounter["id"]] = (encounter["weight"], encounter["min_layer"])
    assert found == tuned

    print("Balance targets contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
