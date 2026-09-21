#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"stress role contract failed: {message}")


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


rules = read("src/run/StressRules.hpp")
for token in (
    "psychopathMechanicEnabled",
    "if (!psychopathMechanicEnabled)",
    "removeTrait(actor, BreakdownTraitId)",
    "removeTrait(actor, ResolveTraitId)",
    "psychopathMechanicEnabled &&",
):
    if token not in rules:
        fail(f"generic stress rules are missing the psychopath gate: {token}")

for path, actor_expression in {
    "src/combat/EffectSystem.cpp": "StressPsychopathRules::appliesTo(entity.definitionId)",
    "src/combat/CardPlaySystem.cpp": "StressPsychopathRules::appliesTo(sourceEntity.definitionId)",
    "src/run/RunController.cpp": "StressPsychopathRules::appliesTo(actor.definitionId)",
    "src/scenes/CombatScene.cpp": "StressPsychopathRules::appliesTo(entity.definitionId)",
}.items():
    if actor_expression not in read(path):
        fail(f"{path} does not select the unique stress mechanic explicitly")

player_turn = read("src/combat/PlayerTurnSystem.cpp")
legacy_branch = "if (hasBreakdown) {\n                discardRandomCards(state, 1"
if legacy_branch in player_turn:
    fail("ordinary characters still receive the old generic breakdown discard")

allowed_bonus_files = {
    "src/run/StressPsychopathRules.hpp",
    "src/combat/ModifierSystem.cpp",
    "src/combat/PlayerTurnSystem.cpp",
    "src/ui/CombatViewModelBuilder.cpp",
}
bonus_tokens = (
    "damageBonusForStress(",
    "startTurnEnergyBonus(",
    "startTurnDiscardCount(",
    "breakdownSeverity(",
)
for path in (ROOT / "src").rglob("*.cpp"):
    relative = path.relative_to(ROOT).as_posix()
    text = path.read_text(encoding="utf-8")
    if any(token in text for token in bonus_tokens) and relative not in allowed_bonus_files:
        fail(f"direct stress-band bonus leaked into {relative}")

archetypes = json.loads((ROOT / "data/archetypes/playable_archetypes.json").read_text(encoding="utf-8"))
mechanic_owners = [a["id"] for a in archetypes if a.get("mechanic_id") == "stress_psychopath"]
if mechanic_owners != ["lost_psychopath"]:
    fail(f"stress_psychopath mechanic must belong only to lost_psychopath, got {mechanic_owners}")

relics = json.loads((ROOT / "data/relics/test_relics.json").read_text(encoding="utf-8"))
expected_relics = {"grounding_bead", "cracked_metronome", "ash_filter", "red_thread_spool"}
for relic in relics:
    if relic.get("id") in expected_relics and relic.get("mechanic_id") != "stress_psychopath":
        fail(f"{relic['id']} must be restricted to the psychopath mechanic")

reward_rules = read("src/rewards/RewardPoolRules.cpp")
for token in ("matchesRunMechanic", 'relic.mechanicId == "default"', "relic.mechanicId == mechanicId"):
    if token not in reward_rules:
        fail(f"mechanic-specific relic filtering is missing {token}")

for path in (
    "src/rewards/RewardGenerator.cpp",
    "src/run/RunController.cpp",
    "src/flow/GameFlowController.cpp",
    "src/active_items/ActiveItemRerollSystem.cpp",
):
    if "archetypeMechanicId" not in read(path):
        fail(f"{path} does not filter relics by the run mechanic")

for path, token in {
    "src/events/RunEventRequirement.hpp": "requiredMechanicId",
    "src/data/parsers/RunEventDefinitionParser.cpp": 'optionalString("mechanic_id", "")',
    "src/events/RunEventRequirement.cpp": "WrongRunMechanic",
}.items():
    if token not in read(path):
        fail(f"mechanic-specific event filtering is missing {token} in {path}")

events = json.loads((ROOT / "data/events/run_events.json").read_text(encoding="utf-8"))
restricted_events = {"breathing_wall", "silent_metronome", "ash_inhaler"}
for event in events:
    if event.get("id") in restricted_events:
        mechanic = event.get("requirements", {}).get("mechanic_id")
        if mechanic != "stress_psychopath":
            fail(f"{event['id']} must be restricted to the psychopath mechanic")

print("Stress role contract passed: direct stress bonuses belong only to lost_psychopath")
