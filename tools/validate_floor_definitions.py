#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
FLOORS_PATH = DATA_DIR / "run" / "floors.json"


NODE_POOLS = ("combat", "elite", "boss")
NODE_TYPES = {"combat", "elite", "boss", "chest", "event", "shop", "rest"}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def error(owner: str, message: str) -> str:
    return f"{owner}: {message}"


def expect_string(errors: list[str], owner: str, key: str, value: Any, *, required: bool = True) -> str:
    if value is None:
        if required:
            errors.append(error(owner, f"{key} must be a non-empty string"))
        return ""
    if not isinstance(value, str):
        errors.append(error(owner, f"{key} must be a string"))
        return ""
    if required and not value:
        errors.append(error(owner, f"{key} must not be empty"))
    return value


def expect_positive_int(errors: list[str], owner: str, key: str, value: Any) -> int | None:
    if not isinstance(value, int) or isinstance(value, bool):
        errors.append(error(owner, f"{key} must be a positive integer"))
        return None
    if value <= 0:
        errors.append(error(owner, f"{key} must be positive"))
        return None
    return value


def validate_path_field(errors: list[str], owner: str, field_name: str, raw_path: str, base_dir: Path) -> Path | None:
    candidate = (base_dir / raw_path).resolve()
    try:
        candidate.relative_to(ROOT.resolve())
    except ValueError:
        errors.append(error(owner, f"{field_name} must stay inside the project directory"))
        return None
    if not candidate.is_file():
        errors.append(error(owner, f"{field_name} points to missing file '{raw_path}'"))
        return None
    return candidate


def load_enemy_ids() -> set[str]:
    result: set[str] = set()
    enemy_dir = DATA_DIR / "enemies"
    for path in sorted(enemy_dir.glob("*.json")):
        try:
            root = load_json(path)
        except Exception:  # noqa: BLE001 - the project validator reports parse errors separately.
            continue
        if not isinstance(root, list):
            continue
        for enemy in root:
            if not isinstance(enemy, dict):
                continue
            enemy_id = enemy.get("id")
            if isinstance(enemy_id, str) and enemy_id:
                result.add(enemy_id)
    return result


def event_pool_ids(event: dict[str, Any]) -> list[str]:
    result: list[str] = []
    single = event.get("event_pool_id")
    if isinstance(single, str) and single:
        result.append(single)

    raw_pools = event.get("event_pools", [])
    if isinstance(raw_pools, list):
        for item in raw_pools:
            if isinstance(item, str) and item:
                result.append(item)
    return result


def event_belongs_to_pool(event: dict[str, Any], pool_id: str) -> bool:
    pools = event_pool_ids(event)
    if not pools:
        return pool_id == "act1"
    return pool_id in pools


def event_has_skip_choice(event: dict[str, Any]) -> bool:
    choices = event.get("choices")
    if not isinstance(choices, list):
        return False
    for choice in choices:
        if not isinstance(choice, dict):
            continue
        effects = choice.get("effects")
        if not isinstance(effects, list):
            continue
        for effect in effects:
            if isinstance(effect, dict) and effect.get("type") == "skip":
                return True
    return False


def validate_event_pools(errors: list[str], floors: list[Any]) -> None:
    event_path = DATA_DIR / "events" / "run_events.json"
    try:
        events = load_json(event_path)
    except Exception as exc:  # noqa: BLE001
        errors.append(f"{rel(event_path)}: cannot read JSON: {exc}")
        return

    if not isinstance(events, list):
        errors.append(f"{rel(event_path)}: root must be an array")
        return

    floor_pool_ids = {
        floor.get("event_pool_id")
        for floor in floors
        if isinstance(floor, dict) and isinstance(floor.get("event_pool_id"), str) and floor.get("event_pool_id")
    }

    for index, event in enumerate(events):
        owner = f"{rel(event_path)}[{index}]"
        if not isinstance(event, dict):
            continue
        event_id = event.get("id")
        if not isinstance(event_id, str) or not event_id:
            continue
        raw_event_pools = event.get("event_pools")
        if raw_event_pools is not None and not isinstance(raw_event_pools, list):
            errors.append(error(owner, "event_pools must be an array of strings"))
        for pool_id in event_pool_ids(event):
            if pool_id not in floor_pool_ids:
                errors.append(error(owner, f"event '{event_id}' references unknown event pool '{pool_id}'"))
        if not event_has_skip_choice(event):
            errors.append(error(owner, f"event '{event_id}' must have a skip/no-op choice"))

    for floor in floors:
        if not isinstance(floor, dict) or not floor.get("is_implemented", True):
            continue
        pool_id = floor.get("event_pool_id")
        if not isinstance(pool_id, str) or not pool_id:
            continue
        pool_events = [event for event in events if isinstance(event, dict) and event_belongs_to_pool(event, pool_id)]
        if len(pool_events) < 8:
            errors.append(error(f"{rel(FLOORS_PATH)} floor '{floor.get('id')}'", f"event pool '{pool_id}' has only {len(pool_events)} event(s), expected at least 8"))
        if not any(
            any(isinstance(effect, dict) and effect.get("type") == "gain_random_relic"
                for choice in event.get("choices", []) if isinstance(choice, dict)
                for effect in choice.get("effects", []) if isinstance(effect, dict))
            for event in pool_events
        ):
            errors.append(error(f"{rel(FLOORS_PATH)} floor '{floor.get('id')}'", f"event pool '{pool_id}' must include at least one random relic reward"))


def validate_act_map_path(errors: list[str], owner: str, floor: dict[str, Any], base_dir: Path, *, required: bool) -> None:
    map_config_id = floor.get("map_config_id")
    map_config_path = floor.get("map_config_path")
    if not isinstance(map_config_id, str) or not map_config_id:
        if required:
            errors.append(error(owner, "implemented floor must define map_config_id"))
        return
    if not isinstance(map_config_path, str) or not map_config_path:
        if required:
            errors.append(error(owner, "implemented floor must define map_config_path"))
        return

    path = validate_path_field(errors, owner, "map_config_path", map_config_path, base_dir)
    if path is None:
        return
    try:
        data = load_json(path)
    except Exception as exc:  # noqa: BLE001
        errors.append(error(owner, f"cannot read map_config_path '{map_config_path}': {exc}"))
        return
    if not isinstance(data, dict):
        errors.append(error(owner, "map config root must be an object"))
        return
    if data.get("id") != map_config_id:
        errors.append(error(owner, f"map_config_id '{map_config_id}' does not match {rel(path)} id '{data.get('id')}'"))

    layer_counts = data.get("layer_node_counts")
    if not isinstance(layer_counts, list) or len(layer_counts) < 4:
        errors.append(error(owner, "map config layer_node_counts must contain at least 4 layers"))
    elif any(not isinstance(value, int) or isinstance(value, bool) or value <= 0 for value in layer_counts):
        errors.append(error(owner, "map config layer_node_counts must contain positive integers"))

    for field in ("event_weight", "combat_weight", "extra_connection_chance", "question_mark_combat_chance"):
        value = data.get(field)
        if not isinstance(value, int) or isinstance(value, bool) or value < 0:
            errors.append(error(owner, f"map config {field} must be a non-negative integer"))

    for field in ("shop", "chests", "elites"):
        if not isinstance(data.get(field), dict):
            errors.append(error(owner, f"map config {field} must be an object"))


def validate_encounter_path(errors: list[str], owner: str, floor: dict[str, Any], base_dir: Path, *, required: bool) -> None:
    encounter_table_id = floor.get("encounter_table_id")
    encounter_table_path = floor.get("encounter_table_path")
    if not isinstance(encounter_table_id, str) or not encounter_table_id:
        if required:
            errors.append(error(owner, "implemented floor must define encounter_table_id"))
        return
    if not isinstance(encounter_table_path, str) or not encounter_table_path:
        if required:
            errors.append(error(owner, "implemented floor must define encounter_table_path"))
        return

    path = validate_path_field(errors, owner, "encounter_table_path", encounter_table_path, base_dir)
    if path is None:
        return
    try:
        data = load_json(path)
    except Exception as exc:  # noqa: BLE001
        errors.append(error(owner, f"cannot read encounter_table_path '{encounter_table_path}': {exc}"))
        return
    if not isinstance(data, dict) or not isinstance(data.get("pools"), dict):
        errors.append(error(owner, "encounter table must contain object 'pools'"))
        return

    enemy_ids = load_enemy_ids()
    seen_encounters: set[str] = set()
    for pool_name in NODE_POOLS:
        pool = data["pools"].get(pool_name)
        if not isinstance(pool, list) or not pool:
            errors.append(error(owner, f"encounter pool '{pool_name}' must be a non-empty array"))
            continue
        for index, encounter in enumerate(pool):
            encounter_owner = f"{owner}.{pool_name}[{index}]"
            if not isinstance(encounter, dict):
                errors.append(error(encounter_owner, "encounter must be an object"))
                continue
            encounter_id = encounter.get("id")
            if not isinstance(encounter_id, str) or not encounter_id:
                errors.append(error(encounter_owner, "id must be a non-empty string"))
            elif encounter_id in seen_encounters:
                errors.append(error(encounter_owner, f"duplicate encounter id '{encounter_id}'"))
            else:
                seen_encounters.add(encounter_id)

            enemies = encounter.get("enemies")
            if not isinstance(enemies, list) or not enemies:
                errors.append(error(encounter_owner, "enemies must be a non-empty array"))
            else:
                for enemy_id in enemies:
                    if not isinstance(enemy_id, str) or not enemy_id:
                        errors.append(error(encounter_owner, "enemy ids must be non-empty strings"))
                    elif enemy_id not in enemy_ids:
                        errors.append(error(encounter_owner, f"references unknown enemy '{enemy_id}'"))

            weight = encounter.get("weight", 1)
            if not isinstance(weight, int) or isinstance(weight, bool) or weight <= 0:
                errors.append(error(encounter_owner, "weight must be a positive integer"))

            min_layer = encounter.get("min_layer", -1)
            max_layer = encounter.get("max_layer", -1)
            if not isinstance(min_layer, int) or isinstance(min_layer, bool) or min_layer < -1:
                errors.append(error(encounter_owner, "min_layer must be -1 or a non-negative integer"))
            if not isinstance(max_layer, int) or isinstance(max_layer, bool) or max_layer < -1:
                errors.append(error(encounter_owner, "max_layer must be -1 or a non-negative integer"))
            if isinstance(min_layer, int) and isinstance(max_layer, int) and min_layer >= 0 and max_layer >= 0 and min_layer > max_layer:
                errors.append(error(encounter_owner, "min_layer must be <= max_layer"))


def validate_floors() -> list[str]:
    errors: list[str] = []
    try:
        root = load_json(FLOORS_PATH)
    except Exception as exc:  # noqa: BLE001
        return [f"{rel(FLOORS_PATH)}: cannot read JSON: {exc}"]

    if not isinstance(root, dict):
        return [f"{rel(FLOORS_PATH)}: root must be an object"]
    floors = root.get("floors")
    if not isinstance(floors, list) or not floors:
        return [f"{rel(FLOORS_PATH)}: floors must be a non-empty array"]

    validate_event_pools(errors, floors)

    ids: set[str] = set()
    indices: set[int] = set()
    implemented_count = 0
    base_dir = FLOORS_PATH.parent
    known_ids = {item.get("id") for item in floors if isinstance(item, dict)}

    for index, floor in enumerate(floors):
        owner = f"{rel(FLOORS_PATH)}.floors[{index}]"
        if not isinstance(floor, dict):
            errors.append(error(owner, "must be an object"))
            continue

        floor_id = expect_string(errors, owner, "id", floor.get("id"))
        floor_index = expect_positive_int(errors, owner, "index", floor.get("index"))
        expect_positive_int(errors, owner, "act", floor.get("act", floor.get("index")))
        expect_string(errors, owner, "name_text_id", floor.get("name_text_id"))
        expect_string(errors, owner, "theme_id", floor.get("theme_id"), required=False)
        next_floor_id = expect_string(errors, owner, "next_floor_id", floor.get("next_floor_id", ""), required=False)

        if floor_id:
            if floor_id in ids:
                errors.append(error(owner, f"duplicate floor id '{floor_id}'"))
            ids.add(floor_id)
        if floor_index is not None:
            if floor_index in indices:
                errors.append(error(owner, f"duplicate floor index {floor_index}"))
            indices.add(floor_index)

        is_implemented = floor.get("is_implemented", True)
        if not isinstance(is_implemented, bool):
            errors.append(error(owner, "is_implemented must be boolean"))
            continue

        if is_implemented:
            implemented_count += 1
            event_pool_id = floor.get("event_pool_id")
            if not isinstance(event_pool_id, str) or not event_pool_id:
                errors.append(error(owner, "implemented floor must define event_pool_id"))

        # Validate optional skeleton paths even for disabled future floors. This keeps floor2 content honest
        # while still preventing runtime from entering an unfinished floor.
        validate_act_map_path(errors, owner, floor, base_dir, required=is_implemented)
        validate_encounter_path(errors, owner, floor, base_dir, required=is_implemented)

        if next_floor_id and next_floor_id not in known_ids:
            errors.append(error(owner, f"next_floor_id '{next_floor_id}' does not exist"))

    if implemented_count == 0:
        errors.append(f"{rel(FLOORS_PATH)}: at least one floor must be implemented")
    if 1 not in indices:
        errors.append(f"{rel(FLOORS_PATH)}: floor index 1 must exist")

    return errors


def main() -> int:
    errors = validate_floors()
    if errors:
        print(f"Floor definition validation failed with {len(errors)} error(s):")
        for item in errors:
            print(f" - {item}")
        return 1

    data = load_json(FLOORS_PATH)
    implemented = [floor for floor in data["floors"] if floor.get("is_implemented", True)]
    skeletons = [
        floor for floor in data["floors"]
        if not floor.get("is_implemented", True)
        and isinstance(floor.get("map_config_path"), str)
        and isinstance(floor.get("encounter_table_path"), str)
    ]
    print(f"Floor definitions OK: {len(data['floors'])} floor(s), {len(implemented)} implemented, {len(skeletons)} skeleton")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
