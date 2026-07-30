#include "AchievementEvaluator.hpp"

#include "achievements/AchievementRules.hpp"

#include <algorithm>

std::vector<std::string> AchievementEvaluator::findNewlyCompleted(
    const AchievementDatabase& achievements,
    const ProfileData& profile,
    const RunState* run
) {
    std::vector<std::string> completedIds;
    for (const AchievementDefinition* achievement : achievements.all()) {
        if (achievement == nullptr) {
            continue;
        }
        if (std::find(profile.completedAchievementIds.begin(), profile.completedAchievementIds.end(), achievement->id) !=
            profile.completedAchievementIds.end()) {
            continue;
        }
        if (!AchievementRules::isUnlocked(*achievement, profile)) {
            continue;
        }
        if (AchievementRules::isCompleted(*achievement, profile, run)) {
            completedIds.push_back(achievement->id);
        }
    }

    return completedIds;
}
