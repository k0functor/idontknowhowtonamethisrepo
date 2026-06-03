#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstanceId.hpp"
#include "core/Random.hpp"
#include "dice/DiceCorruption.hpp"
#include "entities/EntityId.hpp"

#include <optional>

struct EffectContext {
    EntityId source;

    // UI and callers still pass one primary target, but mixed cards can need
    // one enemy target and one ally target in the same effect list.
    std::optional<EntityId> explicitTarget;
    std::optional<EntityId> explicitEnemyTarget;
    std::optional<EntityId> explicitAllyTarget;

    CardInstanceId cardInstanceId;
    CardId cardDefinitionId;

    DiceCorruption diceCorruption;

    Random* random = nullptr;
};
