#pragma once

#include "challenges/ChallengeDefinition.hpp"
#include "profile/ProfileData.hpp"
#include "run/RunState.hpp"

class ChallengeRules {
public:
    static bool isUnlocked(const ChallengeDefinition& challenge, const ProfileData& profile);
    static bool isCompleted(const ChallengeDefinition& challenge, const RunState& run);
};
