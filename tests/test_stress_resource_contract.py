#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"stress resource contract failed: {message}")


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


required_source_tokens = {
    "src/cards/CardDefinition.hpp": ["stressCost"],
    "src/data/parsers/CardDefinitionParser.cpp": ['optionalInt("stress_cost", 0)', "upgrade.stressCost"],
    "src/combat/CardStressCost.hpp": ["directCost", "conversionCost", "totalCost"],
    "src/combat/CardPlayValidator.cpp": ["CardStressCost::totalCost"],
    "src/combat/CardPlaySystem.cpp": ["CardStressCost::directCost", "StressRules::applyDelta"],
    "src/preview/CardPreview.hpp": ["sourceStressAfterMinimum", "wouldCollapseFromStress"],
    "src/preview/CardPreviewSystem.cpp": ["sourceStressDeltaMinimum", "wouldCollapseFromStress"],
    "src/ui/CardVisualInstance.cpp": ['"S" + std::to_string(model_.stressCost)', "stressPreviewLabel"],
}
for path, tokens in required_source_tokens.items():
    text = read(path)
    for token in tokens:
        if token not in text:
            fail(f"{path} is missing {token}")

expected_cards = {
    "rusted_red_momentum": "rusted_knight",
    "herbalist_distillation": "herbalist",
    "replicant_feedback_loop": "replicant",
    "merchant_liquid_assets": "merchant",
    "monk_ash_mirror": "monk",
}
found = {}
for path in (ROOT / "data/cards").glob("*.json"):
    for card in json.loads(path.read_text(encoding="utf-8")):
        if card.get("id") in expected_cards:
            found[card["id"]] = card

for card_id, owner in expected_cards.items():
    card = found.get(card_id)
    if card is None:
        fail(f"missing stress-spending card {card_id}")
    if card.get("owner_actor") != owner:
        fail(f"{card_id} belongs to the wrong actor")
    cost = card.get("stress_cost", 0)
    if not isinstance(cost, int) or cost <= 0:
        fail(f"{card_id} must spend positive stress")
    upgraded_cost = card.get("upgrade", {}).get("stress_cost", cost)
    if upgraded_cost < 0 or upgraded_cost > cost:
        fail(f"{card_id} upgrade must not increase stress cost")

for language in ("en", "ru"):
    strings = json.loads((ROOT / f"data/localization/{language}/core.json").read_text(encoding="utf-8"))
    for key in (
        "card.upgrade.summary.stress_cost",
        "ui.card_stress_preview.after",
        "ui.card_stress_preview.collapse",
    ):
        if key not in strings:
            fail(f"{language} localization is missing {key}")

print(f"Stress resource contract passed ({len(found)} non-psychopath stress-spending cards)")
