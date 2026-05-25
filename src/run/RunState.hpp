#pragma once

#include "archetypes/PlayableArchetypeId.hpp"
#include "cards/CardId.hpp"
#include "run/DifficultyId.hpp"
#include "run/RunMap.hpp"
#include "run/RunStats.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct RunState {
    PlayableArchetypeId archetypeId;
    DifficultyId difficultyId;

    std::string archetypeMechanicId = "default";

    std::uint32_t seed = 0;
    int gold = 0;
    int act = 1;

    float enemyHpMultiplier = 1.f;
    float enemyDamageMultiplier = 1.f;
    float goldRewardMultiplier = 1.f;

    std::vector<CardId> deckCardIds;
    std::vector<std::string> relicIds;
    std::vector<std::string> actorDefinitionIds;

    RunMap map;
    RunStats stats;
};
