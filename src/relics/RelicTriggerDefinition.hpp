#pragma once

#include "effects/EffectDefinition.hpp"
#include "game/GameEvent.hpp"

#include <optional>
#include <string>
#include <vector>

struct RelicTriggerDefinition {
    GameEventType eventType = GameEventType::CombatStarted;

    // 0 means no turn cadence condition.
    int everyNTurns = 0;

    bool oncePerCombat = false;

    // Optional event filters. Empty filters match every event of eventType.
    std::optional<std::string> statusId;
    std::string sourceSide = "any";

    std::vector<EffectDefinition> effects;
};
