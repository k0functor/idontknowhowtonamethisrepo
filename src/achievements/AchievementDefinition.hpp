#pragma once

#include "localization/TextId.hpp"
#include "profile/UnlockReward.hpp"

#include <string>
#include <vector>

struct AchievementCompletionCondition {
    std::string type;
    std::string floorId;
    std::string archetypeId;
    int minVictories = 0;
    int minDefeats = 0;
    int minCompletedChallenges = 0;
    int minCompletedAchievements = 0;
    int minElitesKilled = 0;
    int minBossesKilled = 0;
    int minEventsCompleted = 0;
    int minRelics = 0;
    int minGold = 0;
    int maxDamageTaken = -1;
};

struct AchievementDefinition {
    std::string id;
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
    std::vector<std::string> requiredCompletedAchievementIds;
    AchievementCompletionCondition completion;
};
