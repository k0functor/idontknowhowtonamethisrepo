#pragma once

#include "achievements/AchievementDatabase.hpp"
#include "profile/ProfileData.hpp"
#include "run/RunState.hpp"

#include <string>
#include <vector>

class AchievementEvaluator {
public:
    static std::vector<std::string> findNewlyCompleted(
        const AchievementDatabase& achievements,
        const ProfileData& profile,
        const RunState* run
    );
};
