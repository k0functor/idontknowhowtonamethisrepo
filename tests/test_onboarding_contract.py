#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

profile = (ROOT / "src/profile/ProfileData.hpp").read_text(encoding="utf-8")
profile_manager = (ROOT / "src/profile/ProfileManager.cpp").read_text(encoding="utf-8")
profile_save = (ROOT / "src/profile/ProfileSaveSystem.cpp").read_text(encoding="utf-8")
combat = (ROOT / "src/scenes/CombatScene.cpp").read_text(encoding="utf-8")
onboarding = (ROOT / "src/scenes/CombatOnboardingHints.cpp").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

assert "seenOnboardingHintIds" in profile
assert "markSelectedOnboardingHintSeen" in profile_manager
assert '"onboarding_hints_seen"' in profile_save
assert "profileSaveVersion = 9" in profile_save

for hint in (
    "combat_card_energy",
    "combat_target",
    "combat_intent",
    "combat_stress",
    "combat_inspect",
    "combat_end_turn",
):
    assert hint in onboarding

assert "selectedCardNeedsTargetChoice" in onboarding
assert "state.turn >= 2" in onboarding
assert "player.stress > 0" in onboarding
assert "onboardingHints_.text()" in combat
assert "ui.combat_keyboard_hint" not in combat, "persistent keyboard help should not compete with one-shot onboarding"
assert "src/scenes/CombatOnboardingHints.cpp" in cmake

for locale in ("ru", "en"):
    strings = json.loads((ROOT / f"data/localization/{locale}/core.json").read_text(encoding="utf-8"))
    for text_id in (
        "onboarding.combat.card_energy",
        "onboarding.combat.target",
        "onboarding.combat.intent",
        "onboarding.combat.stress",
        "onboarding.combat.inspect",
        "onboarding.combat.end_turn",
    ):
        assert strings[text_id]

print("Onboarding contract passed: one-shot contextual combat hints are profile-persistent")
