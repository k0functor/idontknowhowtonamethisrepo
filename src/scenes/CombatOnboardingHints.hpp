#pragma once

#include "combat/CombatState.hpp"
#include "localization/LocalizationManager.hpp"

#include <functional>
#include <string>

class CombatOnboardingHints {
public:
    CombatOnboardingHints(
        const LocalizationManager& localization,
        std::function<bool(const std::string&)> isSeen,
        std::function<void(std::string)> markSeen
    );

    bool update(
        float deltaSeconds,
        const CombatState& state,
        bool combatFinished,
        bool selectedCardNeedsTargetChoice
    );

    const std::string& text() const;

private:
    bool seen(const std::string& hintId) const;
    bool show(const std::string& hintId, const TextId& textId);

private:
    const LocalizationManager& localization_;
    std::function<bool(const std::string&)> isSeen_;
    std::function<void(std::string)> markSeen_;
    std::string text_;
    float remainingSeconds_ = 0.f;
};
