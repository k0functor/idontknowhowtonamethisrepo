#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import math
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
SEED = 4028


def main() -> int:
    targets = sim.load_balance_targets()
    floor_config = targets["floors"]["floor4"]
    assert floor_config["last_regular_layer"] == 13
    assert floor_config["combat_hp_loss_start_target"] == [0.0, 12.0]
    assert floor_config["elite_hp_loss_start_target"] == [3.0, 20.0]

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

    floor4 = [(pool, encounter) for floor_id, pool, encounter in sim.load_encounters() if floor_id == "floor4"]
    statuses: list[tuple[str, str, float, tuple[float, float]]] = []
    for index, (pool, encounter) in enumerate(floor4):
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
        statuses.append((pool, encounter["id"], hp_loss, target))

    severe = [
        item for item in statuses
        if item[2] > item[3][1] * 1.6 + 1.0
    ]
    assert not severe, "floor4 severe difficulty spikes: " + ", ".join(
        f"{pool}/{encounter_id}={loss:.2f} vs max {target[1]:.2f}"
        for pool, encounter_id, loss, target in severe
    )

    close_to_target = [
        item for item in statuses
        if item[2] >= max(0.0, item[3][0] * 0.5) and item[2] <= item[3][1] * 1.35 + 1.0
    ]
    assert len(close_to_target) >= math.ceil(len(statuses) * 0.88), (
        f"floor4 target coverage regressed: {len(close_to_target)}/{len(statuses)}"
    )

    boss_rows = [item for item in statuses if item[0] == "boss"]
    assert len(boss_rows) == 3
    for _, encounter_id, loss, target in boss_rows:
        assert loss >= target[0] * 0.5, f"floor4 boss {encounter_id} is too soft: {loss:.2f}"
        assert loss <= target[1] * 1.35, f"floor4 boss {encounter_id} is too punishing: {loss:.2f}"

    combat = [encounter for pool, encounter in floor4 if pool == "combat"]
    early_singles = [encounter for encounter in combat if len(encounter.get("enemies", [])) == 1]
    assert early_singles
    assert all(encounter.get("min_layer") == 0 and encounter.get("max_layer") == 0 for encounter in early_singles)

    print("Floor 4 balance contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
