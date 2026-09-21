#!/usr/bin/env python3
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(path: str):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


def main() -> int:
    events = load("data/events/run_events.json")
    by_id = {event["id"]: event for event in events}
    assert len(events) == 97, f"event audit must refine, not inflate, the pool: {len(events)}"

    effect_counts = Counter(
        effect["type"]
        for event in events
        for choice in event["choices"]
        for effect in choice.get("effects", [])
    )
    assert effect_counts["upgrade_random_card"] >= 12, effect_counts

    missing_hp_choices = [
        choice
        for event in events
        for choice in event["choices"]
        if choice.get("requirements", {}).get("min_missing_hp", 0) > 0
    ]
    assert len(missing_hp_choices) >= 10, len(missing_hp_choices)

    developed_deck_choices = [
        choice
        for event in events
        for choice in event["choices"]
        if choice.get("requirements", {}).get("min_upgraded_cards", 0) > 0
    ]
    assert len(developed_deck_choices) >= 5, len(developed_deck_choices)

    compact_deck_choices = [
        choice
        for event in events
        for choice in event["choices"]
        if choice.get("requirements", {}).get("max_deck_size", 0) > 0
    ]
    assert len(compact_deck_choices) >= 3, len(compact_deck_choices)

    # Deck-thinning events must no longer be happy to eat a tiny deck.
    for event_id, choice_index, minimum in (
        ("silt_scriptorium", 1, 8),
        ("cracked_star_map", 0, 10),
        ("void_orchestra_pit", 1, 10),
        ("black_crown_mirror", 1, 10),
        ("chain_archive", 1, 10),
        ("court_of_empty_names", 1, 10),
    ):
        actual = by_id[event_id]["choices"][choice_index].get("requirements", {}).get("min_deck_size", 0)
        assert actual >= minimum, (event_id, actual, minimum)

    # The confession-themed filler events are now one persistent branch.
    assert by_id["furnace_confessional"].get("requirements", {}).get("has_flag") == "chain.confession.started"
    assert by_id["dead_king_confession"].get("requirements", {}).get("has_flag") == "chain.confession.continued"
    rusted_effects = [effect for choice in by_id["rusted_confessional"]["choices"] for effect in choice.get("effects", [])]
    assert any(effect.get("flag") == "chain.confession.honest" for effect in rusted_effects)
    assert any(effect.get("flag") == "chain.confession.lied" for effect in rusted_effects)

    selector = (ROOT / "src/events/RunEventSelector.cpp").read_text(encoding="utf-8")
    flow = (ROOT / "src/flow/GameFlowController.cpp").read_text(encoding="utf-8")
    assert '"event.seen." + eventId' in selector
    assert "unseen.empty() ? available : unseen" in selector
    assert '"event.seen." + eventId' in flow

    parser = (ROOT / "src/data/parsers/RunEventDefinitionParser.cpp").read_text(encoding="utf-8")
    controller = (ROOT / "src/run/RunController.cpp").read_text(encoding="utf-8")
    formatter = (ROOT / "src/events/RunEventChoicePreviewFormatter.cpp").read_text(encoding="utf-8")
    for token in ("min_missing_hp", "min_upgraded_cards", "max_deck_size", "upgrade_random_card"):
        assert token in parser, token
    assert "CardUpgrade::isUpgradable" in controller
    assert "RunEventOutcomeType::CardUpgraded" in controller
    assert 'event.choice.effect.upgrade_random_card' in formatter

    # Repetition should be lower than before the audit. No exact three-choice effect signature may dominate the pool.
    signatures = Counter(
        tuple(tuple(effect["type"] for effect in choice.get("effects", [])) for choice in event["choices"])
        for event in events
    )
    assert signatures.most_common(1)[0][1] <= 3, signatures.most_common(5)

    for lang in ("ru", "en"):
        requirements = load(f"data/localization/{lang}/event_requirements.json")
        run = load(f"data/localization/{lang}/run.json")
        for key in (
            "event.choice.effect.upgrade_random_card",
            "event.choice.requirement.max_deck_size",
            "event.choice.requirement.min_missing_hp",
            "event.choice.requirement.min_upgraded_cards",
            "event.choice.unavailable.deck_size_max",
            "event.choice.unavailable.missing_hp",
            "event.choice.unavailable.upgraded_cards",
        ):
            assert requirements.get(key), (lang, key)
        assert run.get("event.outcome.card_upgraded"), lang

    print(
        "Event quality contract passed: "
        f"upgrade effects={effect_counts['upgrade_random_card']}, "
        f"wounded choices={len(missing_hp_choices)}, developed-deck choices={len(developed_deck_choices)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
