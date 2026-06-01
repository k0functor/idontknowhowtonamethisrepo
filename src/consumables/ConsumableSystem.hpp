#pragma once

#include "combat/EffectSystem.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "core/Random.hpp"
#include "entities/EntityId.hpp"

#include <optional>
#include <string>

class ConsumableSystem {
public:
    explicit ConsumableSystem(const ConsumableDatabase& database);

    bool useConsumable(
        CombatState& state,
        const std::string& consumableId,
        EntityId source,
        std::optional<EntityId> explicitTarget,
        const EffectSystem& effectSystem,
        Random& random
    ) const;

private:
    const ConsumableDatabase& database_;
};
