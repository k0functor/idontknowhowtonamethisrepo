#pragma once

#include "cards/CardInstanceId.hpp"
#include "combat/CardPlayValidator.hpp"
#include "combat/EffectSystem.hpp"
#include "combat/EnergySystem.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "entities/EntityId.hpp"
#include "game/GameEventBus.hpp"

#include <optional>
#include <string>

struct PlayCardRequest {
    CardInstanceId cardInstanceId;
    EntityId source;
    std::optional<EntityId> target;
};

struct PlayCardResult {
    bool played = false;
    std::string reason;
};

class CardPlaySystem {
public:
    CardPlaySystem(
        const CardDatabase& cardDatabase,
        const CardPlayValidator& validator,
        const EnergySystem& energySystem,
        const EffectSystem& effectSystem,
        const GameEventBus* eventBus = nullptr
    );

    PlayCardResult playCard(
        CombatState& state,
        const PlayCardRequest& request,
        Random& random
    ) const;

private:
    const CardDatabase& cardDatabase_;
    const CardPlayValidator& validator_;
    const EnergySystem& energySystem_;
    const EffectSystem& effectSystem_;
    const GameEventBus* eventBus_ = nullptr;
};
