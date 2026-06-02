#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import random
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
ACT_PATH = ROOT / "data/run/acts/act1.json"

ROOM_GLYPHS = {
    "combat": "C",
    "elite": "E",
    "event": "?",
    "shop": "$",
    "chest": "T",
    "rest": "R",
    "boss": "B",
}


def load_act() -> dict[str, Any]:
    with ACT_PATH.open("r", encoding="utf-8-sig") as file:
        data = json.load(file)
    if not isinstance(data, dict):
        raise SystemExit(f"{ACT_PATH.relative_to(ROOT)}: root must be an object")
    return data


def layer_widths(config: dict[str, Any]) -> list[int]:
    counts = config.get("layer_node_counts")
    if isinstance(counts, list):
        return [int(count) for count in counts]

    layer_count = int(config.get("layer_count", 8))
    middle_min = int(config.get("middle_min_nodes", 2))
    middle_max = int(config.get("middle_max_nodes", 4))
    rng = random.Random(1)
    return [
        1 if layer in {0, layer_count - 2, layer_count - 1} else rng.randint(middle_min, middle_max)
        for layer in range(layer_count)
    ]


def collect_slots(layers: list[list[str]], min_layer: int, max_layer: int, used: set[tuple[int, int]]) -> list[tuple[int, int]]:
    result: list[tuple[int, int]] = []
    for layer in range(max(0, min_layer), min(max_layer, len(layers) - 1) + 1):
        for index in range(len(layers[layer])):
            slot = (layer, index)
            if slot not in used:
                result.append(slot)
    return result


def place_specials(
    layers: list[list[str]],
    used: set[tuple[int, int]],
    candidates: list[tuple[int, int]],
    count: int,
    room_type: str,
    rng: random.Random,
    label: str,
) -> None:
    if count <= 0:
        return
    if count > len(candidates):
        raise SystemExit(f"Cannot place {label}: requested {count}, candidates {len(candidates)}")

    for _ in range(count):
        choice_index = rng.randrange(len(candidates))
        layer, index = candidates.pop(choice_index)
        layers[layer][index] = room_type
        used.add((layer, index))


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


def connect_layers(widths: list[int], rng: random.Random, extra_connection_chance: int) -> list[list[list[int]]]:
    edges: list[list[list[int]]] = []
    extra_probability = max(0, min(100, extra_connection_chance)) / 100.0

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
            if selected[source]:
                continue
            selected[source].append(rng.choice(adjacent_target_indices(source, from_count, to_count)))

        for source in range(from_count):
            for target in adjacent_target_indices(source, from_count, to_count):
                if target not in selected[source] and rng.random() < extra_probability:
                    selected[source].append(target)

        for targets in selected:
            targets.sort()
        edges.append(selected)

    return edges


def generate_snapshot(seed: int, config: dict[str, Any]) -> tuple[list[list[str]], list[list[list[int]]]]:
    rng = random.Random(seed)
    widths = layer_widths(config)
    layer_count = len(widths)
    pre_boss_layer = layer_count - 2
    boss_layer = layer_count - 1

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
            combat_weight = int(config.get("combat_weight", 70))
            event_weight = int(config.get("event_weight", 30))
            total = max(1, combat_weight + event_weight)
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
        if not isinstance(item, dict):
            continue
        candidates = collect_slots(layers, int(item.get("min_layer", 1)), int(item.get("max_layer", pre_boss_layer - 1)), used)
        place_specials(layers, used, candidates, int(item.get("count", 0)), room_type, rng, key)

    elites = config.get("elites", {})
    if isinstance(elites, dict):
        count = rng.randint(int(elites.get("min", 0)), int(elites.get("max", 0)))
        candidates = collect_slots(layers, int(elites.get("min_layer", 1)), int(elites.get("max_layer", pre_boss_layer - 1)), used)
        place_specials(layers, used, candidates, count, "elite", rng, "elites")

    events = config.get("events")
    if isinstance(events, dict):
        count = rng.randint(int(events.get("min", 0)), int(events.get("max", 0)))
        candidates = collect_slots(layers, int(events.get("min_layer", 1)), int(events.get("max_layer", pre_boss_layer - 1)), used)
        place_specials(layers, used, candidates, count, "event", rng, "events")

    edges = connect_layers([len(layer) for layer in layers], rng, int(config.get("extra_connection_chance", 0)))
    return layers, edges


def main() -> int:
    parser = argparse.ArgumentParser(description="Print a deterministic act 1 map snapshot for content review.")
    parser.add_argument("--seed", type=int, default=1, help="Snapshot seed. Defaults to 1.")
    args = parser.parse_args()

    config = load_act()
    layers, edges = generate_snapshot(args.seed, config)

    print(f"Act: {config.get('id', 'act1')}  seed={args.seed}")
    print("Legend: C=combat E=elite ?=event $=shop T=chest R=rest B=boss")
    for layer_index, room_types in enumerate(layers):
        glyphs = " ".join(ROOM_GLYPHS.get(room_type, "!") for room_type in room_types)
        print(f"L{layer_index:02d}: {glyphs}")
    print("Edges:")
    for layer_index, layer_edges in enumerate(edges):
        parts = [f"{source}->{','.join(str(target) for target in targets)}" for source, targets in enumerate(layer_edges)]
        print(f"L{layer_index:02d}->L{layer_index + 1:02d}: {'; '.join(parts)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
