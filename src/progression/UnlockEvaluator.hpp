#pragma once

#include "achievements/AchievementDatabase.hpp"
#include "challenges/ChallengeDatabase.hpp"
#include "profile/UnlockReward.hpp"

#include <string>
#include <vector>

class UnlockEvaluator {
public:
    static UnlockReward collectChallengeRewards(
        const ChallengeDatabase& challenges,
        const std::vector<std::string>& completedIds
    );

    static UnlockReward collectAchievementRewards(
        const AchievementDatabase& achievements,
        const std::vector<std::string>& completedIds
    );
};
