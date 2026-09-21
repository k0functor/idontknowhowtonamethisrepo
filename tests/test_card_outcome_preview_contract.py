#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, *needles: str) -> str:
    text = (ROOT / path).read_text(encoding="utf-8")
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError(f"{path} is missing: {', '.join(missing)}")
    return text


def main() -> int:
    require(
        "src/preview/CardOutcomePreview.hpp",
        "modifiedDamage",
        "hpDamage",
        "block",
        "healing",
        "requiresTargetSelection",
        "modifierLabels",
    )
    require(
        "src/preview/CardPreviewSystem.cpp",
        "previewTargetsForEffect",
        "PreviewBlockState",
        "applyRepeatedDamagePreview",
        "targets.requiresSelection",
        "targets.random",
        "preview.outcome.modifiedDamage",
        "preview.outcome.hpDamage",
        "preview.outcome.block",
        "preview.outcome.healing",
        "damage.modifierLabels",
        "blockRange.modifierDescriptions",
    )
    require(
        "tests/GameplayIntegrationTests.cpp",
        "testExactRepeatedDamagePreview",
        "consume target block only once across repeated hits",
    )
    require(
        "src/combat/DamagePreview.hpp",
        "modifierLabels",
    )
    require(
        "src/combat/ModifierSystem.hpp",
        "modifierDescriptions",
    )
    model = require(
        "src/ui/CardViewModel.hpp",
        "previewRequiresTarget",
    )
    if "outcomePreviewLabel" in model:
        raise AssertionError("calculated attack/block/heal values must not be duplicated on the card")
    builder = (ROOT / "src/ui/CardViewModelBuilder.cpp").read_text(encoding="utf-8")
    if "outcomePreviewText" in builder or "ui.card_preview.damage" in builder:
        raise AssertionError("card UI must rely on the card description instead of a duplicate outcome summary")
    visual = (ROOT / "src/ui/CardVisualInstance.cpp").read_text(encoding="utf-8")
    if "outcomePreviewLabel" in visual:
        raise AssertionError("card visual must not draw a duplicate calculated outcome line")
    require(
        "src/ui/CombatView.cpp",
        "cardIterator->stressPreviewLabel",
    )

    keys = {
        "ui.card_preview.damage",
        "ui.card_preview.damage_with_hp",
        "ui.card_preview.block",
        "ui.card_preview.heal",
        "ui.card_preview.choose_target",
        "ui.card_preview.random_target",
    }
    for locale in ("en", "ru"):
        data = json.loads((ROOT / f"data/localization/{locale}/core.json").read_text(encoding="utf-8"))
        missing = sorted(keys.difference(data))
        if missing:
            raise AssertionError(f"{locale} localization is missing: {', '.join(missing)}")

    require(
        "CMakeLists.txt",
        "src/preview/CardOutcomePreview.hpp",
        "test_card_outcome_preview_contract.py",
    )

    print("Card outcome preview contract validated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
