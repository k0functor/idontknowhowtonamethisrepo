#!/usr/bin/env python3
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALID_ROLES = {"striker", "defender", "support", "controller", "bruiser", "minion", "boss"}
TEAM_EFFECT_TARGETS = {"all_enemies", "random_enemy"}
TEAM_EFFECT_TYPES = {"block", "heal", "apply_status"}


def load_enemies() -> dict[str, dict]:
    result: dict[str, dict] = {}
    for path in sorted((ROOT / "data" / "enemies").glob("*.json")):
        for enemy in json.loads(path.read_text(encoding="utf-8")):
            result[enemy["id"]] = enemy
    return result


def has_team_action(enemy: dict) -> bool:
    for action in enemy.get("actions", []):
        for effect in action.get("effects", []):
            if effect.get("target") in TEAM_EFFECT_TARGETS and effect.get("type") in TEAM_EFFECT_TYPES:
                return True
    return False


def main() -> int:
    errors: list[str] = []
    enemies = load_enemies()
    roles = Counter()

    for enemy_id, enemy in sorted(enemies.items()):
        role = enemy.get("role")
        if role not in VALID_ROLES:
            errors.append(f"enemy '{enemy_id}' has missing or invalid role {role!r}")
            continue
        roles[role] += 1

        if role == "boss" and enemy_id == "training_dummy":
            errors.append("training dummy must not be classified as a boss")

    for required_role in ("striker", "defender", "support", "controller", "bruiser", "minion", "boss"):
        if roles[required_role] == 0:
            errors.append(f"enemy roster has no '{required_role}' role")

    team_actors = [enemy_id for enemy_id, enemy in enemies.items() if has_team_action(enemy)]
    if len(team_actors) < 15:
        errors.append(f"only {len(team_actors)} enemies have team-oriented actions; expected at least 15")

    for encounter_path in sorted((ROOT / "data" / "encounters").glob("act*_encounters.json")):
        root = json.loads(encounter_path.read_text(encoding="utf-8"))
        combat_groups = []
        mixed_groups = []
        trio_groups = []
        support_groups = []

        for encounter in root.get("pools", {}).get("combat", []):
            ids = encounter.get("enemies", [])
            if len(ids) <= 1:
                continue
            combat_groups.append(encounter["id"])
            group_roles = {enemies[enemy_id]["role"] for enemy_id in ids if enemy_id in enemies}
            if len(group_roles) >= 2:
                mixed_groups.append(encounter["id"])
            if len(ids) == 3:
                trio_groups.append(encounter["id"])
            if any(has_team_action(enemies[enemy_id]) for enemy_id in ids if enemy_id in enemies):
                support_groups.append(encounter["id"])

        owner = encounter_path.relative_to(ROOT)
        if len(combat_groups) < 5:
            errors.append(f"{owner}: expected at least five multi-enemy combat encounters")
        if not mixed_groups:
            errors.append(f"{owner}: no mixed-role combat encounter")
        if not trio_groups:
            errors.append(f"{owner}: no three-enemy combat encounter")
        if not support_groups:
            errors.append(f"{owner}: no group contains an enemy with a team-oriented action")

    rewards = json.loads((ROOT / "data" / "rewards" / "reward_tables.json").read_text(encoding="utf-8"))
    group_bonus = rewards.get("group_gold_bonus_percent_per_extra_enemy")
    if not isinstance(group_bonus, int) or not (1 <= group_bonus <= 100):
        errors.append("reward_tables.json must define group_gold_bonus_percent_per_extra_enemy in [1, 100]")

    if errors:
        print("Group encounter depth test failed:")
        for error in errors:
            print(f" - {error}")
        return 1

    print(
        "Group encounter depth test passed: "
        f"{len(enemies)} enemies, {len(team_actors)} team actors, roles={dict(sorted(roles.items()))}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
