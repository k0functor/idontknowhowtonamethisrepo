#include "ChallengeEvaluator.hpp"

#include "challenges/ChallengeRules.hpp"

#include <algorithm>

std::vector<std::string> ChallengeEvaluator::findNewlyCompleted(
    const ChallengeDatabase& challenges,
    const ProfileData& profile,
    const RunState& run
) {
    std::vector<std::string> completedIds;
    if (run.challengeId.empty()) {
        return completedIds;
    }

    for (const ChallengeDefinition* challenge : challenges.all()) {
        if (challenge == nullptr) {
            continue;
        }
        if (std::find(profile.completedChallengeIds.begin(), profile.completedChallengeIds.end(), challenge->id) !=
            profile.completedChallengeIds.end()) {
            continue;
        }
        if (!ChallengeRules::isUnlocked(*challenge, profile)) {
            continue;
        }
        if (ChallengeRules::isCompleted(*challenge, run)) {
            completedIds.push_back(challenge->id);
        }
    }

    return completedIds;
}
