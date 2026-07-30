#pragma once

#include "achievements/AchievementDefinition.hpp"
#include "profile/ProfileData.hpp"
#include "run/RunState.hpp"

class AchievementRules {
public:
    static bool isUnlocked(const AchievementDefinition& achievement, const ProfileData& profile);
    static bool isCompleted(const AchievementDefinition& achievement, const ProfileData& profile, const RunState* run);
};
