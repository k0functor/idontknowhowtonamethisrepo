#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict, deque
from pathlib import Path
from typing import Any

MAX_ENEMIES_PER_ENCOUNTER = 3
ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
FLOORS_PATH = DATA_DIR / "run" / "floors.json"
EVENTS_PATH = DATA_DIR / "events" / "run_events.json"

ROOM_TYPES = {"combat", "elite", "event", "shop", "chest", "rest", "boss"}
ENCOUNTER_POOLS = {"combat", "elite", "boss"}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def load_index(directory: Path) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for path in sorted(directory.glob("*.json")):
        data = load_json(path)
        if not isinstance(data, list):
            continue
        for item in data:
            if isinstance(item, dict) and isinstance(item.get("id"), str):
                result[item["id"]] = item
    return result


def layer_widths(config: dict[str, Any]) -> list[int]:
    counts = config.get("layer_node_counts")
    if isinstance(counts, list):
        return [int(count) for count in counts]
    layer_count = int(config.get("layer_count", 8))
    middle_min = int(config.get("middle_min_nodes", 2))
    return [1 if layer in {0, layer_count - 2, layer_count - 1} else middle_min for layer in range(layer_count)]


def floor_label(floor: dict[str, Any]) -> str:
    floor_id = floor.get("id", "<unknown>")
    act_id = floor.get("map_config_id", "<no-act>")
    return f"floor '{floor_id}' ({act_id})"


def resolve_floor_path(floor: dict[str, Any], field: str) -> Path | None:
    raw = floor.get(field)
    if not isinstance(raw, str) or not raw:
        return None
    return (FLOORS_PATH.parent / raw).resolve()


def collect_slots(layers: list[list[str]], min_layer: int, max_layer: int, used: set[tuple[int, int]]) -> list[tuple[int, int]]:
    result: list[tuple[int, int]] = []
    for layer in range(max(0, min_layer), min(max_layer, len(layers) - 1) + 1):
        for index in range(len(layers[layer])):
            slot = (layer, index)
            if slot not in used:
                result.append(slot)
    return result


def placement_rule_allows(candidate: tuple[int, int], placed: list[tuple[int, int]], config: dict[str, Any]) -> bool:
    layer, _ = candidate
    max_per_layer = int(config.get("max_per_layer", 0))
    min_layer_gap = int(config.get("min_layer_gap", 0))
    if max_per_layer > 0 and sum(1 for placed_layer, _ in placed if placed_layer == layer) >= max_per_layer:
        return False
    if min_layer_gap > 0 and any(abs(placed_layer - layer) < min_layer_gap for placed_layer, _ in placed):
        return False
    return True


def place_specials(
    layers: list[list[str]],
    used: set[tuple[int, int]],
    candidates: list[tuple[int, int]],
    count: int,
    room_type: str,
    rng: Any,
    errors: list[str],
    label: str,
    owner: str,
    placement_config: dict[str, Any] | None = None,
) -> None:
    if count <= 0:
        return
    if count > len(candidates):
        errors.append(f"{owner}: cannot place {label}: requested {count}, candidates {len(candidates)}")
        return
    placed: list[tuple[int, int]] = []
    placement_config = placement_config or {}
    for _ in range(count):
        valid_indices = [
            index for index, candidate in enumerate(candidates)
            if placement_rule_allows(candidate, placed, placement_config)
        ]
        if not valid_indices:
            errors.append(f"{owner}: cannot place {label}: placement rules are too strict")
            return
        choice_index = rng.choice(valid_indices)
        layer, index = candidates.pop(choice_index)
        layers[layer][index] = room_type
        used.add((layer, index))
        placed.append((layer, index))


def adjacent_target_indices(from_index: int, from_count: int, to_count: int) -> list[int]:
    if from_count <= 0 or to_count <= 0:
        return []
    if from_count == 1 or to_count == 1:
        return list(range(to_count))
    first = (from_index * to_count) // from_count
    last = ((from_index + 1) * to_count) // from_count
    if last >= to_count:
        last = to_count - 1
    if first > last:
        first = last
    return list(range(first, last + 1))


def connect_layers(widths: list[int], rng: Any, extra_connection_chance: int) -> list[list[list[int]]]:
    edges: list[list[list[int]]] = []
    extra_probability = max(0, min(100, int(extra_connection_chance))) / 100.0
    for layer in range(len(widths) - 1):
        from_count = widths[layer]
        to_count = widths[layer + 1]
        selected: list[list[int]] = [[] for _ in range(from_count)]
        if from_count == 1 or to_count == 1:
            edges.append([adjacent_target_indices(source, from_count, to_count) for source in range(from_count)])
            continue
        for target in range(to_count):
            sources = [source for source in range(from_count) if target in adjacent_target_indices(source, from_count, to_count)]
            selected[rng.choice(sources)].append(target)
        for source in range(from_count):
            if not selected[source]:
                selected[source].append(rng.choice(adjacent_target_indices(source, from_count, to_count)))
        for source in range(from_count):
            for target in adjacent_target_indices(source, from_count, to_count):
                if target not in selected[source] and rng.random() < extra_probability:
                    selected[source].append(target)
        for targets in selected:
            targets.sort()
        edges.append(selected)
    return edges


def generate_snapshot(seed: int, config: dict[str, Any], owner: str, errors: list[str]) -> tuple[list[list[str]], list[list[list[int]]]]:
    import random

    rng = random.Random(seed)
    widths = layer_widths(config)
    layer_count = len(widths)
    pre_boss_layer = layer_count - 2
    boss_layer = layer_count - 1
    combat_weight = int(config.get("combat_weight", 70))
    event_weight = int(config.get("event_weight", 30))
    total = max(1, combat_weight + event_weight)

    layers: list[list[str]] = []
    for layer, width in enumerate(widths):
        if layer == 0:
            layers.append(["combat"] * width)
        elif layer == pre_boss_layer:
            layers.append(["rest"] * width)
        elif layer == boss_layer:
            layers.append(["boss"] * width)
        elif "events" in config:
            layers.append(["combat"] * width)
        else:
            room_types = ["combat" if rng.randint(1, total) <= combat_weight else "event" for _ in range(width)]
            if event_weight > 0 and combat_weight > 0 and width > 1:
                max_events = max(1, min(width - 1, round(width * event_weight / total)))
                event_indices = [index for index, room_type in enumerate(room_types) if room_type == "event"]
                while len(event_indices) > max_events:
                    index = rng.choice(event_indices)
                    room_types[index] = "combat"
                    event_indices.remove(index)
            layers.append(room_types)

    used: set[tuple[int, int]] = set()
    for key, room_type in (("chests", "chest"), ("shop", "shop")):
        item = config.get(key, {})
        if isinstance(item, dict):
            candidates = collect_slots(layers, int(item.get("min_layer", 1)), int(item.get("max_layer", pre_boss_layer - 1)), used)
            place_specials(layers, used, candidates, int(item.get("count", 0)), room_type, rng, errors, key, owner, item)

    elites = config.get("elites", {})
    if isinstance(elites, dict):
        count = rng.randint(int(elites.get("min", 0)), int(elites.get("max", 0)))
        candidates = collect_slots(layers, int(elites.get("min_layer", 1)), int(elites.get("max_layer", pre_boss_layer - 1)), used)
        place_specials(layers, used, candidates, count, "elite", rng, errors, "elites", owner, elites)

    fixed_events = config.get("events")
    if isinstance(fixed_events, dict):
        count = rng.randint(int(fixed_events.get("min", 0)), int(fixed_events.get("max", 0)))
        candidates = collect_slots(layers, int(fixed_events.get("min_layer", 1)), int(fixed_events.get("max_layer", pre_boss_layer - 1)), used)
        place_specials(layers, used, candidates, count, "event", rng, errors, "events", owner, fixed_events)

    edges = connect_layers([len(layer) for layer in layers], rng, int(config.get("extra_connection_chance", 0)))
    return layers, edges


def encounter_eligible(encounter: dict[str, Any], layer: int) -> bool:
    minimum = int(encounter.get("min_layer", 0))
    maximum = int(encounter.get("max_layer", 999))
    return minimum <= layer <= maximum


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


def events_for_pool(events: Any, pool_id: str) -> list[dict[str, Any]]:
    if not isinstance(events, list):
        return []
    return [event for event in events if isinstance(event, dict) and event_belongs_to_pool(event, pool_id)]



def validate_placement_rules(config: dict[str, Any], owner: str, label: str, errors: list[str]) -> None:
    max_per_layer = int(config.get("max_per_layer", 0))
    min_layer_gap = int(config.get("min_layer_gap", 0))
    if max_per_layer < 0:
        errors.append(f"{owner}: {label}.max_per_layer must be non-negative")
    if min_layer_gap < 0:
        errors.append(f"{owner}: {label}.min_layer_gap must be non-negative")

def validate_act_config(config: Any, floor: dict[str, Any], errors: list[str]) -> dict[str, Any] | None:
    owner = floor_label(floor)
    act_id = floor.get("map_config_id")
    if not isinstance(config, dict):
        errors.append(f"{owner}: map config root must be an object")
        return None
    if config.get("id") != act_id:
        errors.append(f"{owner}: map config id must be '{act_id}', got '{config.get('id')}'")
    widths = layer_widths(config)
    if len(widths) < 4:
        errors.append(f"{owner}: must have at least 4 layers")
        return config
    if any(width <= 0 for width in widths):
        errors.append(f"{owner}: layer_node_counts must contain only positive values")
    if widths[0] != 1:
        errors.append(f"{owner}: first layer must contain exactly one start combat node")
    if widths[-1] != 1:
        errors.append(f"{owner}: boss layer must contain exactly one boss node")

    pre_boss_layer = len(widths) - 2
    middle_capacity = sum(widths[layer] for layer in range(1, pre_boss_layer))
    reserved_max = 0

    for key in ("shop", "chests"):
        item = config.get(key, {})
        if not isinstance(item, dict):
            errors.append(f"{owner}: '{key}' config must be an object")
            continue
        validate_placement_rules(item, owner, key, errors)
        count = int(item.get("count", 0))
        min_layer = int(item.get("min_layer", 1))
        max_layer = int(item.get("max_layer", pre_boss_layer - 1))
        reserved_max += max(0, count)
        if count < 0:
            errors.append(f"{owner}: {key}.count must be non-negative")
        if min_layer > max_layer:
            errors.append(f"{owner}: {key}.min_layer must be <= max_layer")
        if min_layer <= 0 or max_layer >= pre_boss_layer:
            errors.append(f"{owner}: {key} must be placed after start and before the rest layer")
        capacity = sum(widths[layer] for layer in range(max(0, min_layer), min(max_layer, pre_boss_layer - 1) + 1))
        if count > capacity:
            errors.append(f"{owner}: cannot place {key}: requested {count}, capacity {capacity}")
        if item.get("full_layer", False):
            if min_layer != max_layer:
                errors.append(f"{owner}: {key}.full_layer requires min_layer == max_layer")
            elif 0 <= min_layer < len(widths) and count != widths[min_layer]:
                errors.append(f"{owner}: {key}.full_layer requires count {widths[min_layer]}, got {count}")

    elites = config.get("elites", {})
    if not isinstance(elites, dict):
        errors.append(f"{owner}: 'elites' config must be an object")
    else:
        validate_placement_rules(elites, owner, "elites", errors)
        minimum = int(elites.get("min", 0))
        maximum = int(elites.get("max", 0))
        min_layer = int(elites.get("min_layer", 1))
        max_layer = int(elites.get("max_layer", pre_boss_layer - 1))
        reserved_max += max(0, maximum)
        if minimum < 0 or maximum < 0 or minimum > maximum:
            errors.append(f"{owner}: elites min/max must be non-negative and min <= max")
        if min_layer <= 0 or max_layer >= pre_boss_layer:
            errors.append(f"{owner}: elites must be placed after start and before the rest layer")

    fixed_events = config.get("events")
    if fixed_events is not None:
        if not isinstance(fixed_events, dict):
            errors.append(f"{owner}: 'events' config must be an object when present")
        else:
            validate_placement_rules(fixed_events, owner, "events", errors)
            minimum = int(fixed_events.get("min", 0))
            maximum = int(fixed_events.get("max", 0))
            min_layer = int(fixed_events.get("min_layer", 1))
            max_layer = int(fixed_events.get("max_layer", pre_boss_layer - 1))
            reserved_max += max(0, maximum)
            if minimum < 0 or maximum < 0 or minimum > maximum:
                errors.append(f"{owner}: events min/max must be non-negative and min <= max")
            if min_layer <= 0 or max_layer >= pre_boss_layer:
                errors.append(f"{owner}: events must be placed after start and before the rest layer")

    if reserved_max > middle_capacity:
        errors.append(f"{owner}: special-node maximum {reserved_max} exceeds middle-layer capacity {middle_capacity}")
    if int(config.get("question_mark_combat_chance", 0)) < 0 or int(config.get("question_mark_combat_chance", 0)) > 100:
        errors.append(f"{owner}: question_mark_combat_chance must be in 0..100")
    if int(config.get("combat_weight", 0)) < 0 or int(config.get("event_weight", 0)) < 0:
        errors.append(f"{owner}: combat/event weights must be non-negative")
    if int(config.get("combat_weight", 0)) + int(config.get("event_weight", 0)) <= 0:
        errors.append(f"{owner}: combat/event weights must not both be zero")

    return config


def validate_encounters(encounters_root: Any, enemy_ids: set[str], config: dict[str, Any], floor: dict[str, Any], errors: list[str]) -> dict[str, list[dict[str, Any]]]:
    owner = floor_label(floor)
    pools: dict[str, list[dict[str, Any]]] = {pool: [] for pool in ENCOUNTER_POOLS}
    if not isinstance(encounters_root, dict) or not isinstance(encounters_root.get("pools"), dict):
        errors.append(f"{owner}: encounter table must contain object 'pools'")
        return pools

    seen_ids: set[str] = set()
    for pool_name, encounters in encounters_root["pools"].items():
        if pool_name not in ENCOUNTER_POOLS:
            errors.append(f"{owner}: encounter pool '{pool_name}' is unknown")
            continue
        if not isinstance(encounters, list):
            errors.append(f"{owner}: encounter pool '{pool_name}' must be an array")
            continue
        for index, encounter in enumerate(encounters):
            encounter_owner = f"{owner} {pool_name} encounter[{index}]"
            if not isinstance(encounter, dict):
                errors.append(f"{encounter_owner} must be an object")
                continue
            encounter_id = encounter.get("id")
            if not isinstance(encounter_id, str) or not encounter_id:
                errors.append(f"{encounter_owner} must have a non-empty id")
                continue
            if encounter_id in seen_ids:
                errors.append(f"{owner}: encounter id '{encounter_id}' is duplicated")
            seen_ids.add(encounter_id)
            enemies = encounter.get("enemies")
            if not isinstance(enemies, list) or not enemies:
                errors.append(f"{owner}: encounter '{encounter_id}' must contain at least one enemy")
            else:
                if len(enemies) > MAX_ENEMIES_PER_ENCOUNTER:
                    errors.append(f"{owner}: encounter '{encounter_id}' contains more than {MAX_ENEMIES_PER_ENCOUNTER} enemies")
                for enemy_id in enemies:
                    if not isinstance(enemy_id, str) or enemy_id not in enemy_ids:
                        errors.append(f"{owner}: encounter '{encounter_id}' references unknown enemy '{enemy_id}'")
            if int(encounter.get("weight", 0)) <= 0:
                errors.append(f"{owner}: encounter '{encounter_id}' must have positive weight")
            if int(encounter.get("min_layer", 0)) > int(encounter.get("max_layer", 999)):
                errors.append(f"{owner}: encounter '{encounter_id}' has min_layer > max_layer")
            pools[pool_name].append(encounter)

    widths = layer_widths(config)
    boss_layer = len(widths) - 1
    if len(pools["combat"]) < 10:
        errors.append(f"{owner}: should have at least 10 combat encounters")
    if len(pools["elite"]) < 5:
        errors.append(f"{owner}: should have at least 5 elite encounters")
    required_boss_encounters = int(floor.get("boss_encounter_min", 3))
    if required_boss_encounters < 1:
        errors.append(f"{owner}: boss_encounter_min must be at least 1")
        required_boss_encounters = 1
    if len(pools["boss"]) < required_boss_encounters:
        errors.append(f"{owner}: should have at least {required_boss_encounters} boss encounter(s)")
    if not any(encounter_eligible(encounter, boss_layer) for encounter in pools["boss"]):
        errors.append(f"{owner}: has no boss encounter eligible for boss layer {boss_layer}")

    elite_cfg = config.get("elites", {}) if isinstance(config.get("elites"), dict) else {}
    elite_min = int(elite_cfg.get("min_layer", 1))
    elite_max = int(elite_cfg.get("max_layer", len(widths) - 3))
    if not any(any(encounter_eligible(encounter, layer) for encounter in pools["elite"]) for layer in range(elite_min, elite_max + 1)):
        errors.append(f"{owner}: has elite nodes but no elite encounter is eligible for configured elite layers")

    for layer in range(0, len(widths) - 2):
        if layer == int(config.get("chests", {}).get("min_layer", -1)) and config.get("chests", {}).get("full_layer", False):
            continue
        if not any(encounter_eligible(encounter, layer) for encounter in pools["combat"]):
            errors.append(f"{owner}: has no normal combat encounter eligible for layer {layer}")
    return pools


def validate_event_pool(events: Any, config: dict[str, Any], floor: dict[str, Any], errors: list[str]) -> list[dict[str, Any]]:
    owner = floor_label(floor)
    pool_id = floor.get("event_pool_id", "")
    can_generate_events = int(config.get("event_weight", 0)) > 0 or isinstance(config.get("events"), dict)
    if not can_generate_events:
        return []
    if not isinstance(events, list) or not events:
        errors.append(f"{owner}: can generate event rooms, but data/events/run_events.json is empty")
        return []

    pooled_events = events_for_pool(events, str(pool_id))
    if len(pooled_events) < 8:
        errors.append(f"{owner}: event pool '{pool_id}' should contain at least 8 events")
    if floor.get("index") == 1 and len(pooled_events) < 10:
        errors.append(f"{owner}: first-floor event pool '{pool_id}' should contain at least 10 events")

    seen: set[str] = set()
    for index, event in enumerate(pooled_events):
        event_owner = f"{owner} event[{index}]"
        event_id = event.get("id")
        if not isinstance(event_id, str) or not event_id:
            errors.append(f"{event_owner} must have a non-empty id")
            continue
        if event_id in seen:
            errors.append(f"{owner}: event id '{event_id}' is duplicated within pool '{pool_id}'")
        seen.add(event_id)
        choices = event.get("choices")
        if not isinstance(choices, list) or len(choices) < 2:
            errors.append(f"{owner}: event '{event_id}' should have at least two choices")
        elif not any(any(isinstance(effect, dict) and effect.get("type") == "skip" for effect in choice.get("effects", [])) for choice in choices if isinstance(choice, dict)):
            errors.append(f"{owner}: event '{event_id}' should provide a skip/no-op choice")
    return pooled_events


def validate_snapshot(seed: int, layers: list[list[str]], edges: list[list[list[int]]], config: dict[str, Any], encounter_pools: dict[str, list[dict[str, Any]]], event_count: int, floor: dict[str, Any], errors: list[str]) -> None:
    owner = floor_label(floor)
    if not layers:
        errors.append(f"{owner} seed {seed}: generated no layers")
        return
    pre_boss_layer = len(layers) - 2
    boss_layer = len(layers) - 1
    if layers[0] != ["combat"]:
        errors.append(f"{owner} seed {seed}: first layer must be exactly one combat node")
    if any(room != "rest" for room in layers[pre_boss_layer]):
        errors.append(f"{owner} seed {seed}: pre-boss layer must contain only rest nodes")
    if layers[boss_layer] != ["boss"]:
        errors.append(f"{owner} seed {seed}: boss layer must be exactly one boss node")

    counts = Counter(room for layer in layers for room in layer)
    shop_count = int(config.get("shop", {}).get("count", 0))
    chest_count = int(config.get("chests", {}).get("count", 0))
    elite_cfg = config.get("elites", {}) if isinstance(config.get("elites"), dict) else {}
    elite_min = int(elite_cfg.get("min", 0))
    elite_max = int(elite_cfg.get("max", 0))
    if counts["shop"] != shop_count:
        errors.append(f"{owner} seed {seed}: expected {shop_count} shop nodes, got {counts['shop']}")
    if counts["chest"] != chest_count:
        errors.append(f"{owner} seed {seed}: expected {chest_count} chest nodes, got {counts['chest']}")
    if counts["elite"] < elite_min or counts["elite"] > elite_max:
        errors.append(f"{owner} seed {seed}: elite node count {counts['elite']} outside {elite_min}..{elite_max}")
    fixed_events = config.get("events") if isinstance(config.get("events"), dict) else None
    if fixed_events is not None:
        event_min = int(fixed_events.get("min", 0))
        event_max = int(fixed_events.get("max", 0))
        if counts["event"] < event_min or counts["event"] > event_max:
            errors.append(f"{owner} seed {seed}: event node count {counts['event']} outside {event_min}..{event_max}")
    if counts["event"] > 0 and event_count <= 0:
        errors.append(f"{owner} seed {seed}: generated event nodes but the event pool is empty")

    chest_cfg = config.get("chests", {}) if isinstance(config.get("chests"), dict) else {}
    if chest_cfg.get("full_layer", False):
        chest_layer = int(chest_cfg.get("min_layer", -1))
        if 0 <= chest_layer < len(layers) and any(room != "chest" for room in layers[chest_layer]):
            errors.append(f"{owner} seed {seed}: configured full chest layer {chest_layer} is not all chests")

    for layer_index, room_types in enumerate(layers):
        for node_index, room_type in enumerate(room_types):
            if room_type not in ROOM_TYPES:
                errors.append(f"{owner} seed {seed}: unknown room type '{room_type}' at L{layer_index}:{node_index}")
            if room_type in ENCOUNTER_POOLS:
                if not any(encounter_eligible(encounter, layer_index) for encounter in encounter_pools[room_type]):
                    errors.append(f"{owner} seed {seed}: no {room_type} encounter is eligible for generated layer {layer_index}")
            if room_type == "event" and int(config.get("question_mark_combat_chance", 0)) > 0:
                if not any(encounter_eligible(encounter, layer_index) for encounter in encounter_pools["combat"]):
                    errors.append(f"{owner} seed {seed}: event node on layer {layer_index} can become combat, but no normal encounter is eligible")

    if len(edges) != len(layers) - 1:
        errors.append(f"{owner} seed {seed}: edge layer count does not match generated layers")
        return
    for layer_index, layer_edges in enumerate(edges):
        if len(layer_edges) != len(layers[layer_index]):
            errors.append(f"{owner} seed {seed}: L{layer_index} edge source count does not match node count")
        for source, targets in enumerate(layer_edges):
            if not targets:
                errors.append(f"{owner} seed {seed}: L{layer_index}:{source} has no outgoing edge")
            for target in targets:
                if target < 0 or target >= len(layers[layer_index + 1]):
                    errors.append(f"{owner} seed {seed}: L{layer_index}:{source} points outside L{layer_index + 1}")

    reachable: set[tuple[int, int]] = {(0, 0)}
    queue: deque[tuple[int, int]] = deque([(0, 0)])
    while queue:
        layer, node = queue.popleft()
        if layer >= len(edges):
            continue
        for target in edges[layer][node]:
            item = (layer + 1, target)
            if item not in reachable:
                reachable.add(item)
                queue.append(item)
    all_nodes = {(layer, node) for layer, room_types in enumerate(layers) for node in range(len(room_types))}
    unreachable = all_nodes - reachable
    if unreachable:
        sample = sorted(unreachable)[:5]
        errors.append(f"{owner} seed {seed}: generated unreachable nodes from start, sample={sample}")

    reverse: dict[tuple[int, int], list[tuple[int, int]]] = defaultdict(list)
    for layer, layer_edges in enumerate(edges):
        for source, targets in enumerate(layer_edges):
            for target in targets:
                reverse[(layer + 1, target)].append((layer, source))
    can_reach_boss: set[tuple[int, int]] = {(boss_layer, 0)}
    queue = deque([(boss_layer, 0)])
    while queue:
        item = queue.popleft()
        for previous in reverse.get(item, []):
            if previous not in can_reach_boss:
                can_reach_boss.add(previous)
                queue.append(previous)
    dead_ends = all_nodes - can_reach_boss
    if dead_ends:
        sample = sorted(dead_ends)[:5]
        errors.append(f"{owner} seed {seed}: generated nodes that cannot reach boss, sample={sample}")


def load_floors(errors: list[str]) -> list[dict[str, Any]]:
    try:
        root = load_json(FLOORS_PATH)
    except Exception as exc:  # noqa: BLE001
        errors.append(f"{rel(FLOORS_PATH)}: cannot read JSON: {exc}")
        return []
    if not isinstance(root, dict) or not isinstance(root.get("floors"), list):
        errors.append(f"{rel(FLOORS_PATH)}: root must be an object with a floors array")
        return []
    return [floor for floor in root["floors"] if isinstance(floor, dict) and floor.get("is_implemented", True)]


def validate_floor(floor: dict[str, Any], seeds: int, enemy_ids: set[str], events_root: Any, errors: list[str]) -> tuple[int, int, int, int, int]:
    map_path = resolve_floor_path(floor, "map_config_path")
    encounter_path = resolve_floor_path(floor, "encounter_table_path")
    owner = floor_label(floor)
    if map_path is None or encounter_path is None:
        errors.append(f"{owner}: implemented floor must define map_config_path and encounter_table_path")
        return (0, 0, 0, 0, 0)
    if not map_path.is_file():
        errors.append(f"{owner}: missing map config {map_path}")
        return (0, 0, 0, 0, 0)
    if not encounter_path.is_file():
        errors.append(f"{owner}: missing encounter table {encounter_path}")
        return (0, 0, 0, 0, 0)

    config = validate_act_config(load_json(map_path), floor, errors)
    if config is None:
        return (0, 0, 0, 0, 0)
    encounter_pools = validate_encounters(load_json(encounter_path), enemy_ids, config, floor, errors)
    floor_events = validate_event_pool(events_root, config, floor, errors)
    event_count = len(floor_events)

    for seed in range(1, max(1, seeds) + 1):
        snapshot_errors: list[str] = []
        layers, edges = generate_snapshot(seed, config, owner, snapshot_errors)
        errors.extend(snapshot_errors)
        validate_snapshot(seed, layers, edges, config, encounter_pools, event_count, floor, errors)

    widths = layer_widths(config)
    return (len(widths), len(encounter_pools["combat"]), len(encounter_pools["elite"]), len(encounter_pools["boss"]), event_count)


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate all implemented floor vertical slices.")
    parser.add_argument("--seeds", type=int, default=250, help="How many deterministic map seeds to validate per floor. Defaults to 250.")
    parser.add_argument("--floor", default="", help="Optional floor id to validate, for example floor1.")
    args = parser.parse_args()

    errors: list[str] = []
    floors = load_floors(errors)
    if args.floor:
        floors = [floor for floor in floors if floor.get("id") == args.floor]
        if not floors:
            errors.append(f"No implemented floor with id '{args.floor}'")
    enemy_ids = set(load_index(DATA_DIR / "enemies").keys())
    events_root = load_json(EVENTS_PATH) if EVENTS_PATH.is_file() else []

    summaries: list[tuple[str, int, int, int, int, int]] = []
    for floor in sorted(floors, key=lambda item: int(item.get("index", 0))):
        layers, combat, elite, boss, events = validate_floor(floor, max(1, args.seeds), enemy_ids, events_root, errors)
        summaries.append((str(floor.get("id", "<unknown>")), layers, combat, elite, boss, events))

    if errors:
        print(f"Floor vertical slice validation failed with {len(errors)} error(s):")
        for item in errors:
            print(f" - {item}")
        return 1

    detail = "; ".join(
        f"{floor_id}: {layers} layers, {combat} combat, {elite} elite, {boss} boss, {events} events"
        for floor_id, layers, combat, elite, boss, events in summaries
    )
    print(f"Floor vertical slices OK: {len(summaries)} floor(s), {max(1, args.seeds)} map seeds each; {detail}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
