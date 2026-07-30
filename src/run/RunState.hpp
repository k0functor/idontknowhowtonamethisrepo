#pragma once

#include "active_items/ActiveItemState.hpp"
#include "archetypes/PlayableArchetypeId.hpp"
#include "cards/CardId.hpp"
#include "run/DifficultyId.hpp"
#include "run/RunMap.hpp"
#include "run/RunStats.hpp"
#include "run/RunActorState.hpp"
#include "run/RunPendingRoomState.hpp"
#include "run/RunPhase.hpp"
#include "run/RunCompletionType.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct RunState {
    PlayableArchetypeId archetypeId;
    DifficultyId difficultyId;

    std::string archetypeMechanicId = "default";

    // Empty for a normal run. Challenge runs will set this to the challenge id,
    // which keeps challenges separate from ordinary achievement-like goals.
    std::string challengeId;

    std::uint32_t seed = 0;
    std::string randomState;
    int gold = 0;
    int act = 1;
    std::string currentFloorId = "floor1";
    int currentFloorIndex = 1;
    std::string nextFloorId = "floor2";
    bool actCompleted = false;
    int completedAct = 0;
    RunCompletionType completionType = RunCompletionType::InProgress;
    RunPhase phase = RunPhase::Map;
    std::vector<std::string> defeatedBossEnemyIds;
    std::vector<std::string> eventFlags;

    float enemyHpMultiplier = 1.f;
    float enemyDamageMultiplier = 1.f;
    float goldRewardMultiplier = 1.f;

    std::vector<CardId> deckCardIds;
    std::vector<int> upgradedDeckIndices;

    std::vector<std::string> relicIds;

    // The run owns exactly one active-item slot. An empty id means the slot is empty.
    ActiveItemState activeItem;

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
