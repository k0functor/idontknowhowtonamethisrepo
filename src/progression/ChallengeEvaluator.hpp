#pragma once

#include "challenges/ChallengeDatabase.hpp"
#include "profile/ProfileData.hpp"
#include "run/RunState.hpp"

#include <string>
#include <vector>

class ChallengeEvaluator {
public:
    static std::vector<std::string> findNewlyCompleted(
        const ChallengeDatabase& challenges,
        const ProfileData& profile,
        const RunState& run
    );
};
