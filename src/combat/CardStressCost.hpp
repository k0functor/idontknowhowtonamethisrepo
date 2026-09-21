#pragma once

#include "cards/CardDefinition.hpp"
#include "effects/EffectType.hpp"

#include <algorithm>

namespace CardStressCost {
inline int conversionCost(const CardDefinition& definition) {
    int result = 0;
    for (const EffectDefinition& effect : definition.effects) {
        if (isStressConversionEffect(effect.type) && effect.value.isFixed()) {
            result += std::max(0, effect.value.fixedAmount()) * std::max(1, effect.repeatCount);
        }
    }
    return result;
}

inline int directCost(const CardDefinition& definition) {
    return std::max(0, definition.stressCost);
}

inline int totalCost(const CardDefinition& definition) {
    return directCost(definition) + conversionCost(definition);
}
} // namespace CardStressCost
