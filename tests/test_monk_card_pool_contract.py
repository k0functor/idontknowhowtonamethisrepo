#!/usr/bin/env python3
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CARDS_PATH = ROOT / "data/cards/monk_cards.json"

cards = json.loads(CARDS_PATH.read_text(encoding="utf-8"))
by_id = {card["id"]: card for card in cards}
rarities = Counter(card["rarity"] for card in cards)

assert len(cards) >= 30, f"Monk needs at least 30 cards, found {len(cards)}"
assert rarities["uncommon"] >= 6, rarities
assert rarities["rare"] >= 4, rarities

expected = {
    "monk_cinder_crossing",
    "monk_ashen_passage",
    "monk_smoke_ignition",
    "monk_three_aspects",
    "monk_still_point",
    "monk_wheel_without_end",
}
assert expected <= by_id.keys(), f"missing Monk expansion cards: {sorted(expected - by_id.keys())}"

# The three uncommon transition cards must form a closed stance cycle.
cycle = {
    "monk_cinder_crossing": ("stance_flame", "stance_ash"),
    "monk_ashen_passage": ("stance_ash", "stance_smoke"),
    "monk_smoke_ignition": ("stance_smoke", "stance_flame"),
}
for card_id, (payoff_stance, destination_stance) in cycle.items():
    card = by_id[card_id]
    assert card["rarity"] == "uncommon"
    scalings = [effect.get("scaling", {}) for effect in card["effects"]]
    assert any(scaling.get("status") == payoff_stance for scaling in scalings), card_id
    assert any(
        effect.get("type") == "enter_stance" and effect.get("status") == destination_stance
        for effect in card["effects"]
    ), card_id

# Rare cards must support three distinct stance strategies instead of being generic damage upgrades.
three_aspects = by_id["monk_three_aspects"]
assert {
    effect.get("scaling", {}).get("status")
    for effect in three_aspects["effects"]
    if effect.get("scaling")
} == {"stance_flame", "stance_ash", "stance_smoke"}

still_point = by_id["monk_still_point"]
assert {"retain", "exhaust"} <= set(still_point.get("keywords", []))
assert all(effect.get("value", {}).get("amount") == 0 for effect in still_point["effects"])

wheel = by_id["monk_wheel_without_end"]
wheel_stances = [
    effect.get("status")
    for effect in wheel["effects"]
    if effect.get("type") == "enter_stance"
]
assert wheel_stances == ["stance_flame", "stance_ash", "stance_smoke"], wheel_stances
assert "retain" in wheel["upgrade"].get("keywords", [])

# Every new card needs a meaningful upgrade.
for card_id in expected:
    upgrade = by_id[card_id].get("upgrade", {})
    assert upgrade, f"{card_id} has no upgrade"
    assert any(key in upgrade for key in ("effects", "energy_cost", "stress_cost", "keywords")), card_id

print(
    "Monk card pool contract passed: "
    f"{len(cards)} cards, {rarities['uncommon']} uncommon, {rarities['rare']} rare"
)
