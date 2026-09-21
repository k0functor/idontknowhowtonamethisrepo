#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"reroll die contract failed: {message}")


items = json.loads((ROOT / "data" / "active_items" / "foundation_items.json").read_text(encoding="utf-8"))
die = next((item for item in items if item.get("id") == "reroll_die"), None)
if die is None:
    fail("reroll_die item is missing")
if die.get("use_contexts") != ["reward", "shop", "chest"]:
    fail("reroll_die must be limited to reward, shop, and chest contexts")
if die.get("max_charge") != 6 or die.get("charge_cost") != 4:
    fail("reroll_die must bank up to six charge and spend four per use")
if die.get("starting_charge") != 1:
    fail("reroll_die must provide a small starting charge after acquisition")
if die.get("effects") != [{"type": "reroll_offers", "amount": 1}]:
    fail("reroll_die must use the reroll_offers effect")

reroll_cpp = (ROOT / "src" / "active_items" / "ActiveItemRerollSystem.cpp").read_text(encoding="utf-8")
flow_cpp = (ROOT / "src" / "flow" / "GameFlowController.cpp").read_text(encoding="utf-8")
reward_cpp = (ROOT / "src" / "scenes" / "RewardScene.cpp").read_text(encoding="utf-8")
shop_cpp = (ROOT / "src" / "scenes" / "ShopScene.cpp").read_text(encoding="utf-8")

for token, owner in (
    ("rerollReward", reroll_cpp),
    ("rerollShop", reroll_cpp),
    ("RewardOptionType::Gold", reroll_cpp),
    ("ShopOfferType::CardRemoval", reroll_cpp),
    ("reward_selection_locked", flow_cpp),
    ("setPendingCombatReward", flow_cpp),
    ("setPendingChestReward", flow_cpp),
    ("setPendingShop", flow_cpp),
    ("KEY_SPACE", reward_cpp),
    ("KEY_SPACE", shop_cpp),
):
    if token not in owner:
        fail(f"missing source contract token: {token}")

for locale in ("en", "ru"):
    localization = json.loads((ROOT / "data" / "localization" / locale / "active_items.json").read_text(encoding="utf-8"))
    for key in (
        "active_item.reroll_die.name",
        "active_item.reroll_die.description",
        "active_item.feedback.rerolled_reward",
        "active_item.feedback.rerolled_shop",
        "active_item.feedback.reward_selection_locked",
    ):
        if not localization.get(key):
            fail(f"{locale}: missing or empty localization {key}")

print("Reroll die contract OK")
