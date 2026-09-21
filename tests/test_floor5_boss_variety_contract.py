#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    enemies = {item["id"]: item for item in json.loads((ROOT / "data/enemies/act5_enemies.json").read_text(encoding="utf-8-sig"))}
    pools = json.loads((ROOT / "data/encounters/act5_encounters.json").read_text(encoding="utf-8-sig"))["pools"]
    boss_encounters = pools["boss"]

    expected = {"the_oath_buried_king", "the_last_regent", "choir_of_chains"}
    assert len(boss_encounters) == 3
    assert {encounter["enemies"][0] for encounter in boss_encounters} == expected
    assert all(encounter.get("weight") == 1 for encounter in boss_encounters)

    for enemy_id in expected:
        boss = enemies[enemy_id]
        assert boss.get("role") == "boss"
        assert len(boss.get("phases", [])) == 3
        assert boss.get("max_hp", 0) >= 300

    king = enemies["the_oath_buried_king"]
    king_phases = king["phases"]
    assert sum(len(phase.get("summon_enemy_ids", [])) for phase in king_phases) >= 4

    regent = enemies["the_last_regent"]
    regent_phases = {phase["id"]: phase for phase in regent["phases"]}
    assert not any(phase.get("summon_enemy_ids") for phase in regent["phases"]), "Regent should be the solo scaling final boss"
    assert any(
        effect.get("status") == "strength"
        for phase in regent["phases"]
        for effect in phase.get("on_enter_effects", [])
    )
    assert regent_phases["no_successor"].get("player_turn_effects"), "Regent final phase must keep time pressure"

    choir = enemies["choir_of_chains"]
    choir_actions = {action["id"]: action for action in choir["actions"]}
    refrain = choir_actions["choir_of_chains_iron_refrain"]
    damage = next(effect for effect in refrain["effects"] if effect.get("type") == "damage")
    assert damage.get("repeat_count", 1) >= 3
    choir_phases = {phase["id"]: phase for phase in choir["phases"]}
    assert any(effect.get("type") == "lose_energy" for effect in choir_phases["iron_refrain"].get("player_turn_effects", []))
    assert not any(effect.get("type") == "lose_energy" for effect in choir_phases["final_cadence"].get("player_turn_effects", []))
    assert any(effect.get("type") == "gain_stress" for effect in choir_phases["final_cadence"].get("player_turn_effects", []))

    en = json.loads((ROOT / "data/localization/en/enemies.json").read_text(encoding="utf-8-sig"))
    ru = json.loads((ROOT / "data/localization/ru/enemies.json").read_text(encoding="utf-8-sig"))
    for key in (
        "enemy.the_last_regent.name",
        "enemy.choir_of_chains.name",
        "enemy.phase.the_last_regent.no_successor",
        "enemy.phase.choir_of_chains.final_cadence",
    ):
        assert key in en and key in ru

    print("Floor 5 boss variety contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
