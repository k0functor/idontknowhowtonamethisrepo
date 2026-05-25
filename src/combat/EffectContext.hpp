#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstanceId.hpp"
#include "core/Random.hpp"
#include "dice/DiceCorruption.hpp"
#include "entities/EntityId.hpp"

#include <optional>

struct EffectContext {
    EntityId source;
    std::optional<EntityId> explicitTarget;

    CardInstanceId cardInstanceId;
    CardId cardDefinitionId;

    DiceCorruption diceCorruption;

    Random* random = nullptr;
};
