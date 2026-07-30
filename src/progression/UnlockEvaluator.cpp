#include "UnlockEvaluator.hpp"

#include <algorithm>

namespace {
void appendUnique(std::vector<std::string>& destination, const std::vector<std::string>& source) {
    for (const std::string& id : source) {
        if (std::find(destination.begin(), destination.end(), id) == destination.end()) {
            destination.push_back(id);
        }
    }
}

void appendReward(UnlockReward& destination, const UnlockReward& source) {
    appendUnique(destination.archetypeIds, source.archetypeIds);
    appendUnique(destination.cardIds, source.cardIds);
    appendUnique(destination.relicIds, source.relicIds);
}
}

UnlockReward UnlockEvaluator::collectChallengeRewards(
    const ChallengeDatabase& challenges,
    const std::vector<std::string>& completedIds
) {
    UnlockReward reward;
    for (const std::string& id : completedIds) {
        if (challenges.contains(id)) {
            appendReward(reward, challenges.get(id).reward);
        }
    }
    return reward;
}

UnlockReward UnlockEvaluator::collectAchievementRewards(
    const AchievementDatabase& achievements,
    const std::vector<std::string>& completedIds
) {
    UnlockReward reward;
    for (const std::string& id : completedIds) {
        if (achievements.contains(id)) {
            appendReward(reward, achievements.get(id).reward);
        }
    }
    return reward;
}
