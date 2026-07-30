#pragma once

#include "localization/TextId.hpp"
#include "profile/UnlockReward.hpp"

#include <string>
#include <vector>

struct ChallengeCompletionCondition {
    std::string type;
    std::string floorId;
    std::string archetypeId;
    int minElitesKilled = 0;
    int minBossesKilled = 0;
    int maxShopsVisited = -1;
    int maxDamageTaken = -1;
};

struct ChallengeDefinition {
    std::string id;

    // A challenge is a separate fixed run, not an achievement clone.
    // These fields describe how the run is created when the player starts it
    // from the challenge screen. Optional fixed lists override the archetype
    // defaults after the base run is created.
    std::string startingArchetypeId;
    std::string startingDifficultyId = "normal";
    std::string startingFloorId;
    int startingGoldOverride = -1;
    std::vector<std::string> fixedStartingDeckCardIds;
    std::vector<std::string> fixedStartingRelicIds;
    std::vector<std::string> fixedStartingConsumableIds;

    TextId nameTextId;
    TextId descriptionTextId;
    TextId goalTextId;
    TextId rewardTextId;
    TextId unlockHintTextId;
    UnlockReward reward;
    bool isAvailable = true;
    int selectionOrder = 0;
    std::vector<std::string> requiredUnlockedArchetypeIds;
    std::vector<std::string> requiredCompletedChallengeIds;
    ChallengeCompletionCondition completion;
};
