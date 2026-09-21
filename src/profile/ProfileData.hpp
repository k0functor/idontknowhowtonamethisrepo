#pragma once

#include "profile/ProfileCountEntry.hpp"
#include "profile/ProfileProgressEntry.hpp"
#include "profile/ProfileRunStatistics.hpp"

#include <string>
#include <vector>

struct ProfileData {
    int slotIndex = 0;
    bool isEmpty = true;

    std::vector<std::string> unlockedCardIds;
    std::vector<std::string> unlockedRelicIds;
    std::vector<std::string> unlockedArchetypeIds;
    std::vector<std::string> discoveredEnemyIds;
    std::vector<std::string> discoveredStatusIds;
    std::vector<std::string> discoveredConsumableIds;
    std::vector<std::string> completedChallengeIds;
    std::vector<std::string> completedAchievementIds;
    std::vector<std::string> seenOnboardingHintIds;
    std::vector<ProfileProgressEntry> progressLog;

    // Long-lived profile statistics. These are not unlock flags: they track
    // how often the player actually used or picked up content across runs.
    std::vector<ProfileCountEntry> cardPlayCounts;
    std::vector<ProfileCountEntry> relicPickCounts;

    int victories = 0;
    int defeats = 0;
    ProfileRunStatistics lifetimeRunStats;
};
