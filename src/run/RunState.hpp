#pragma once

#include "archetypes/PlayableArchetypeId.hpp"
#include "cards/CardId.hpp"
#include "run/DifficultyId.hpp"
#include "run/RunMap.hpp"
#include "run/RunStats.hpp"
#include "run/RunActorState.hpp"
#include "run/RunPendingRoomState.hpp"

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
    bool actCompleted = false;
    int completedAct = 0;
    std::vector<std::string> defeatedBossEnemyIds;

    float enemyHpMultiplier = 1.f;
    float enemyDamageMultiplier = 1.f;
    float goldRewardMultiplier = 1.f;

    std::vector<CardId> deckCardIds;
    std::vector<int> upgradedDeckIndices;

    std::vector<std::string> relicIds;

    // Consumables / potions owned by the current run.
    // Most archetypes start with 3 slots, but relics / events / archetype mechanics may change this.
    std::vector<std::string> consumableIds;
    int maxConsumables = 3;

    std::vector<std::string> actorDefinitionIds;
    std::vector<std::string> rewardCardPoolIds;
    std::vector<RunActorState> actorStates;

    RunMap map;
    RunStats stats;

    RunPendingRoomState pendingRoom;
};
