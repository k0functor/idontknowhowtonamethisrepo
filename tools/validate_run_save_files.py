#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
SAVES_DIR = ROOT / "saves"

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



def validate_save(path: Path) -> list[str]:
    errors: list[str] = []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        return [error(path, f"cannot read JSON: {exc}")]

    if not require_type(errors, path, "save root", data, dict):
        return errors
    if data.get("version") != 1:
        errors.append(error(path, "version must be 1"))
    validate_floor_state(path, data, errors)
    validate_deck(path, data, errors)
    validate_actor_states(path, data, errors)
    validate_consumables(path, data, errors)
    validate_pending_room(path, data, errors)
    return errors


def main() -> int:
    save_files = sorted(SAVES_DIR.glob("slot_*/run_save*.json"))
    all_errors: list[str] = []
    for save_file in save_files:
        all_errors.extend(validate_save(save_file))

    if all_errors:
        print(f"Run save validation failed with {len(all_errors)} error(s):")
        for item in all_errors:
            print(f" - {item}")
        return 1

    print(f"Run save files OK: {len(save_files)} file(s) checked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
