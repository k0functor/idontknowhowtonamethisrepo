#!/usr/bin/env python3
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
from statistics import mean
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
FLOORS_PATH = DATA_DIR / "run" / "floors.json"


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def effect_amount(effect: dict[str, Any]) -> int:
    value = effect.get("value")
    if isinstance(value, dict):
        amount = value.get("amount")
        if isinstance(amount, int) and not isinstance(amount, bool):
            return amount
    return 0


def load_enemies() -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for path in sorted((DATA_DIR / "enemies").glob("*.json")):
        root = load_json(path)
        if not isinstance(root, list):
            continue
        for enemy in root:
            if isinstance(enemy, dict) and isinstance(enemy.get("id"), str):
                result[enemy["id"]] = enemy
    return result


def enemy_pressure(enemy: dict[str, Any]) -> dict[str, float]:
    damage: list[int] = []
    block: list[int] = []
    status_counts: Counter[str] = Counter()
    energy_loss = 0
    heal: list[int] = []

    for action in enemy.get("actions", []):
        if not isinstance(action, dict):
            continue
        for effect in action.get("effects", []):
            if not isinstance(effect, dict):
                continue
            effect_type = effect.get("type")
            amount = effect_amount(effect)
            if effect_type == "damage":
                damage.append(amount)
            elif effect_type == "block":
                block.append(amount)
            elif effect_type == "heal":
                heal.append(amount)
            elif effect_type == "lose_energy":
                energy_loss += amount
            elif effect_type == "apply_status":
                status = effect.get("status")
                if isinstance(status, str) and status:
                    status_counts[status] += amount

    return {
        "hp": float(enemy.get("max_hp", 0) if isinstance(enemy.get("max_hp"), int) else 0),
        "avg_damage": float(mean(damage) if damage else 0.0),
        "max_damage": float(max(damage) if damage else 0.0),
        "avg_block": float(mean(block) if block else 0.0),
        "avg_heal": float(mean(heal) if heal else 0.0),
        "poison": float(status_counts["poison"]),
        "weak": float(status_counts["weak"]),
        "vulnerable": float(status_counts["vulnerable"]),
        "strength": float(status_counts["strength"]),
        "dexterity": float(status_counts["dexterity"]),
        "energy_loss": float(energy_loss),
    }


def encounter_pressure(encounter: dict[str, Any], enemies: dict[str, dict[str, Any]]) -> dict[str, float]:
    total: Counter[str] = Counter()
    for enemy_id in encounter.get("enemies", []):
        enemy = enemies.get(enemy_id)
        if not isinstance(enemy, dict):
            continue
        pressure = enemy_pressure(enemy)
        for key, value in pressure.items():
            total[key] += value
    return {key: float(total[key]) for key in total}


def floor_path(floor: dict[str, Any], raw: str) -> Path:
    return (DATA_DIR / "run" / raw).resolve()


def format_avg(values: list[float]) -> str:
    return f"{mean(values):.1f}" if values else "0.0"


def print_pool_report(pool_name: str, encounters: list[dict[str, Any]], enemies: dict[str, dict[str, Any]]) -> None:
    hp_values: list[float] = []
    damage_values: list[float] = []
    max_damage_values: list[float] = []
    weighted_hp = 0.0
    weighted_damage = 0.0
    total_weight = 0
    effect_counts: Counter[str] = Counter()

    for encounter in encounters:
        pressure = encounter_pressure(encounter, enemies)
        weight = encounter.get("weight", 1)
        if not isinstance(weight, int) or isinstance(weight, bool) or weight <= 0:
            weight = 1
        hp = pressure.get("hp", 0.0)
        damage = pressure.get("avg_damage", 0.0)
        max_damage = pressure.get("max_damage", 0.0)
        hp_values.append(hp)
        damage_values.append(damage)
        max_damage_values.append(max_damage)
        weighted_hp += hp * weight
        weighted_damage += damage * weight
        total_weight += weight
        for key in ("poison", "weak", "vulnerable", "energy_loss", "strength", "dexterity"):
            if pressure.get(key, 0.0) > 0:
                effect_counts[key] += 1

    weighted_hp_text = f"{weighted_hp / total_weight:.1f}" if total_weight else "0.0"
    weighted_damage_text = f"{weighted_damage / total_weight:.1f}" if total_weight else "0.0"
    print(f"  {pool_name}: {len(encounters)} encounter(s)")
    print(f"    avg total HP: {format_avg(hp_values)} (weighted {weighted_hp_text})")
    print(f"    avg action damage: {format_avg(damage_values)} (weighted {weighted_damage_text})")
    print(f"    avg max hit: {format_avg(max_damage_values)}")
    if effect_counts:
        effects = ", ".join(f"{key}={effect_counts[key]}" for key in sorted(effect_counts))
        print(f"    effect coverage: {effects}")


def main() -> int:
    floors_root = load_json(FLOORS_PATH)
    floors = floors_root.get("floors", []) if isinstance(floors_root, dict) else []
    enemies = load_enemies()

    print("Floor balance report")
    for floor in floors:
        if not isinstance(floor, dict) or not floor.get("is_implemented", True):
            continue
        floor_id = floor.get("id", "<unknown>")
        print(f"\n{floor_id} ({floor.get('name_text_id', '<no name>')})")
        encounter_path_raw = floor.get("encounter_table_path")
        if not isinstance(encounter_path_raw, str) or not encounter_path_raw:
            print("  no encounter table")
            continue
        encounter_path = floor_path(floor, encounter_path_raw)
        table = load_json(encounter_path)
        pools = table.get("pools", {}) if isinstance(table, dict) else {}
        for pool_name in ("combat", "elite", "boss"):
            encounters = pools.get(pool_name, [])
            if isinstance(encounters, list):
                print_pool_report(pool_name, encounters, enemies)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
