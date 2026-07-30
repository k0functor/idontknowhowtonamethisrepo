#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"active item acquisition contract failed: {message}")


items = json.loads((ROOT / "data" / "active_items" / "foundation_items.json").read_text(encoding="utf-8"))
if not any(item.get("reward_eligible") for item in items):
    fail("at least one active item must be reward eligible")
if not any(item.get("shop_eligible") for item in items):
    fail("at least one active item must be shop eligible")

reward_tables = json.loads((ROOT / "data" / "rewards" / "reward_tables.json").read_text(encoding="utf-8"))
for node in ("elite", "boss", "chest"):
    chance = reward_tables["nodes"][node].get("active_item_chance_percent")
    if not isinstance(chance, int) or chance <= 0:
        fail(f"{node} must have a positive active item chance")

shop_tables = json.loads((ROOT / "data" / "shop" / "shop_tables.json").read_text(encoding="utf-8"))
chance = shop_tables.get("active_item_offer_chance_percent")
if not isinstance(chance, int) or chance <= 0:
    fail("shops must have a positive active item offer chance")

sources = {
    "acquisition": (ROOT / "src" / "active_items" / "ActiveItemAcquisitionSystem.cpp").read_text(encoding="utf-8"),
    "reward_scene": (ROOT / "src" / "scenes" / "RewardScene.cpp").read_text(encoding="utf-8"),
    "shop_scene": (ROOT / "src" / "scenes" / "ShopScene.cpp").read_text(encoding="utf-8"),
    "serializer": (ROOT / "src" / "save" / "RunStateSerializer.cpp").read_text(encoding="utf-8"),
    "flow": (ROOT / "src" / "flow" / "GameFlowController.cpp").read_text(encoding="utf-8"),
}
for token, owner in (
    ("chooseReward", sources["acquisition"]),
    ("chooseShopOffer", sources["acquisition"]),
    ("ActiveItemComparisonView", sources["reward_scene"]),
    ("ActiveItemComparisonView", sources["shop_scene"]),
    ("active_item_id", sources["serializer"]),
    ("ShopOfferType::ActiveItem", sources["serializer"]),
    ("activeItemChancePercent", sources["flow"]),
):
    if token not in owner:
        fail(f"missing source contract token: {token}")

for locale in ("en", "ru"):
    active = json.loads((ROOT / "data" / "localization" / locale / "active_items.json").read_text(encoding="utf-8"))
    run = json.loads((ROOT / "data" / "localization" / locale / "run.json").read_text(encoding="utf-8"))
    for key in (
        "active_item.compare.title",
        "active_item.compare.current",
        "active_item.compare.offered",
        "active_item.compare.keep",
        "active_item.compare.equip",
        "active_item.compare.buy_equip",
    ):
        if not active.get(key):
            fail(f"{locale}: missing {key}")
    for key in ("reward.take_active_item", "shop.kind.active_item", "shop.status.active_item_equipped"):
        if not run.get(key):
            fail(f"{locale}: missing {key}")

print("Active item acquisition contract OK")
