#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENEMY_DIR = ROOT / "data" / "enemies"
STATUS_DIR = ROOT / "data" / "statuses"

ALLOWED_CONDITIONS = {
    "min_turn",
    "max_turn",
    "self_hp_below_percent",
    "self_hp_above_percent",
    "any_player_hp_below_percent",
    "any_player_hp_above_percent",
    "any_other_enemy_hp_below_percent",
    "min_alive_enemies",
    "max_alive_enemies",
    "required_self_statuses",
    "forbidden_self_statuses",
    "required_player_statuses",
    "forbidden_player_statuses",
}
PERCENT_FIELDS = {
    "self_hp_below_percent",
    "self_hp_above_percent",
    "any_player_hp_below_percent",
    "any_player_hp_above_percent",
    "any_other_enemy_hp_below_percent",
}
STATUS_FIELDS = {
    "required_self_statuses",
    "forbidden_self_statuses",
    "required_player_statuses",
    "forbidden_player_statuses",
}


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def load_status_ids() -> set[str]:
    result: set[str] = set()
    for path in sorted(STATUS_DIR.glob("*.json")):
        for status in load_json(path):
            result.add(status["id"])
    return result


def baseline_eligible(conditions: dict) -> bool:
    return (
        conditions.get("min_turn", 1) <= 1
        and conditions.get("min_alive_enemies", 1) <= 1
        and "self_hp_below_percent" not in conditions
        and "any_player_hp_below_percent" not in conditions
        and "any_other_enemy_hp_below_percent" not in conditions
        and not conditions.get("required_self_statuses")
        and not conditions.get("required_player_statuses")
    )


def main() -> int:
    errors: list[str] = []
    statuses = load_status_ids()

    action_count = 0
    conditional_count = 0
    cooldown_count = 0
    team_condition_count = 0
    status_condition_count = 0

    for path in sorted(ENEMY_DIR.glob("*.json")):
        for enemy in load_json(path):
            actions = enemy.get("actions", [])
            if not actions:
                errors.append(f"{enemy.get('id', '<unknown>')}: no actions")
                continue

            has_baseline = False
            for action in actions:
                owner = f"{enemy['id']}.{action.get('id', '<unknown>')}"
                action_count += 1

                for field in ("weight", "cooldown", "max_consecutive_uses"):
                    if field not in action:
                        errors.append(f"{owner}: missing {field}")

                weight = action.get("weight")
                cooldown = action.get("cooldown")
                max_consecutive = action.get("max_consecutive_uses")
                if not isinstance(weight, int) or weight < 1:
                    errors.append(f"{owner}: weight must be a positive integer")
                if not isinstance(cooldown, int) or cooldown < 0:
                    errors.append(f"{owner}: cooldown must be a non-negative integer")
                if not isinstance(max_consecutive, int) or max_consecutive < 0:
                    errors.append(f"{owner}: max_consecutive_uses must be non-negative")

                if isinstance(cooldown, int) and cooldown > 0:
                    cooldown_count += 1

                conditions = action.get("conditions", {})
                if not isinstance(conditions, dict):
                    errors.append(f"{owner}: conditions must be an object")
                    continue
                unknown = sorted(set(conditions) - ALLOWED_CONDITIONS)
                if unknown:
                    errors.append(f"{owner}: unknown condition fields {unknown}")

                if conditions:
                    conditional_count += 1
                if conditions.get("min_alive_enemies", 1) > 1:
                    team_condition_count += 1
                if any(conditions.get(field) for field in STATUS_FIELDS):
                    status_condition_count += 1

                min_turn = conditions.get("min_turn", 1)
                max_turn = conditions.get("max_turn")
                min_alive = conditions.get("min_alive_enemies", 1)
                max_alive = conditions.get("max_alive_enemies")
                if not isinstance(min_turn, int) or min_turn < 1:
                    errors.append(f"{owner}: min_turn must be >= 1")
                if max_turn is not None and (
                    not isinstance(max_turn, int) or max_turn < min_turn
                ):
                    errors.append(f"{owner}: max_turn must be >= min_turn")
                if not isinstance(min_alive, int) or min_alive < 1:
                    errors.append(f"{owner}: min_alive_enemies must be >= 1")
                if max_alive is not None and (
                    not isinstance(max_alive, int) or max_alive < min_alive
                ):
                    errors.append(f"{owner}: max_alive_enemies must be >= min_alive_enemies")

                for field in PERCENT_FIELDS:
                    if field not in conditions:
                        continue
                    value = conditions[field]
                    if not isinstance(value, int) or not 1 <= value <= 100:
                        errors.append(f"{owner}: {field} must be in [1, 100]")

                for field in STATUS_FIELDS:
                    values = conditions.get(field, [])
                    if not isinstance(values, list) or not all(
                        isinstance(value, str) and value for value in values
                    ):
                        errors.append(f"{owner}: {field} must be a list of non-empty strings")
                        continue
                    for status_id in values:
                        if status_id not in statuses:
                            errors.append(f"{owner}: {field} references unknown status {status_id}")

                required_self = set(conditions.get("required_self_statuses", []))
                forbidden_self = set(conditions.get("forbidden_self_statuses", []))
                required_player = set(conditions.get("required_player_statuses", []))
                forbidden_player = set(conditions.get("forbidden_player_statuses", []))
                if required_self & forbidden_self:
                    errors.append(f"{owner}: self status is both required and forbidden")
                if required_player & forbidden_player:
                    errors.append(f"{owner}: player status is both required and forbidden")

                has_baseline = has_baseline or baseline_eligible(conditions)

            if not has_baseline:
                errors.append(
                    f"{enemy['id']}: no action is naturally eligible on turn 1 in a solo encounter"
                )

    if action_count < 250:
        errors.append(f"expected a substantial enemy action catalogue, found {action_count}")
    if conditional_count < 100:
        errors.append(f"too few conditional actions: {conditional_count}")
    if cooldown_count < 100:
        errors.append(f"too few actions with cooldowns: {cooldown_count}")
    if team_condition_count < 10:
        errors.append(f"too few group-aware actions: {team_condition_count}")
    if status_condition_count < 20:
        errors.append(f"too few status-reactive actions: {status_condition_count}")

    if errors:
        for error in errors:
            print(f"[enemy-ai] {error}")
        return 1

    print(
        "Enemy AI contract passed: "
        f"{action_count} actions, {conditional_count} conditional, "
        f"{cooldown_count} with cooldowns, {team_condition_count} group-aware, "
        f"{status_condition_count} status-reactive"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
