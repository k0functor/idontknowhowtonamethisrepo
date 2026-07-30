#pragma once

#include "cards/CardType.hpp"
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
    std::optional<CardType> cardType;
    std::optional<std::string> breakdownType;
    int minimumBreakdownSeverity = 0;
    std::string sourceSide = "any";

    // 0 means no minimum amount condition. Useful for “HP damage was actually dealt”.
    int minimumAmount = 0;

    std::vector<EffectDefinition> effects;
};
