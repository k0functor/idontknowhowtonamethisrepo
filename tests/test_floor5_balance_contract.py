#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
from statistics import mean

ROOT = Path(__file__).resolve().parents[1]
SIM_PATH = ROOT / "tools" / "simulate_balance.py"

spec = importlib.util.spec_from_file_location("simulate_balance", SIM_PATH)
assert spec is not None and spec.loader is not None
sim = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sim)

SAMPLES = 96
TURNS = 5
SEED = 5029


def main() -> int:
    targets = sim.load_balance_targets()
    floor_config = targets["floors"]["floor5"]
    assert floor_config["last_regular_layer"] == 17
    assert floor_config["combat_hp_loss_start_target"] == [0.0, 12.0]
    assert floor_config["elite_hp_loss_start_target"] == [2.0, 24.0]

    cards = {item["id"]: item for item in sim.load_list("cards") if isinstance(item.get("id"), str)}
    enemies = {item["id"]: item for item in sim.load_list("enemies") if isinstance(item.get("id"), str)}
    relics = {item["id"]: item for item in sim.load_list("relics") if isinstance(item.get("id"), str)}
    drones = {item["id"]: item for item in sim.load_list("drones") if isinstance(item.get("id"), str)}
    archetypes = json.loads((ROOT / "data/archetypes/playable_archetypes.json").read_text(encoding="utf-8-sig"))

    starter_reports = []
    for index, archetype in enumerate(archetypes):
        if not archetype.get("is_available", True) or len(archetype.get("actors", [])) != 1:
            continue
        starter_reports.append(
            sim.simulate_starter_deck(
                archetype,
                cards,
                relics,
                drones,
                SAMPLES,
                TURNS,
                SEED + index * 100003,
            )
        )
    player_damage = mean(report["damage_per_turn"] for report in starter_reports)
    player_block = mean(report["block_per_turn"] for report in starter_reports)
    projected_damage = player_damage * float(floor_config["offense_multiplier"])
    projected_block = player_block * float(floor_config["defense_multiplier"])

    floor5 = [(pool, encounter) for floor_id, pool, encounter in sim.load_encounters() if floor_id == "floor5"]
    statuses: list[tuple[str, str, float, tuple[float, float], int]] = []
    for index, (pool, encounter) in enumerate(floor5):
        metrics = sim.simulate_encounter(encounter, enemies, SAMPLES, TURNS, SEED + index * 31337)
        energy_efficiency = max(0.65, 1.0 - 0.12 * metrics["energy_loss_per_turn"])
        encounter_damage = projected_damage * energy_efficiency
        encounter_damage *= max(0.5, 1.0 - metrics["player_offense_penalty"])
        encounter_block = projected_block * energy_efficiency
        defensive_drag = 0.55 * metrics["enemy_block_per_turn"] + 0.7 * metrics["enemy_heal_per_turn"]
        effective_damage = max(encounter_damage * 0.55, encounter_damage - defensive_drag)
        turns_to_kill = metrics["total_hp"] / effective_damage
        hp_loss = max(0.0, metrics["incoming_damage_per_turn"] - encounter_block) * turns_to_kill
        target = sim.target_range(floor_config, pool, encounter)
        statuses.append((pool, encounter["id"], hp_loss, target, int(encounter.get("min_layer", 0))))

    severe = [item for item in statuses if item[2] > item[3][1] * 1.25 + 1.0]
    assert not severe, "floor5 severe difficulty spikes: " + ", ".join(
        f"{pool}/{encounter_id}={loss:.2f} vs max {target[1]:.2f}"
        for pool, encounter_id, loss, target, _ in severe
    )

    combat = [item for item in statuses if item[0] == "combat"]
    elite = [item for item in statuses if item[0] == "elite"]
    boss = [item for item in statuses if item[0] == "boss"]
    assert len(combat) == 32
    assert len(elite) == 14
    assert len(boss) == 3

    within_combat = [item for item in combat if item[2] <= item[3][1] + 1.0]
    within_elite = [item for item in elite if item[2] >= item[3][0] * 0.5 and item[2] <= item[3][1] + 1.0]
    assert len(within_combat) >= 31, f"floor5 combat coverage regressed: {len(within_combat)}/32"
    assert len(within_elite) >= 13, f"floor5 elite coverage regressed: {len(within_elite)}/14"

    late_combat = [item[2] for item in combat if item[4] >= 8]
    late_elite = [item[2] for item in elite if item[4] >= 8]
    assert mean(late_combat) >= 9.0, f"floor5 late combat became too soft: {mean(late_combat):.2f}"
    assert mean(late_elite) >= 14.0, f"floor5 late elite became too soft: {mean(late_elite):.2f}"

    boss_losses = []
    for _, boss_id, boss_loss, boss_target, _ in boss:
        assert boss_loss >= boss_target[0] * 0.75, f"floor5 boss {boss_id} is too soft: {boss_loss:.2f}"
        assert boss_loss <= boss_target[1], f"floor5 boss {boss_id} is too punishing: {boss_loss:.2f}"
        boss_losses.append(boss_loss)
    assert max(boss_losses) - min(boss_losses) <= 25.0, f"floor5 boss spread is too wide: {boss_losses}"

    boss_def = enemies["the_oath_buried_king"]
    phases = {phase["id"]: phase for phase in boss_def.get("phases", [])}
    assert "the_oath_buried_king_tax_the_living" in phases["sealed_court"]["action_ids"]
    assert len(phases["opened_tomb"].get("summon_enemy_ids", [])) == 2
    assert len(phases["last_oath"].get("summon_enemy_ids", [])) == 2

    print("Floor 5 balance contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
