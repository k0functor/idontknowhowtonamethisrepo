#pragma once

#include <algorithm>
#include <string>

namespace StressPsychopathRules {
inline constexpr const char* ActorDefinitionId = "lost_psychopath";
inline constexpr int FirstBonusThreshold = 50;
inline constexpr int SecondBonusThreshold = 100;
inline constexpr int ThirdBonusThreshold = 150;
inline constexpr int StressPerDamageBonus = 50;
inline constexpr int MaximumDamageBonus = 3;

inline bool appliesTo(const std::string& actorDefinitionId) {
    return actorDefinitionId == ActorDefinitionId;
}

inline int damageBonusForStress(const int stress) {
    if (stress < FirstBonusThreshold) {
        return 0;
    }

    return std::clamp(stress / StressPerDamageBonus, 0, MaximumDamageBonus);
}

inline int nextDamageBonusThreshold(const int stress) {
    if (stress < FirstBonusThreshold) {
        return FirstBonusThreshold;
    }

    if (stress < SecondBonusThreshold) {
        return SecondBonusThreshold;
    }

    if (stress < ThirdBonusThreshold) {
        return ThirdBonusThreshold;
    }

    return 0;
}
} // namespace StressPsychopathRules
