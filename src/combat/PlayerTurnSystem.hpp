#pragma once

#include "cards/DrawSystem.hpp"
#include "combat/CombatState.hpp"
#include "combat/EffectSystem.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "game/GameEventBus.hpp"
#include "run/StressBreakdownRules.hpp"

#include <cstddef>

class PlayerTurnSystem {
public:
    PlayerTurnSystem(
        const DrawSystem& drawSystem,
        const CardDatabase& cardDatabase,
        const GameEventBus* eventBus = nullptr,
        const EffectSystem* effectSystem = nullptr
    );

    void startTurn(
        CombatState& state,
        std::size_t handSize,
        Random& random
    ) const;

    void endTurn(CombatState& state, Random& random) const;

    void applyStressBreakdown(
        CombatState& state,
        EntityId playerId,
        StressBreakdownRules::BreakdownType type,
        Random& random
    ) const;

private:
    void resolveEndOfTurnStatusCards(CombatState& state, Random& random) const;
    void discardHand(CombatState& state) const;
    void applyStartOfTurnTraitEffects(CombatState& state, Random& random) const;

private:
    const DrawSystem& drawSystem_;
    const CardDatabase& cardDatabase_;
    const GameEventBus* eventBus_ = nullptr;
    const EffectSystem* effectSystem_ = nullptr;
};
