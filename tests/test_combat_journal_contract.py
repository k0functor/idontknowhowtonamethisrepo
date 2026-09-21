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
        "src/combat/CombatLog.hpp",
        "sequence",
        "maxEntries_ = 256u",
        "nextSequence_",
    )
    require(
        "src/combat/CombatLog.cpp",
        "entry.sequence = nextSequence_++",
        "entries_.erase",
        "nextSequence_ = 1u",
    )
    for path in ("src/combat/DamageSystem.cpp", "src/combat/BlockSystem.cpp"):
        require(
            path,
            "prefix + \"_text_id\"",
            "modifierTrace",
            '"modifiers"',
            '"card"',
        )
    require(
        "src/combat/EffectSystem.cpp",
        '"before"',
        '"after"',
        '"target_text_id"',
    )
    require(
        "src/statuses/StatusSystem.cpp",
        '"source_text_id"',
        '"target_text_id"',
    )
    require(
        "src/ui/CombatViewModel.hpp",
        "enum class CombatJournalTone",
        "CombatJournalEntryViewModel",
        "recentJournalEntries",
    )
    require(
        "src/ui/CombatViewModelBuilder.cpp",
        "isCompactJournalEntry",
        "journalToneFor",
        "journalDetail",
        "damage_breakdown_modified",
        "stress_transition_card",
        "recentJournalEntries(state, 7)",
    )
    require(
        "src/ui/CombatView.cpp",
        "journalUiTone",
        "model_.recentJournalEntries",
        "entry.detail",
    )

    keys = {
        "combat.log.unknown_entity",
        "combat.log.damage_compact",
        "combat.log.damage_breakdown",
        "combat.log.damage_breakdown_modified",
        "combat.log.block_compact",
        "combat.log.block_breakdown",
        "combat.log.block_breakdown_modified",
        "combat.log.heal_compact",
        "combat.log.gain_stress_compact",
        "combat.log.lose_stress_compact",
        "combat.log.stress_transition",
        "combat.log.stress_transition_card",
        "combat.log.caused_by_card",
        "combat.log.status_applied_from",
    }
    for locale in ("en", "ru"):
        data = json.loads((ROOT / f"data/localization/{locale}/combat_logs.json").read_text(encoding="utf-8"))
        missing = sorted(keys.difference(data))
        if missing:
            raise AssertionError(f"{locale} combat journal localization is missing: {', '.join(missing)}")

    require("tests/CombatCoreTests.cpp", "testCombatLogSequenceAndCapacity", "damageLog.variables", "blockLog.variables")
    require("CMakeLists.txt", "test_combat_journal_contract.py")

    print("Compact combat journal contract validated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
