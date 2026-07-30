#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"active item expansion contract failed: {message}")


items = json.loads((ROOT / "data" / "active_items" / "foundation_items.json").read_text(encoding="utf-8"))
by_id = {item.get("id"): item for item in items}
expected = {
    "hourglass": ("skip_enemy_turn", {"combat"}, 5),
    "alchemist_flask": ("create_consumable", {"map", "reward", "shop", "chest", "event", "rest"}, 3),
    "compass": ("reroll_map_choices", {"map"}, 4),
    "mirror": ("copy_card", {"reward", "shop"}, 5),
}
for item_id, (effect, contexts, charge) in expected.items():
    item = by_id.get(item_id)
    if item is None:
        fail(f"missing {item_id}")
    if set(item.get("use_contexts", [])) != contexts:
        fail(f"{item_id} has wrong use contexts")
    if item.get("max_charge") != charge or item.get("charge_cost") != charge:
        fail(f"{item_id} must consume a full charge")
    effects = item.get("effects", [])
    if len(effects) != 1 or effects[0].get("type") != effect:
        fail(f"{item_id} has wrong effect")
    if not item.get("reward_eligible") or not item.get("shop_eligible"):
        fail(f"{item_id} must be obtainable")

sources = {
    "context": (ROOT / "src" / "active_items" / "ActiveItemContextSystem.cpp").read_text(encoding="utf-8"),
    "combat": (ROOT / "src" / "scenes" / "CombatScene.cpp").read_text(encoding="utf-8"),
    "turn": (ROOT / "src" / "combat" / "TurnSystem.cpp").read_text(encoding="utf-8"),
    "reward": (ROOT / "src" / "scenes" / "RewardScene.cpp").read_text(encoding="utf-8"),
    "shop": (ROOT / "src" / "scenes" / "ShopScene.cpp").read_text(encoding="utf-8"),
    "flow": (ROOT / "src" / "flow" / "GameFlowController.cpp").read_text(encoding="utf-8"),
}
for token, owner in (
    ("createRandomConsumable", sources["context"]),
    ("rerollAvailableMapNodes", sources["context"]),
    ("copyCard", sources["context"]),
    ("SkipEnemyTurn", sources["combat"]),
    ("skipNextEnemyTurn_", sources["combat"]),
    ("active_item.hint.hourglass_armed", sources["combat"]),
    ("skipEnemyActions", sources["turn"]),
    ("onCopyCard_", sources["reward"]),
    ("onCopyCard_", sources["shop"]),
    ("copyCardWithActiveItem", sources["flow"]),
):
    if token not in owner:
        fail(f"missing source contract token: {token}")

for locale in ("en", "ru"):
    active = json.loads((ROOT / "data" / "localization" / locale / "active_items.json").read_text(encoding="utf-8"))
    for item_id in expected:
        for suffix in ("name", "description"):
            key = f"active_item.{item_id}.{suffix}"
            if not active.get(key):
                fail(f"{locale}: missing {key}")
    for key in (
        "active_item.feedback.created_consumable",
        "active_item.feedback.map_rerolled",
        "active_item.feedback.card_copied",
        "active_item.feedback.enemy_turn_skipped",
        "active_item.hint.copy_selected",
    ):
        if not active.get(key):
            fail(f"{locale}: missing {key}")

print("Active item expansion contract OK")
