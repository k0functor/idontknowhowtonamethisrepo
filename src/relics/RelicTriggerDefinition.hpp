#pragma once

#include "effects/EffectDefinition.hpp"
#include "game/GameEvent.hpp"

#include <vector>

struct RelicTriggerDefinition {
    GameEventType eventType = GameEventType::CombatStarted;

    // 0 means no turn cadence condition.
    int everyNTurns = 0;

    bool oncePerCombat = false;

    std::vector<EffectDefinition> effects;
};
