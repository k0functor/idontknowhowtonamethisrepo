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
    std::optional<CardType> previousCardType;

    // Exact 1-based position of the played card in the current player turn.
    // 0 disables this condition.
    int cardNumberThisTurn = 0;

    // Optional owner-side state filters for build-around relics.
    std::optional<std::string> ownerStatusId;
    int minimumDrones = 0;
    std::optional<std::string> breakdownType;
    int minimumBreakdownSeverity = 0;
    std::string sourceSide = "any";

    // 0 means no minimum amount condition. Useful for “HP damage was actually dealt”.
    int minimumAmount = 0;

    std::vector<EffectDefinition> effects;
};
