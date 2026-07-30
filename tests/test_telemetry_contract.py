#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8-sig")


def main() -> int:
    run_stats = read("src/run/RunStats.hpp")
    serializer = read("src/save/RunStateSerializer.cpp")
    writer = read("src/telemetry/RunTelemetryWriter.cpp")
    flow = read("src/flow/GameFlowController.cpp")
    cmake = read("CMakeLists.txt")

    fields = {
        "damageDealt": "damage_dealt",
        "damageBlocked": "damage_blocked",
        "blockGained": "block_gained",
        "combatTurns": "combat_turns",
        "cardsPlayedInCombat": "cards_played_in_combat",
        "energySpentOnCards": "energy_spent_on_cards",
        "maximumSingleHit": "maximum_single_hit",
        "longestCombatTurns": "longest_combat_turns",
        "mostCardsPlayedInCombat": "most_cards_played_in_combat",
    }
    for field, key in fields.items():
        assert field in run_stats, f"RunStats is missing {field}"
        assert f'"{key}"' in serializer, f"run save serialization is missing {key}"

    assert "RunTelemetryWriter::append" in flow
    assert "saves/telemetry/run_history.jsonl" in flow
    assert "std::ios::app" in writer, "telemetry must append instead of replacing prior runs"
    assert "schema_version" in writer
    assert "http://" not in writer and "https://" not in writer, "local telemetry must not use the network"
    assert "simulate_balance" in cmake and "report_run_telemetry" in cmake

    report_path = ROOT / "reports" / "balance_simulation.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    assert report["schema_version"] == 2
    assert len(report["starter_decks"]) == 7
    assert len(report["encounters"]) == 211
    assert any(item["floor_id"] == "floor5" and item["pool"] == "boss" for item in report["floor_aggregates"])

    print("Telemetry contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
