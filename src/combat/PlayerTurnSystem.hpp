#pragma once

#include "cards/DrawSystem.hpp"
#include "combat/CombatState.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"

#include <cstddef>

class PlayerTurnSystem {
public:
    PlayerTurnSystem(
        const DrawSystem& drawSystem,
        const CardDatabase& cardDatabase
    );

    void startTurn(
        CombatState& state,
        std::size_t handSize,
        Random& random
    ) const;

    void endTurn(CombatState& state) const;

private:
    void discardHand(CombatState& state) const;
    void applyStartOfTurnTraitEffects(CombatState& state, Random& random) const;

private:
    const DrawSystem& drawSystem_;
    const CardDatabase& cardDatabase_;
};
