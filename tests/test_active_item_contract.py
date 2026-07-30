#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"active item contract failed: {message}")


items_path = ROOT / "data" / "active_items" / "foundation_items.json"
items = json.loads(items_path.read_text(encoding="utf-8"))
if not isinstance(items, list) or not items:
    fail("foundation item data must be a non-empty array")

known_contexts = {"map", "combat", "reward", "shop", "chest", "event", "rest"}
known_effects = {"heal_party", "gain_gold", "reroll_offers", "skip_enemy_turn", "create_consumable", "reroll_map_choices", "copy_card", "stabilize_stress"}
ids: set[str] = set()
for item in items:
    item_id = item.get("id")
    if not isinstance(item_id, str) or not item_id:
        fail("every item needs a non-empty id")
    if item_id in ids:
        fail(f"duplicate item id {item_id}")
    ids.add(item_id)
    max_charge = item.get("max_charge")
    cost = item.get("charge_cost")
    if not isinstance(max_charge, int) or max_charge <= 0:
        fail(f"{item_id}: max_charge must be positive")
    if not isinstance(cost, int) or not 1 <= cost <= max_charge:
        fail(f"{item_id}: charge_cost must be in 1..max_charge")
    shop_price = item.get("shop_price")
    if not isinstance(shop_price, int) or shop_price < 0:
        fail(f"{item_id}: shop_price must be a non-negative integer")
    for flag in ("reward_eligible", "shop_eligible"):
        if not isinstance(item.get(flag), bool):
            fail(f"{item_id}: {flag} must be boolean")
    if item.get("shop_eligible") and shop_price <= 0:
        fail(f"{item_id}: shop eligible items need a positive shop price")
    contexts = item.get("use_contexts")
    if not isinstance(contexts, list) or not contexts or any(c not in known_contexts for c in contexts):
        fail(f"{item_id}: invalid use contexts")
    effects = item.get("effects")
    if not isinstance(effects, list) or not effects:
        fail(f"{item_id}: effects must be non-empty")
    for effect in effects:
        if effect.get("type") not in known_effects:
            fail(f"{item_id}: unknown effect type")
        if not isinstance(effect.get("amount"), int) or effect["amount"] <= 0:
            fail(f"{item_id}: effect amount must be positive")

for locale in ("en", "ru"):
    localization = json.loads((ROOT / "data" / "localization" / locale / "active_items.json").read_text(encoding="utf-8"))
    for item in items:
        for field in ("name", "description"):
            if item[field] not in localization:
                fail(f"{locale}: missing {item[field]}")

run_state = (ROOT / "src" / "run" / "RunState.hpp").read_text(encoding="utf-8")
serializer = (ROOT / "src" / "save" / "RunStateSerializer.cpp").read_text(encoding="utf-8")
flow = (ROOT / "src" / "flow" / "GameFlowController.cpp").read_text(encoding="utf-8")
controller = (ROOT / "src" / "run" / "RunController.cpp").read_text(encoding="utf-8")
for token, owner in (
    ("ActiveItemState activeItem", run_state),
    ('{"active_item"', serializer),
    ("KEY_SPACE", flow),
    ("renderActiveItemHud", flow),
    ("addCombatRoomCharge", controller),
):
    if token not in owner:
        fail(f"missing source contract token: {token}")

if "reroll_die" not in ids:
    fail("reroll_die must be present in the foundation item set")

print(f"Active item contract OK: {len(items)} item(s)")
