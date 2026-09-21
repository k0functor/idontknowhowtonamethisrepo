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

SAMPLES = 128
TURNS = 5
SEED = 3030


def main() -> int:
    targets = sim.load_balance_targets()
    assert targets["floors"]["floor2"]["combat_hp_loss_target"][0] == 0.0
    assert targets["floors"]["floor3"]["combat_hp_loss_target"][0] == 0.0

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
    base_damage = mean(report["damage_per_turn"] for report in starter_reports)
    base_block = mean(report["block_per_turn"] for report in starter_reports)

    encounters = sim.load_encounters()
    for floor_number in (1, 2, 3):
        floor_id = f"floor{floor_number}"
        floor_config = targets["floors"][floor_id]
        projected_damage = base_damage * float(floor_config["offense_multiplier"])
        projected_block = base_block * float(floor_config["defense_multiplier"])
        rows: list[tuple[str, dict, float, tuple[float, float]]] = []

        floor_encounters = [(pool, encounter) for fid, pool, encounter in encounters if fid == floor_id]
        for index, (pool, encounter) in enumerate(floor_encounters):
            metrics = sim.simulate_encounter(encounter, enemies, SAMPLES, TURNS, SEED + floor_number * 100000 + index * 31337)
            energy_efficiency = max(0.65, 1.0 - 0.12 * metrics["energy_loss_per_turn"])
            encounter_damage = projected_damage * energy_efficiency
            encounter_damage *= max(0.5, 1.0 - metrics["player_offense_penalty"])
            encounter_block = projected_block * energy_efficiency
            defensive_drag = 0.55 * metrics["enemy_block_per_turn"] + 0.7 * metrics["enemy_heal_per_turn"]
            effective_damage = max(encounter_damage * 0.55, encounter_damage - defensive_drag)
            turns_to_kill = metrics["total_hp"] / effective_damage
            hp_loss = max(0.0, metrics["incoming_damage_per_turn"] - encounter_block) * turns_to_kill
            rows.append((pool, encounter, hp_loss, sim.target_range(floor_config, pool, encounter)))

        severe = [row for row in rows if row[2] > row[3][1] * 1.15 + 1.0]
        assert not severe, f"{floor_id} regained severe spikes: " + ", ".join(
            f"{pool}/{encounter['id']}={loss:.2f}" for pool, encounter, loss, _ in severe
        )

        elite_rows = [row for row in rows if row[0] == "elite"]
        boss_rows = [row for row in rows if row[0] == "boss"]
        assert elite_rows and boss_rows
        for pool, encounter, loss, target in elite_rows + boss_rows:
            assert loss >= target[0] * 0.70, f"{floor_id} {pool} {encounter['id']} is too soft: {loss:.2f}"
            assert loss <= target[1] * 1.10 + 1.0, f"{floor_id} {pool} {encounter['id']} is too hard: {loss:.2f}"

        boss_losses = [row[2] for row in boss_rows]
        assert max(boss_losses) - min(boss_losses) <= 15.0, f"{floor_id} boss spread regressed: {boss_losses}"

        combat_rows = [row for row in rows if row[0] == "combat"]
        if floor_number == 2:
            late = [row[2] for row in combat_rows if int(row[1].get("min_layer", 0)) >= 4]
            assert mean(late) >= 4.0, f"floor2 late combat became too soft: {mean(late):.2f}"
            singles = [row[1] for row in combat_rows if len(row[1].get("enemies", [])) == 1]
            assert all(int(encounter.get("max_layer", 0)) <= 4 for encounter in singles)
        elif floor_number == 3:
            late = [row[2] for row in combat_rows if int(row[1].get("min_layer", 0)) >= 4]
            assert mean(late) >= 8.0, f"floor3 late combat became too soft: {mean(late):.2f}"
            singles = [row[1] for row in combat_rows if len(row[1].get("enemies", [])) == 1]
            assert all(int(encounter.get("max_layer", 0)) <= 5 for encounter in singles)

    print("Floors 1-3 balance contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
