#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
errors = []


def require(condition: bool, message: str) -> None:
    if not condition:
        errors.append(message)


status_cards = {card["id"]: card for card in json.loads((ROOT / "data/cards/status_cards.json").read_text(encoding="utf-8"))}
burn = status_cards.get("burn_status", {})
wound = status_cards.get("wound_status", {})
require({"unplayable", "ethereal"}.issubset(set(burn.get("keywords", []))), "burn_status must be unplayable and ethereal")
require(any(effect.get("type") == "damage" and effect.get("target") == "self" for effect in burn.get("effects", [])), "burn_status must carry an end-of-turn self-damage effect")
require(wound.get("keywords") == ["unplayable"], "wound_status must only be unplayable")
require(wound.get("effects") == [], "wound_status must clog the hand without an unreachable play effect")

monk_cards = {card["id"]: card for card in json.loads((ROOT / "data/cards/monk_cards.json").read_text(encoding="utf-8"))}
for card_id in ("monk_enter_flame", "monk_enter_ash", "monk_enter_smoke"):
    require("innate" in monk_cards.get(card_id, {}).get("keywords", []), f"{card_id} must exercise the innate opening-hand mechanic")

hand_header = (ROOT / "src/cards/Hand.hpp").read_text(encoding="utf-8")
draw_source = (ROOT / "src/cards/DrawSystem.cpp").read_text(encoding="utf-8")
turn_source = (ROOT / "src/combat/PlayerTurnSystem.cpp").read_text(encoding="utf-8")
play_source = (ROOT / "src/combat/CardPlaySystem.cpp").read_text(encoding="utf-8")
require("MaximumSize = 10" in hand_header, "Hand must define a ten-card maximum")
require("drawOpeningHand" in draw_source and "CardKeyword::Innate" in draw_source, "DrawSystem must draw innate cards into the opening hand")
require("CardKeyword::Ethereal" in turn_source, "end-turn cleanup must recognize ethereal cards")
require("card.temporary || ethereal" in turn_source, "unplayed temporary cards and ethereal cards must exhaust")
require("removedCard->temporary" in play_source, "played temporary cards must exhaust")
require("resolveEndOfTurnStatusCards" in turn_source, "status cards must have an explicit end-of-turn resolution path")

if errors:
    for error in errors:
        print(f"[FAIL] {error}")
    raise SystemExit(1)

print("Card lifecycle contract passed")
