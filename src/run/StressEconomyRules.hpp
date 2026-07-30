#pragma once

#include <algorithm>

namespace StressEconomyRules {
inline constexpr int DamagePerStress = 3;
inline constexpr int MaximumStressFromSingleHit = 12;
inline constexpr int RestCalmAmount = 60;

inline int stressFromHpDamage(const int hpDamage) {
    if (hpDamage <= 0) {
        return 0;
    }

    return std::min(
        MaximumStressFromSingleHit,
        std::max(1, (hpDamage + DamagePerStress - 1) / DamagePerStress)
    );
}
} // namespace StressEconomyRules
