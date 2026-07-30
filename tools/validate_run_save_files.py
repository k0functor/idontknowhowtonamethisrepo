#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
SAVES_DIR = ROOT / "saves"
DATA_DIR = ROOT / "data"

PROFILE_PROGRESS_TYPES = {
    "archetype_unlocked": "archetype",
    "card_unlocked": "card",
    "relic_unlocked": "relic",
    "enemy_discovered": "enemy",
    "status_discovered": "status",
    "consumable_discovered": "consumable",
    "challenge_completed": "challenge",
    "achievement_completed": "achievement",
}

NODE_TYPE_TO_PENDING_TYPES = {
    "combat": {"combat_reward"},
    "elite": {"combat_reward"},
    "boss": {"combat_reward"},
    "chest": {"chest_reward"},
    "shop": {"shop"},
    "rest": {"merchant_rest"},
    "event": {"event"},
}


def error(path: Path, message: str) -> str:
    return f"{path.relative_to(ROOT)}: {message}"


def require_type(errors: list[str], path: Path, owner: str, value: Any, expected: type | tuple[type, ...]) -> bool:
    if not isinstance(value, expected):
        if isinstance(expected, tuple):
            expected_name = "/".join(t.__name__ for t in expected)
        else:
            expected_name = expected.__name__
        errors.append(error(path, f"{owner} must be {expected_name}"))
        return False
    return True


def string_list(errors: list[str], path: Path, owner: str, value: Any) -> list[str]:
    if not require_type(errors, path, owner, value, list):
        return []
    result: list[str] = []
    for index, item in enumerate(value):
        if not isinstance(item, str):
            errors.append(error(path, f"{owner}[{index}] must be string"))
            continue
        result.append(item)
    return result



def count_list(errors: list[str], path: Path, owner: str, value: Any, known_ids: set[str]) -> None:
    if not require_type(errors, path, owner, value, list):
        return
    seen: set[str] = set()
    for index, item in enumerate(value):
        if not require_type(errors, path, f"{owner}[{index}]", item, dict):
            continue
        content_id = item.get("content_id")
        count = item.get("count")
        if not isinstance(content_id, str) or not content_id:
            errors.append(error(path, f"{owner}[{index}].content_id must be a non-empty string"))
            continue
        if content_id in seen:
            errors.append(error(path, f"{owner} contains duplicate id '{content_id}'"))
        seen.add(content_id)
        if known_ids and content_id not in known_ids:
            errors.append(error(path, f"{owner}[{index}] references unknown id '{content_id}'"))
        if not isinstance(count, int) or isinstance(count, bool) or count <= 0:
            errors.append(error(path, f"{owner}[{index}].count must be a positive integer"))

def load_id_set(relative_path: str) -> set[str]:
    path = ROOT / relative_path
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception:  # noqa: BLE001 - save validation should report save errors, not duplicate project validation.
        return set()
    if not isinstance(data, list):
        return set()
    result: set[str] = set()
    for item in data:
        if isinstance(item, dict) and isinstance(item.get("id"), str):
            result.add(item["id"])
    return result


def load_ids_from_directory(directory_name: str) -> set[str]:
    result: set[str] = set()
    content_dir = DATA_DIR / directory_name
    if not content_dir.exists():
        return result
    for path in sorted(content_dir.glob("*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except Exception:  # noqa: BLE001
            continue
        if not isinstance(data, list):
            continue
        for item in data:
            if isinstance(item, dict) and isinstance(item.get("id"), str):
                result.add(item["id"])
    return result


def load_card_ids() -> set[str]:
    result: set[str] = set()
    for path in sorted((DATA_DIR / "cards").glob("*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except Exception:  # noqa: BLE001
            continue
        if not isinstance(data, list):
            continue
        for item in data:
            if isinstance(item, dict) and isinstance(item.get("id"), str):
                result.add(item["id"])
    return result


def validate_known_ids(
    errors: list[str],
    path: Path,
    owner: str,
    ids: list[str],
    known_ids: set[str],
) -> None:
    if not known_ids:
        return
    for id_value in ids:
        if id_value not in known_ids:
            errors.append(error(path, f"{owner} references unknown id '{id_value}'"))


def validate_profile_save(
    path: Path,
    card_ids: set[str],
    relic_ids: set[str],
    archetype_ids: set[str],
    challenge_ids: set[str],
    achievement_ids: set[str],
    enemy_ids: set[str],
    status_ids: set[str],
    consumable_ids: set[str],
) -> list[str]:
    errors: list[str] = []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        return [error(path, f"cannot read JSON: {exc}")]

    if not require_type(errors, path, "profile root", data, dict):
        return errors
    if data.get("version") not in {1, 2, 3, 4, 5, 6, 7}:
        errors.append(error(path, "version must be 1, 2, 3, 4, 5, 6 or 7"))

    slot_name = path.parent.name
    expected_slot_index = None
    if slot_name.startswith("slot_"):
        try:
            expected_slot_index = int(slot_name.removeprefix("slot_")) - 1
        except ValueError:
            expected_slot_index = None

    slot_index = data.get("slot_index")
    if not isinstance(slot_index, int) or isinstance(slot_index, bool) or slot_index < 0:
        errors.append(error(path, "slot_index must be a non-negative integer"))
    elif expected_slot_index is not None and slot_index != expected_slot_index:
        errors.append(error(path, f"slot_index must match directory index {expected_slot_index}"))

    if not isinstance(data.get("is_empty", False), bool):
        errors.append(error(path, "is_empty must be boolean"))
    for counter_name in ("victories", "defeats"):
        value = data.get(counter_name, 0)
        if not isinstance(value, int) or isinstance(value, bool) or value < 0:
            errors.append(error(path, f"{counter_name} must be a non-negative integer"))

    unlocked_cards = string_list(errors, path, "unlocked_cards", data.get("unlocked_cards", []))
    unlocked_relics = string_list(errors, path, "unlocked_relics", data.get("unlocked_relics", []))
    unlocked_archetypes = string_list(errors, path, "unlocked_archetypes", data.get("unlocked_archetypes", []))
    discovered_enemies = string_list(errors, path, "discovered_enemies", data.get("discovered_enemies", []))
    discovered_statuses = string_list(errors, path, "discovered_statuses", data.get("discovered_statuses", []))
    discovered_consumables = string_list(errors, path, "discovered_consumables", data.get("discovered_consumables", []))
    completed_challenges = string_list(errors, path, "completed_challenges", data.get("completed_challenges", []))
    completed_achievements = string_list(errors, path, "completed_achievements", data.get("completed_achievements", []))

    if not data.get("is_empty", False) and not unlocked_archetypes:
        errors.append(error(path, "non-empty profile must unlock at least one archetype"))

    for owner, values in (
        ("unlocked_cards", unlocked_cards),
        ("unlocked_relics", unlocked_relics),
        ("unlocked_archetypes", unlocked_archetypes),
        ("discovered_enemies", discovered_enemies),
        ("discovered_statuses", discovered_statuses),
        ("discovered_consumables", discovered_consumables),
        ("completed_challenges", completed_challenges),
        ("completed_achievements", completed_achievements),
    ):
        duplicates = sorted({value for value in values if values.count(value) > 1})
        for value in duplicates:
            errors.append(error(path, f"{owner} contains duplicate id '{value}'"))

    validate_known_ids(errors, path, "unlocked_cards", unlocked_cards, card_ids)
    validate_known_ids(errors, path, "unlocked_relics", unlocked_relics, relic_ids)
    validate_known_ids(errors, path, "unlocked_archetypes", unlocked_archetypes, archetype_ids)
    validate_known_ids(errors, path, "discovered_enemies", discovered_enemies, enemy_ids)
    validate_known_ids(errors, path, "discovered_statuses", discovered_statuses, status_ids)
    validate_known_ids(errors, path, "discovered_consumables", discovered_consumables, consumable_ids)
    validate_known_ids(errors, path, "completed_challenges", completed_challenges, challenge_ids)
    validate_known_ids(errors, path, "completed_achievements", completed_achievements, achievement_ids)

    count_list(errors, path, "card_play_counts", data.get("card_play_counts", []), card_ids)
    count_list(errors, path, "relic_pick_counts", data.get("relic_pick_counts", []), relic_ids)


    lifetime_run_stats = data.get("lifetime_run_stats", {})
    if require_type(errors, path, "lifetime_run_stats", lifetime_run_stats, dict):
        for counter_name in (
            "combats_won",
            "combats_lost",
            "enemies_killed",
            "elites_killed",
            "bosses_killed",
            "events_completed",
            "shops_visited",
            "chests_opened",
            "rests_used",
            "damage_taken",
            "consumables_used",
            "gold_gained",
            "gold_spent",
            "cards_added",
            "cards_removed",
            "cards_upgraded",
            "cards_skipped",
            "rewards_skipped",
            "relics_gained",
            "consumables_gained",
            "nodes_completed",
        ):
            value = lifetime_run_stats.get(counter_name, 0)
            if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                errors.append(error(path, f"lifetime_run_stats.{counter_name} must be a non-negative integer"))

    progress_log = data.get("progress_log", [])
    if require_type(errors, path, "progress_log", progress_log, list):
        seen_progress_entries: set[tuple[str, str]] = set()
        known_by_type = {
            "archetype": archetype_ids,
            "card": card_ids,
            "relic": relic_ids,
            "enemy": enemy_ids,
            "status": status_ids,
            "consumable": consumable_ids,
            "challenge": challenge_ids,
            "achievement": achievement_ids,
        }
        for index, entry in enumerate(progress_log):
            if not require_type(errors, path, f"progress_log[{index}]", entry, dict):
                continue
            entry_type = entry.get("type")
            content_id = entry.get("content_id")
            if not isinstance(entry_type, str) or not entry_type:
                errors.append(error(path, f"progress_log[{index}].type must be a non-empty string"))
                continue
            if not isinstance(content_id, str) or not content_id:
                errors.append(error(path, f"progress_log[{index}].content_id must be a non-empty string"))
                continue
            if entry_type not in PROFILE_PROGRESS_TYPES:
                errors.append(error(path, f"progress_log[{index}].type has unknown value '{entry_type}'"))
                continue
            key = (entry_type, content_id)
            if key in seen_progress_entries:
                errors.append(error(path, f"progress_log contains duplicate entry '{entry_type}:{content_id}'"))
            seen_progress_entries.add(key)
            known_ids = known_by_type.get(PROFILE_PROGRESS_TYPES[entry_type], set())
            if known_ids and content_id not in known_ids:
                errors.append(error(path, f"progress_log[{index}] references unknown id '{content_id}'"))
    return errors


def validate_actor_states(path: Path, data: dict[str, Any], errors: list[str]) -> None:
    actor_ids = string_list(errors, path, "actor_definition_ids", data.get("actor_definition_ids", []))
    actor_states = data.get("actor_states", [])
    if not require_type(errors, path, "actor_states", actor_states, list):
        return

    seen_actors: set[str] = set()
    actor_relics: list[str] = []
    for index, actor in enumerate(actor_states):
        if not require_type(errors, path, f"actor_states[{index}]", actor, dict):
            continue
        definition_id = actor.get("definition_id")
        if not isinstance(definition_id, str) or not definition_id:
            errors.append(error(path, f"actor_states[{index}].definition_id must be a non-empty string"))
            continue
        if definition_id in seen_actors:
            errors.append(error(path, f"duplicate actor state '{definition_id}'"))
        seen_actors.add(definition_id)
        if actor_ids and definition_id not in actor_ids:
            errors.append(error(path, f"actor state '{definition_id}' is missing from actor_definition_ids"))
        current_hp = actor.get("current_hp")
        max_hp = actor.get("max_hp")
        if not isinstance(current_hp, int) or not isinstance(max_hp, int) or max_hp <= 0 or current_hp < 0 or current_hp > max_hp:
            errors.append(error(path, f"actor '{definition_id}' has invalid hp {current_hp}/{max_hp}"))
        max_stress = actor.get("max_stress", 200)
        stress = actor.get("stress", 0)
        if not isinstance(stress, int) or not isinstance(max_stress, int) or max_stress <= 0 or stress < 0:
            errors.append(error(path, f"actor '{definition_id}' has invalid stress {stress}/{max_stress}"))
        actor_relics.extend(string_list(errors, path, f"actor_states[{index}].relic_ids", actor.get("relic_ids", [])))

    for actor_id in actor_ids:
        if actor_id not in seen_actors:
            errors.append(error(path, f"actor_definition_ids contains '{actor_id}' but actor_states does not"))

    root_relics = string_list(errors, path, "relic_ids", data.get("relic_ids", []))
    if sorted(set(root_relics)) != sorted(set(actor_relics)):
        errors.append(error(path, "root relic_ids must match the union of actor_states[*].relic_ids"))


def validate_deck(path: Path, data: dict[str, Any], errors: list[str]) -> None:
    deck = string_list(errors, path, "deck_card_ids", data.get("deck_card_ids", []))
    if not deck:
        errors.append(error(path, "deck_card_ids must not be empty"))

    upgraded = data.get("upgraded_deck_indices", [])
    if not require_type(errors, path, "upgraded_deck_indices", upgraded, list):
        return
    seen: set[int] = set()
    for index, value in enumerate(upgraded):
        if not isinstance(value, int):
            errors.append(error(path, f"upgraded_deck_indices[{index}] must be integer"))
            continue
        if value < 0 or value >= len(deck):
            errors.append(error(path, f"upgraded_deck_indices[{index}] points outside deck"))
        if value in seen:
            errors.append(error(path, f"upgraded_deck_indices contains duplicate {value}"))
        seen.add(value)


def validate_consumables(path: Path, data: dict[str, Any], errors: list[str]) -> None:
    consumables = string_list(errors, path, "consumable_ids", data.get("consumable_ids", []))
    max_consumables = data.get("max_consumables", 3)
    if not isinstance(max_consumables, int) or max_consumables < 0:
        errors.append(error(path, "max_consumables must be a non-negative integer"))
        return
    if len(consumables) > max_consumables:
        errors.append(error(path, "consumable_ids contains more items than max_consumables"))



def validate_active_item(path: Path, data: dict[str, Any], errors: list[str], known_ids: set[str]) -> None:
    active_item = data.get("active_item", {"item_id": "", "charge": 0})
    if not require_type(errors, path, "active_item", active_item, dict):
        return
    item_id = active_item.get("item_id", "")
    charge = active_item.get("charge", 0)
    if not isinstance(item_id, str):
        errors.append(error(path, "active_item.item_id must be a string"))
    elif item_id and known_ids and item_id not in known_ids:
        errors.append(error(path, f"active_item references unknown id '{item_id}'"))
    if not isinstance(charge, int) or isinstance(charge, bool) or charge < 0:
        errors.append(error(path, "active_item.charge must be a non-negative integer"))
    if not item_id and charge != 0:
        errors.append(error(path, "empty active_item slot must have zero charge"))


def validate_pending_room(path: Path, data: dict[str, Any], errors: list[str]) -> None:
    pending = data.get("pending_room")
    if pending is None:
        return
    if not require_type(errors, path, "pending_room", pending, dict):
        return
    pending_type = pending.get("type", "none")
    if pending_type == "none":
        return

    run_map = data.get("map", {})
    if not require_type(errors, path, "map", run_map, dict):
        return
    nodes = run_map.get("nodes", [])
    if not require_type(errors, path, "map.nodes", nodes, list):
        return
    node_id = pending.get("node_id")
    if not isinstance(node_id, int):
        errors.append(error(path, "pending_room.node_id must be integer"))
        return

    matching = [node for node in nodes if isinstance(node, dict) and node.get("id") == node_id]
    if not matching:
        errors.append(error(path, "pending_room.node_id does not reference a map node"))
        return
    node = matching[0]
    node_type = node.get("type")
    node_state = node.get("state")
    if pending_type not in NODE_TYPE_TO_PENDING_TYPES.get(node_type, set()):
        errors.append(error(path, "pending_room.type does not match its map node type"))
    expected_state = "completed" if pending_type == "combat_reward" else "current"
    if node_state != expected_state:
        errors.append(error(path, f"pending_room node must be {expected_state}, got {node_state!r}"))

    shop = pending.get("shop", {})
    if pending_type in {"shop", "merchant_rest"}:
        if not require_type(errors, path, "pending_room.shop", shop, dict):
            return
        expected_mode = "merchant_rest" if pending_type == "merchant_rest" else "shop"
        if shop.get("mode", expected_mode) != expected_mode:
            errors.append(error(path, f"pending_room.shop.mode must be {expected_mode!r}"))
        if pending_type == "merchant_rest" and shop.get("merchant_rest_card_shop_open", False):
            if not isinstance(shop.get("max_card_purchases"), int) or shop.get("max_card_purchases", 0) <= 0:
                errors.append(error(path, "open merchant-rest card shop must have positive max_card_purchases"))
    elif pending_type == "event" and not pending.get("event_id"):
        errors.append(error(path, "pending event room must store event_id"))


RUN_PHASES = {"map", "combat", "reward", "event", "shop", "rest", "chest", "floor_complete", "run_complete"}
PENDING_TYPE_TO_PHASE = {
    "combat_reward": "reward",
    "chest_reward": "chest",
    "shop": "shop",
    "merchant_rest": "rest",
    "event": "event",
}


def validate_run_phase(path: Path, data: dict[str, Any], errors: list[str]) -> None:
    version = data.get("version", 1)
    phase = data.get("phase", "map" if version == 1 else None)
    if phase not in RUN_PHASES:
        errors.append(error(path, f"phase must be one of {sorted(RUN_PHASES)}, got {phase!r}"))
        return

    if data.get("act_completed", False):
        if phase != "floor_complete":
            errors.append(error(path, "completed run save must have phase='floor_complete'"))
        return

    pending = data.get("pending_room", {})
    if isinstance(pending, dict):
        pending_type = pending.get("type", "none")
        if pending_type in PENDING_TYPE_TO_PHASE and phase != PENDING_TYPE_TO_PHASE[pending_type]:
            errors.append(error(path, f"pending_room.type={pending_type!r} requires phase={PENDING_TYPE_TO_PHASE[pending_type]!r}"))

    if phase == "run_complete":
        errors.append(error(path, "active run saves must not use phase='run_complete'"))


RUN_COMPLETION_TYPES = {"in_progress", "floor_cleared", "playable_content_complete", "victory"}


def validate_floor_state(path: Path, data: dict[str, Any], errors: list[str]) -> None:
    current_floor_id = data.get("current_floor_id", "floor1")
    if not isinstance(current_floor_id, str) or not current_floor_id:
        errors.append(error(path, "current_floor_id must be a non-empty string"))
    current_floor_index = data.get("current_floor_index", data.get("act", 1))
    if not isinstance(current_floor_index, int) or isinstance(current_floor_index, bool) or current_floor_index <= 0:
        errors.append(error(path, "current_floor_index must be a positive integer"))
    next_floor_id = data.get("next_floor_id", "")
    if not isinstance(next_floor_id, str):
        errors.append(error(path, "next_floor_id must be a string"))
    act = data.get("act", 1)
    if not isinstance(act, int) or isinstance(act, bool) or act <= 0:
        errors.append(error(path, "act must be a positive integer"))

    act_completed = data.get("act_completed", False)
    if not isinstance(act_completed, bool):
        errors.append(error(path, "act_completed must be boolean"))
        act_completed = False

    if "run_completion_type" in data:
        completion_type = data.get("run_completion_type")
    elif act_completed:
        completion_type = "victory" if not next_floor_id else "floor_cleared"
    else:
        completion_type = "in_progress"

    if completion_type not in RUN_COMPLETION_TYPES:
        errors.append(error(path, f"run_completion_type must be one of {sorted(RUN_COMPLETION_TYPES)}, got {completion_type!r}"))
    elif act_completed and completion_type == "in_progress":
        errors.append(error(path, "completed run saves must not have run_completion_type='in_progress'"))
    elif not act_completed and completion_type != "in_progress":
        errors.append(error(path, "in-progress run saves must have run_completion_type='in_progress'"))

    if act_completed and not next_floor_id and completion_type not in {"victory", "playable_content_complete"}:
        errors.append(error(path, "completed final-floor saves should use victory or playable_content_complete"))



def validate_save(path: Path, active_item_ids: set[str]) -> list[str]:
    errors: list[str] = []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        return [error(path, f"cannot read JSON: {exc}")]

    if not require_type(errors, path, "save root", data, dict):
        return errors
    if data.get("version") not in {1, 2, 3}:
        errors.append(error(path, "version must be 1, 2 or 3"))
    challenge_id = data.get("challenge_id", "")
    if not isinstance(challenge_id, str):
        errors.append(error(path, "challenge_id must be a string"))

    validate_floor_state(path, data, errors)
    validate_deck(path, data, errors)
    validate_actor_states(path, data, errors)
    validate_consumables(path, data, errors)
    validate_active_item(path, data, errors, active_item_ids)
    validate_pending_room(path, data, errors)
    validate_run_phase(path, data, errors)
    return errors


def main() -> int:
    run_save_files = sorted(SAVES_DIR.glob("slot_*/run_save*.json"))
    profile_save_files = sorted(SAVES_DIR.glob("slot_*/profile*.json"))

    card_ids = load_card_ids()
    relic_ids = load_id_set("data/relics/relics.json")
    archetype_ids = load_id_set("data/archetypes/playable_archetypes.json")
    challenge_ids = load_ids_from_directory("challenges")
    achievement_ids = load_ids_from_directory("achievements")
    enemy_ids = load_ids_from_directory("enemies")
    status_ids = load_ids_from_directory("statuses")
    consumable_ids = load_ids_from_directory("consumables")
    active_item_ids = load_ids_from_directory("active_items")

    all_errors: list[str] = []
    for save_file in run_save_files:
        all_errors.extend(validate_save(save_file, active_item_ids))
    for save_file in profile_save_files:
        all_errors.extend(validate_profile_save(
            save_file,
            card_ids,
            relic_ids,
            archetype_ids,
            challenge_ids,
            achievement_ids,
            enemy_ids,
            status_ids,
            consumable_ids,
        ))

    if all_errors:
        print(f"Save validation failed with {len(all_errors)} error(s):")
        for item in all_errors:
            print(f" - {item}")
        return 1

    print(
        "Save files OK: "
        f"{len(run_save_files)} run save file(s), "
        f"{len(profile_save_files)} profile save file(s) checked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
