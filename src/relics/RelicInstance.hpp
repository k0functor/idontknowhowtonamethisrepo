#pragma once

#include "cards/CardType.hpp"
#include "relics/RelicId.hpp"

#include <optional>
#include <string>

struct RelicInstance {
    RelicId id;
    std::string ownerActorDefinitionId;

    int triggersThisCombat = 0;
    int totalTriggers = 0;

    // Per-owner card sequence state used by conditional relic triggers.
    int trackedCardTurn = 0;
    int cardsPlayedThisTurn = 0;
    std::optional<CardType> previousCardType;
};
