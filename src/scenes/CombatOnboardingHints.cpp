#include "CombatOnboardingHints.hpp"

#include <algorithm>

CombatOnboardingHints::CombatOnboardingHints(
    const LocalizationManager& localization,
    std::function<bool(const std::string&)> isSeen,
    std::function<void(std::string)> markSeen
)
    : localization_(localization),
      isSeen_(std::move(isSeen)),
      markSeen_(std::move(markSeen)) {}

bool CombatOnboardingHints::seen(const std::string& hintId) const {
    return isSeen_ && isSeen_(hintId);
}

bool CombatOnboardingHints::show(const std::string& hintId, const TextId& textId) {
    if (hintId.empty() || seen(hintId)) {
        return false;
    }

    text_ = localization_.get(textId);
    remainingSeconds_ = 4.5f;
    if (markSeen_) {
        markSeen_(hintId);
    }
    return true;
}

bool CombatOnboardingHints::update(
    const float deltaSeconds,
    const CombatState& state,
    const bool combatFinished,
    const bool selectedCardNeedsTargetChoice
) {
    bool changed = false;
    if (remainingSeconds_ > 0.f) {
        remainingSeconds_ = std::max(0.f, remainingSeconds_ - std::max(0.f, deltaSeconds));
        if (remainingSeconds_ <= 0.f && !text_.empty()) {
            text_.clear();
            changed = true;
        }
    }

    if (state.phase != CombatPhase::PlayerTurn || combatFinished) {
        return changed;
    }

    if (selectedCardNeedsTargetChoice && !seen("combat_target")) {
        text_.clear();
        remainingSeconds_ = 0.f;
        return show("combat_target", TextId("onboarding.combat.target")) || changed;
    }

    if (remainingSeconds_ > 0.f) {
        return changed;
    }

    if (!seen("combat_card_energy") && !state.hand.empty()) {
        return show("combat_card_energy", TextId("onboarding.combat.card_energy")) || changed;
    }
    if (!seen("combat_intent") && !state.aliveEnemyIds().empty()) {
        return show("combat_intent", TextId("onboarding.combat.intent")) || changed;
    }

    const bool hasStress = std::any_of(
        state.players.begin(),
        state.players.end(),
        [](const CombatEntity& player) { return player.isAlive() && player.stress > 0; }
    );
    if (hasStress && !seen("combat_stress")) {
        return show("combat_stress", TextId("onboarding.combat.stress")) || changed;
    }
    if (state.turn >= 2 && !seen("combat_inspect")) {
        return show("combat_inspect", TextId("onboarding.combat.inspect")) || changed;
    }
    if ((state.resources.energy() <= 0 || state.turn >= 2) && !seen("combat_end_turn")) {
        return show("combat_end_turn", TextId("onboarding.combat.end_turn")) || changed;
    }

    return changed;
}

const std::string& CombatOnboardingHints::text() const {
    return text_;
}
