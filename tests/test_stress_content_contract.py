#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(relative):
    return json.loads((ROOT / relative).read_text(encoding="utf-8"))


def fail(message):
    raise SystemExit(f"Stress content contract failed: {message}")


relics = {r["id"]: r for r in load("data/relics/test_relics.json")}
expected_relics = {
    "grounding_bead": None,
    "cracked_metronome": "energy",
    "ash_filter": "status_cards",
    "red_thread_spool": "frenzy",
}
for relic_id, breakdown_type in expected_relics.items():
    relic = relics.get(relic_id)
    if relic is None:
        fail(f"missing stress relic {relic_id}")
    triggers = relic.get("triggers", [])
    if not triggers or triggers[0].get("event") != "stress_breakdown_triggered":
        fail(f"{relic_id} does not react to stress breakdown events")
    if breakdown_type is not None and triggers[0].get("breakdown_type") != breakdown_type:
        fail(f"{relic_id} must filter {breakdown_type} breakdowns")

items = {x["id"]: x for x in load("data/active_items/foundation_items.json")}
chime = items.get("grounding_chime")
if chime is None:
    fail("missing grounding_chime active item")
if chime.get("use_contexts") != ["combat"]:
    fail("grounding_chime must be combat-only")
if not any(e.get("type") == "stabilize_stress" and e.get("amount") == 35 for e in chime.get("effects", [])):
    fail("grounding_chime must stabilize 35 stress")

events = {e["id"]: e for e in load("data/events/run_events.json")}
for event_id in ("breathing_wall", "silent_metronome", "ash_inhaler"):
    if event_id not in events:
        fail(f"missing stress event {event_id}")
    requirements = [c.get("requirements", {}) for c in events[event_id].get("choices", [])]
    if not any("min_stress" in r or "max_stress" in r for r in requirements):
        fail(f"{event_id} has no stress threshold")

primed_actions = []
for path in sorted((ROOT / "data/enemies").glob("*.json")):
    for enemy in load(path.relative_to(ROOT)):
        for action in enemy.get("actions", []):
            prime = [e for e in action.get("effects", []) if e.get("type") == "prime_stress_breakdown"]
            if prime:
                primed_actions.append((enemy.get("id"), action.get("id"), prime[0].get("status")))
if len(primed_actions) < 20:
    fail(f"expected at least 20 priming actions, found {len(primed_actions)}")
allowed = {"discard", "energy", "status_cards", "cost", "frenzy"}
if any(kind not in allowed for _, _, kind in primed_actions):
    fail("enemy action uses an invalid primed breakdown type")

source_checks = {
    "src/combat/CombatState.hpp": ["primeStressBreakdown", "armStressBreakdownGuard"],
    "src/combat/PlayerTurnSystem.cpp": ["StressBreakdownTriggered", "consumeStressBreakdownGuard"],
    "src/relics/RelicTriggerDefinition.hpp": ["breakdownType", "minimumBreakdownSeverity"],
    "src/events/RunEventRequirement.hpp": ["minStress", "requiredTraitIds"],
    "src/scenes/CombatScene.cpp": ["StabilizeStress", "stress_stabilized"],
}
for relative, tokens in source_checks.items():
    text = (ROOT / relative).read_text(encoding="utf-8")
    for token in tokens:
        if token not in text:
            fail(f"{relative} is missing {token}")

for locale in ("en", "ru"):
    bundles = {}
    for path in (ROOT / f"data/localization/{locale}").glob("*.json"):
        bundles.update(json.loads(path.read_text(encoding="utf-8")))
    keys = [
        "relic.grounding_bead.name",
        "relic.cracked_metronome.name",
        "relic.ash_filter.name",
        "relic.red_thread_spool.name",
        "active_item.grounding_chime.name",
        "active_item.feedback.stress_stabilized",
        "combat.log.stress_breakdown_prevented",
        "intent.summary.prime_stress_breakdown",
        "event.choice.requirement.min_stress",
        "event.breathing_wall.title",
        "event.silent_metronome.title",
        "event.ash_inhaler.title",
    ]
    missing = [key for key in keys if not bundles.get(key)]
    if missing:
        fail(f"{locale} localization is missing {missing}")

print(f"Stress content contract passed ({len(primed_actions)} priming actions)")
