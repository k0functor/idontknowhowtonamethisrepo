#!/usr/bin/env python3
from __future__ import annotations

import copy
import importlib.util
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR_PATH = ROOT / "tools" / "validate_run_save_files.py"
TMP_ROOT = ROOT / "tests" / ".tmp_run_save_room_integration"


def load_validator():
    spec = importlib.util.spec_from_file_location("run_save_validator", VALIDATOR_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load run save validator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def base_save(node_type: str, node_state: str, phase: str, pending_type: str) -> dict:
    reward = {
        "source_node_type": node_type,
        "options": [],
    }
    shop = {
        "mode": "merchant_rest" if pending_type == "merchant_rest" else "shop",
        "offers": [],
        "card_removal_price": 75,
        "card_removal_used": False,
        "max_card_purchases": 1 if pending_type == "merchant_rest" else 0,
        "card_purchases_made": 0,
        "merchant_rest_card_shop_open": pending_type == "merchant_rest",
    }
    return {
        "version": 3,
        "archetype_id": "integration_tester",
        "difficulty_id": "normal",
        "archetype_mechanic_id": "default",
        "challenge_id": "",
        "phase": phase,
        "seed": 1903,
        "random_state": "integration-state",
        "gold": 120,
        "act": 1,
        "current_floor_id": "floor1",
        "current_floor_index": 1,
        "next_floor_id": "floor2",
        "act_completed": False,
        "completed_act": 0,
        "run_completion_type": "in_progress",
        "defeated_boss_enemy_ids": [],
        "enemy_hp_multiplier": 1.0,
        "enemy_damage_multiplier": 1.0,
        "gold_reward_multiplier": 1.0,
        "deck_card_ids": ["integration_card"],
        "upgraded_deck_indices": [],
        "relic_ids": [],
        "active_item": {"item_id": "", "charge": 0},
        "consumable_ids": [],
        "max_consumables": 3,
        "actor_definition_ids": ["integration_tester"],
        "reward_card_pool_ids": [],
        "actor_states": [
            {
                "definition_id": "integration_tester",
                "current_hp": 40,
                "max_hp": 40,
                "stress": 0,
                "max_stress": 200,
                "resolve_check_triggered": False,
                "trait_ids": [],
                "relic_ids": [],
            }
        ],
        "map": {
            "current_node_id": 7,
            "nodes": [
                {
                    "id": 7,
                    "type": node_type,
                    "state": node_state,
                    "position": {"x": 0.0, "y": 0.0},
                    "next": [],
                }
            ],
        },
        "stats": {},
        "pending_room": {
            "type": pending_type,
            "node_id": 7,
            "reward": reward,
            "shop": shop,
            "event_id": "integration_event" if pending_type == "event" else "",
        },
    }


def validate_fixture(validator, name: str, payload: dict) -> list[str]:
    path = TMP_ROOT / f"{name}.json"
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    return validator.validate_save(path, set())


def main() -> int:
    validator = load_validator()
    shutil.rmtree(TMP_ROOT, ignore_errors=True)
    TMP_ROOT.mkdir(parents=True)

    valid_cases = {
        "combat_reward": base_save("combat", "completed", "reward", "combat_reward"),
        "chest_reward": base_save("chest", "current", "chest", "chest_reward"),
        "shop": base_save("shop", "current", "shop", "shop"),
        "merchant_rest": base_save("rest", "current", "rest", "merchant_rest"),
        "event": base_save("event", "current", "event", "event"),
    }

    failures: list[str] = []
    try:
        for name, payload in valid_cases.items():
            errors = validate_fixture(validator, name, payload)
            if errors:
                failures.append(f"valid {name} fixture failed: {'; '.join(errors)}")

        invalid_phase = copy.deepcopy(valid_cases["shop"])
        invalid_phase["phase"] = "map"
        if not validate_fixture(validator, "invalid_phase", invalid_phase):
            failures.append("pending shop with map phase was accepted")

        invalid_node_type = copy.deepcopy(valid_cases["event"])
        invalid_node_type["map"]["nodes"][0]["type"] = "combat"
        if not validate_fixture(validator, "invalid_node_type", invalid_node_type):
            failures.append("event pending state attached to a combat node was accepted")

        invalid_reward_state = copy.deepcopy(valid_cases["combat_reward"])
        invalid_reward_state["map"]["nodes"][0]["state"] = "current"
        if not validate_fixture(validator, "invalid_reward_state", invalid_reward_state):
            failures.append("combat reward on a non-completed node was accepted")

        invalid_event = copy.deepcopy(valid_cases["event"])
        invalid_event["pending_room"]["event_id"] = ""
        if not validate_fixture(validator, "invalid_event", invalid_event):
            failures.append("pending event without event_id was accepted")

        serializer_source = (ROOT / "src" / "save" / "RunStateSerializer.cpp").read_text(encoding="utf-8")
        for token in (
            'return "combat_reward";',
            'return "chest_reward";',
            'return "shop";',
            'return "merchant_rest";',
            'return "event";',
            '{"pending_room", pendingRoomToJson(normalized.pendingRoom)}',
        ):
            if token not in serializer_source:
                failures.append(f"serializer contract is missing {token}")
    finally:
        shutil.rmtree(TMP_ROOT, ignore_errors=True)

    if failures:
        print(f"Run save room integration failed with {len(failures)} error(s):")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("Run save room integration OK: 5 valid room states and 4 invalid regressions checked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
