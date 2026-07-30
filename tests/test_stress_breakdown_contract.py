#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    rules = read("src/run/StressBreakdownRules.hpp")
    turn = read("src/combat/PlayerTurnSystem.cpp")
    state = read("src/combat/CombatState.hpp")
    card_cost = read("src/combat/CardCost.cpp")
    combat_scene = read("src/scenes/CombatScene.cpp")
    run_controller = read("src/run/RunController.cpp")
    run_map = read("src/scenes/RunMapScene.cpp")
    merchant_rest = read("src/scenes/MerchantRestScene.cpp")

    for token in (
        "BreakdownType::Discard",
        "BreakdownType::EnergyCrash",
        "BreakdownType::IntrusiveThoughts",
        "BreakdownType::CostSpike",
        "BreakdownType::Frenzy",
    ):
        require(token in turn or token in rules, f"missing stress breakdown type: {token}")

    require("pendingForcedCardPlay" in state, "combat state must queue uncontrolled card plays")
    require("resolvePendingForcedCardPlay" in combat_scene, "combat scene must execute queued uncontrolled attacks")
    require("state.cardCostModifier(source)" in card_cost, "card cost preview and payment must include stress cost spikes")
    require("wound_status" in turn and "burn_status" in turn, "intrusive thoughts must add status cards")

    heal_start = run_controller.index("void RunController::completeRestHeal")
    calm_start = run_controller.index("void RunController::completeRestCalm", heal_start)
    heal_body = run_controller[heal_start:calm_start]
    require("healAllActorsByPercent" in heal_body, "rest heal must still restore health")
    require("reduceAllActorsStress" not in heal_body, "rest heal must not reduce stress")
    require("restStressPreviewText" not in run_map, "normal rest heal preview must not promise stress recovery")
    require("stressPreviewText" not in merchant_rest, "merchant rest heal preview must not promise stress recovery")

    for locale in ("en", "ru"):
        logs = json.loads(read(f"data/localization/{locale}/combat_logs.json"))
        core = json.loads(read(f"data/localization/{locale}/core.json"))
        for key in (
            "combat.log.stress_breakdown_energy",
            "combat.log.stress_breakdown_status_cards",
            "combat.log.stress_breakdown_cost",
            "combat.log.stress_breakdown_forced_card",
        ):
            require(key in logs, f"{locale} localization missing {key}")
        for key in (
            "ui.lost_psychopath_stress_risk.breakdown",
            "ui.lost_psychopath_stress_risk.energy_and_breakdown",
        ):
            require(key in core, f"{locale} localization missing {key}")

    print("Stress breakdown contract OK: five outcomes, forced play, cost spike, status cards, heal-only rest")


if __name__ == "__main__":
    main()
