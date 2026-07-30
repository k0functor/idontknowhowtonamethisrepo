#pragma once

#include "run/StressRules.hpp"

#include <string>

namespace StressPsychopathRules {
inline constexpr const char* ActorDefinitionId = "lost_psychopath";

struct StressBandEffects {
    StressRules::StressBand band = StressRules::StressBand::Calm;
    int attackDamageBonus = 0;
    int startTurnEnergyBonus = 0;
    int startTurnDiscardCount = 0;
    int breakdownSeverity = 0;
};

inline bool appliesTo(const std::string& actorDefinitionId) {
    return actorDefinitionId == ActorDefinitionId;
}

inline int damageBonusForStress(const int stress) {
    switch (StressRules::bandForStress(stress)) {
        case StressRules::StressBand::Calm: return 0;
        case StressRules::StressBand::Tense: return 1;
        case StressRules::StressBand::Pressured: return 2;
        case StressRules::StressBand::Panicked: return 3;
        case StressRules::StressBand::Breaking:
        case StressRules::StressBand::Collapsed:
            return 4;
    }
    return 0;
}

inline int nextDamageBonusThreshold(const int stress) {
    return StressRules::nextBandThreshold(stress);
}

inline int startTurnEnergyBonus(const int stress) {
    return StressRules::bandForStress(stress) == StressRules::StressBand::Breaking ? 1 : 0;
}

inline int startTurnDiscardCount(
    const int stress,
    const bool hasBreakdown,
    const bool hasResolve
) {
    if (hasBreakdown || hasResolve) {
        return 0;
    }

    return StressRules::bandForStress(stress) == StressRules::StressBand::Breaking ? 1 : 0;
}

inline int breakdownSeverity(
    const int stress,
    const bool hasBreakdown,
    const bool hasResolve
) {
    if (!hasBreakdown || hasResolve) {
        return 0;
    }

    switch (StressRules::bandForStress(stress)) {
        case StressRules::StressBand::Panicked: return 1;
        case StressRules::StressBand::Breaking: return 2;
        case StressRules::StressBand::Calm:
        case StressRules::StressBand::Tense:
        case StressRules::StressBand::Pressured:
        case StressRules::StressBand::Collapsed:
            return 0;
    }
    return 0;
}

inline StressBandEffects effectsFor(
    const int stress,
    const bool hasBreakdown,
    const bool hasResolve
) {
    StressBandEffects result;
    result.band = StressRules::bandForStress(stress);
    result.attackDamageBonus = damageBonusForStress(stress);
    result.startTurnEnergyBonus = startTurnEnergyBonus(stress);
    result.startTurnDiscardCount = startTurnDiscardCount(stress, hasBreakdown, hasResolve);
    result.breakdownSeverity = breakdownSeverity(stress, hasBreakdown, hasResolve);
    return result;
}
} // namespace StressPsychopathRules
