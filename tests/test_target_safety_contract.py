#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"target safety contract failed: {message}")


targeting_hpp = (ROOT / "src" / "combat" / "Targeting.hpp").read_text(encoding="utf-8")
targeting_cpp = (ROOT / "src" / "combat" / "Targeting.cpp").read_text(encoding="utf-8")
effects_cpp = (ROOT / "src" / "combat" / "EffectSystem.cpp").read_text(encoding="utf-8")
core_tests = (ROOT / "tests" / "CombatCoreTests.cpp").read_text(encoding="utf-8")

for token in (
    "singleTargetOrThrow",
    "SingleEnemy effect requires an explicit target while several enemies are alive",
    "Ally effect requires an explicit target while several allies are available",
):
    if token not in targeting_cpp:
        fail(f"missing explicit single-target rule: {token}")

if "isValidResolvedTarget" not in targeting_hpp or "isValidResolvedTarget" not in targeting_cpp:
    fail("target validity must be shared between target resolution and effect execution")
if "targetStillValid" not in effects_cpp or "hasValidTarget" not in effects_cpp:
    fail("effect chains must revalidate targets after reactive events")
if "!hasValidTarget(state, effect, context, targets)" not in effects_cpp:
    fail("resource-conversion effects must verify a live target before paying their cost")
if "return aliveAlliesExceptSource(state, context.source);" in targeting_cpp:
    fail("a single-ally effect must never fall back to every living ally")

for token in (
    "testExplicitAndAutomaticSingleTargetSafety",
    "testEffectChainSkipsTargetsKilledByReactions",
    "stress conversion effects must not spend their resource after the selected target has died",
):
    if token not in core_tests:
        fail(f"missing behavioral coverage: {token}")

print("Target safety contract OK")
