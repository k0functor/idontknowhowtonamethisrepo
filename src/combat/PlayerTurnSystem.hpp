#pragma once

#include "cards/DrawSystem.hpp"
#include "combat/CombatState.hpp"
#include "core/Random.hpp"

#include <cstddef>

class PlayerTurnSystem {
public:
    explicit PlayerTurnSystem(const DrawSystem& drawSystem);

    void startTurn(
        CombatState& state,
        std::size_t handSize,
        Random& random
    ) const;

    void endTurn(CombatState& state) const;

private:
    void discardHand(CombatState& state) const;

private:
    const DrawSystem& drawSystem_;
};
