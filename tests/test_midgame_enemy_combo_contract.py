#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENEMY_FILES = [
    ROOT / "data" / "enemies" / "act2_enemies.json",
    ROOT / "data" / "enemies" / "act3_enemies.json",
]
SIMULATION_TOOL = ROOT / "tools" / "simulate_balance.py"


def fixed_amount(effect: dict) -> int:
    value = effect.get("value", {})
    if not isinstance(value, dict) or value.get("type") != "fixed":
        return 0
    amount = value.get("amount", 0)
    return amount if isinstance(amount, int) and not isinstance(amount, bool) else 0


def main() -> int:
    spec = importlib.util.spec_from_file_location("simulate_balance", SIMULATION_TOOL)
    assert spec is not None and spec.loader is not None
    simulation = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(simulation)
    assert simulation.effect_repeat({"repeat_count": 3}) == 3
    assert simulation.effect_repeat({"repeat": 2}) == 2

    checked_followups = 0
    for path in ENEMY_FILES:
        enemies = json.loads(path.read_text(encoding="utf-8"))
        for enemy in enemies:
            produced: dict[str, int] = {}
            required: set[str] = set()
            for action in enemy.get("actions", []):
                conditions = action.get("conditions", {})
                if isinstance(conditions, dict):
                    required.update(conditions.get("required_player_statuses", []))
                for effect in action.get("effects", []):
                    if not isinstance(effect, dict):
                        continue
                    if effect.get("type") != "apply_status":
                        continue
                    if effect.get("target") in {"self", "all_enemies", "random_enemy"}:
                        continue
                    status = effect.get("status")
                    if isinstance(status, str):
                        produced[status] = max(produced.get(status, 0), fixed_amount(effect))
            for status in required:
                assert produced.get(status, 0) >= 2, (
                    f"{enemy['id']} requires player status {status!r} for a follow-up action, "
                    "but does not apply enough stacks for the status to survive the player's turn"
                )
                checked_followups += 1

    assert checked_followups >= 20
    print("Midgame enemy combo contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
