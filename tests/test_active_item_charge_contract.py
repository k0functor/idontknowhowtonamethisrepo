#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"active item charge contract failed: {message}")


items = json.loads((ROOT / "data" / "active_items" / "foundation_items.json").read_text(encoding="utf-8"))
if len(items) < 7:
    fail("foundation active-item catalogue unexpectedly shrank")

for item in items:
    item_id = item.get("id", "<missing>")
    maximum = item.get("max_charge")
    cost = item.get("charge_cost")
    starting = item.get("starting_charge")
    rates = [item.get("combat_charge"), item.get("elite_charge"), item.get("boss_charge")]
    if not isinstance(maximum, int) or maximum <= 0:
        fail(f"{item_id}: invalid max_charge")
    if not isinstance(cost, int) or not 0 < cost <= maximum:
        fail(f"{item_id}: invalid charge_cost")
    if not isinstance(starting, int) or not 0 <= starting <= maximum:
        fail(f"{item_id}: invalid starting_charge")
    if any(not isinstance(rate, int) or rate < 0 for rate in rates):
        fail(f"{item_id}: room charge rates must be non-negative integers")
    if rates[1] < rates[0] or rates[2] < rates[1]:
        fail(f"{item_id}: elite and boss rooms must not charge less than ordinary combat")

system_hpp = (ROOT / "src" / "active_items" / "ActiveItemSystem.hpp").read_text(encoding="utf-8")
system_cpp = (ROOT / "src" / "active_items" / "ActiveItemSystem.cpp").read_text(encoding="utf-8")
parser_cpp = (ROOT / "src" / "data" / "parsers" / "ActiveItemDefinitionParser.cpp").read_text(encoding="utf-8")
acquisition_cpp = (ROOT / "src" / "active_items" / "ActiveItemAcquisitionSystem.cpp").read_text(encoding="utf-8")
flow_cpp = (ROOT / "src" / "flow" / "GameFlowController.cpp").read_text(encoding="utf-8")
comparison_cpp = (ROOT / "src" / "ui" / "ActiveItemComparisonView.cpp").read_text(encoding="utf-8")

for token in (
    "missingChargeForUse",
    "normalCombatRoomsUntilUsable",
    "addCharge",
):
    if token not in system_hpp or token not in system_cpp:
        fail(f"missing shared charge API: {token}")

for token in ("starting_charge", "combat_charge", "elite_charge", "boss_charge"):
    if token not in parser_cpp:
        fail(f"parser does not read {token}")

if "definition.startingCharge" not in acquisition_cpp:
    fail("replacing an active item must use its data-driven starting charge")
if "normalCombatRoomsUntilUsable" not in flow_cpp:
    fail("HUD must forecast remaining ordinary fights before the item is usable")
if "definition.chargeCost" not in flow_cpp or "useMarkerX" not in flow_cpp:
    fail("HUD must distinguish use threshold from maximum stored charge")
if "active_item.compare.recharge" not in comparison_cpp:
    fail("active-item comparison must show recharge profile")

for locale in ("en", "ru"):
    strings = json.loads((ROOT / "data" / "localization" / locale / "active_items.json").read_text(encoding="utf-8"))
    for key in (
        "active_item.charge.ready",
        "active_item.charge.rooms_short",
        "active_item.charge.no_combat_gain",
        "active_item.compare.recharge",
    ):
        if not strings.get(key):
            fail(f"{locale}: missing {key}")

print("Active item charge contract OK")
