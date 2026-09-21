#pragma once

#include "combat/CombatState.hpp"
#include "combat/EffectContext.hpp"
#include "effects/EffectTarget.hpp"

#include <vector>

class Targeting {
public:
    std::vector<EntityId> resolveTargets(
        const CombatState& state,
        EffectTarget target,
        const EffectContext& context
    ) const;

    static bool isValidResolvedTarget(
        const CombatState& state,
        EffectTarget target,
        EntityId candidate,
        EntityId source
    );
};
