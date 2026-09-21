#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(path: str):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


def status_amount(relic: dict, status_id: str) -> int:
    total = 0
    for trigger in relic.get("triggers", []):
        if trigger.get("event") != "combat_started":
            continue
        for effect in trigger.get("effects", []):
            if effect.get("type") == "apply_status" and effect.get("status") == status_id:
                total += int(effect.get("value", {}).get("amount", 0))
    return total


def main() -> int:
    relics = load("data/relics/test_relics.json")
    statuses = {entry["id"] for entry in load("data/statuses/combat_statuses.json")}
    by_id = {entry["id"]: entry for entry in relics}

    forbidden_modifiers = {
        "outgoing_damage_add",
        "outgoing_damage_multiply",
        "block_add",
    }
    for relic in relics:
        for modifier in relic.get("modifiers", []):
            modifier_type = modifier.get("type")
            assert modifier_type not in forbidden_modifiers, (
                f"{relic['id']} directly modifies combat numbers through {modifier_type}; "
                "combat scaling must come from statuses"
            )
            assert modifier_type == "gold_reward_multiply", (
                f"unsupported relic modifier {modifier_type}; only economy modifiers may remain direct"
            )

    assert "strength" in statuses
    assert "dexterity" in statuses
    assert "ferocity" in statuses

    assert status_amount(by_id["iron_nerves"], "dexterity") == 1
    assert status_amount(by_id["bastion_seal"], "strength") == 1
    assert status_amount(by_id["bastion_seal"], "dexterity") == 1
    assert status_amount(by_id["splint_mail"], "dexterity") == 2
    assert status_amount(by_id["ember_lens"], "ferocity") == 2
    assert status_amount(by_id["bent_pauldron"], "dexterity") == 0
    assert any(
        effect.get("status") == "dexterity"
        for trigger in by_id["bent_pauldron"].get("triggers", [])
        for effect in trigger.get("effects", [])
    )
    assert status_amount(by_id["brass_locket"], "dexterity") == 0
    assert any(
        trigger.get("event") == "block_gained"
        and trigger.get("min_amount") == 10
        and any(effect.get("status") == "dexterity" for effect in trigger.get("effects", []))
        for trigger in by_id["brass_locket"].get("triggers", [])
    )
    assert any(
        effect.get("status") == "dexterity"
        for trigger in by_id["grounding_bead"].get("triggers", [])
        for effect in trigger.get("effects", [])
    )

    status_relic_count = sum(
        1
        for relic in relics
        if any(
            effect.get("type") == "apply_status"
            for trigger in relic.get("triggers", [])
            for effect in trigger.get("effects", [])
        )
    )
    assert status_relic_count > len(relics) // 2, (
        f"most relics must grant statuses, got {status_relic_count}/{len(relics)}"
    )

    for lang in ("ru", "en"):
        status_text = load(f"data/localization/{lang}/statuses.json")
        relic_text = load(f"data/localization/{lang}/relics.json")
        assert status_text.get("status.ferocity.name")
        assert status_text.get("status.ferocity.description")
        for relic_id in ("iron_nerves", "bastion_seal", "splint_mail", "ember_lens"):
            description = relic_text[f"relic.{relic_id}.description"]
            assert description

    ru_relics = load("data/localization/ru/relics.json")
    assert "ловкост" in ru_relics["relic.iron_nerves.description"].lower()
    assert "больше" not in ru_relics["relic.iron_nerves.description"].lower()

    print(
        "Status-driven relic contract passed: "
        f"{len(relics)} relics, {status_relic_count} grant statuses, direct combat modifiers removed"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
