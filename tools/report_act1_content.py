#!/usr/bin/env python3
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]


def load_json(relative_path: str) -> Any:
    with (ROOT / relative_path).open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def rarity_counts(items: list[dict[str, Any]]) -> Counter[str]:
    return Counter(str(item.get("rarity", "unknown")) for item in items)


def print_counter(title: str, counter: Counter[str]) -> None:
    parts = [f"{key}={counter[key]}" for key in sorted(counter)]
    print(f"{title}: {', '.join(parts) if parts else 'none'}")


def count_event_choice_effects(events: list[dict[str, Any]]) -> Counter[str]:
    result: Counter[str] = Counter()
    for event in events:
        for choice in event.get("choices", []):
            for effect in choice.get("effects", []):
                effect_type = effect.get("type")
                if isinstance(effect_type, str):
                    result[effect_type] += 1
    return result


def count_combat_effects(items: list[dict[str, Any]]) -> Counter[str]:
    result: Counter[str] = Counter()
    for item in items:
        for effect in item.get("effects", []):
            effect_type = effect.get("type")
            if isinstance(effect_type, str):
                result[effect_type] += 1
    return result


def count_relic_triggers(relics: list[dict[str, Any]]) -> Counter[str]:
    result: Counter[str] = Counter()
    for relic in relics:
        for trigger in relic.get("triggers", []):
            event_type = trigger.get("event")
            if isinstance(event_type, str):
                result[event_type] += 1
    return result


def main() -> int:
    relics = load_json("data/relics/test_relics.json")
    consumables = load_json("data/consumables/potions.json")
    events = load_json("data/events/run_events.json")

    print("Act 1 content report")
    print(f"Relics: {len(relics)}")
    print_counter("Relic rarities", rarity_counts(relics))
    print_counter("Relic triggers", count_relic_triggers(relics))
    print()

    print(f"Consumables: {len(consumables)}")
    print_counter("Consumable rarities", rarity_counts(consumables))
    print_counter("Consumable combat effects", count_combat_effects(consumables))
    print()

    print(f"Events: {len(events)}")
    event_choice_count = sum(len(event.get("choices", [])) for event in events)
    print(f"Event choices: {event_choice_count}")
    print_counter("Event effects", count_event_choice_effects(events))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
