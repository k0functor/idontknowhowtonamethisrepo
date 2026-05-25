#pragma once

#include "cards/DrawSystem.hpp"
#include "combat/BlockSystem.hpp"
#include "combat/DamageSystem.hpp"
#include "combat/EffectContext.hpp"
#include "combat/EffectResolver.hpp"
#include "combat/EnergySystem.hpp"
#include "combat/Targeting.hpp"
#include "effects/EffectDefinition.hpp"
#include "statuses/StatusSystem.hpp"
#include "game/GameEventBus.hpp"

#include <vector>

class EffectSystem {
public:
    EffectSystem(
        const EffectResolver& effectResolver,
        const Targeting& targeting,
        const DamageSystem& damageSystem,
        const BlockSystem& blockSystem,
        const EnergySystem& energySystem,
        const DrawSystem& drawSystem,
        const StatusSystem& statusSystem,
        const GameEventBus* eventBus = nullptr
    );

    void applyEffects(
        CombatState& state,
        const std::vector<EffectDefinition>& effects,
        const EffectContext& context
    ) const;

    void applyEffect(
        CombatState& state,
        const EffectDefinition& effect,
        const EffectContext& context
    ) const;

private:
    const EffectResolver& effectResolver_;
    const Targeting& targeting_;
    const DamageSystem& damageSystem_;
    const BlockSystem& blockSystem_;
    const EnergySystem& energySystem_;
    const DrawSystem& drawSystem_;
    const StatusSystem& statusSystem_;
    const GameEventBus* eventBus_ = nullptr;
};
