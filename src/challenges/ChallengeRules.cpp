#include "ChallengeRules.hpp"

#include <algorithm>

namespace {
bool containsId(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool floorMatches(const ChallengeCompletionCondition& completion, const RunState& run) {
    return completion.floorId.empty() || completion.floorId == run.currentFloorId;
}

bool archetypeMatches(const ChallengeCompletionCondition& completion, const RunState& run) {
    return completion.archetypeId.empty() || completion.archetypeId == run.archetypeId.value;
}

bool statCapsMatch(const ChallengeCompletionCondition& completion, const RunState& run) {
    if (completion.minElitesKilled > 0 && run.stats.elitesKilled < completion.minElitesKilled) {
        return false;
    }

    if (completion.minBossesKilled > 0 && run.stats.bossesKilled < completion.minBossesKilled) {
        return false;
    }

    if (completion.maxShopsVisited >= 0 && run.stats.shopsVisited > completion.maxShopsVisited) {
        return false;
    }

    if (completion.maxDamageTaken >= 0 && run.stats.damageTaken > completion.maxDamageTaken) {
        return false;
    }

    return true;
}
}

bool ChallengeRules::isUnlocked(const ChallengeDefinition& challenge, const ProfileData& profile) {
    if (!challenge.isAvailable) {
        return false;
    }

    for (const std::string& archetypeId : challenge.requiredUnlockedArchetypeIds) {
        if (!containsId(profile.unlockedArchetypeIds, archetypeId)) {
            return false;
        }
    }

    for (const std::string& challengeId : challenge.requiredCompletedChallengeIds) {
        if (!containsId(profile.completedChallengeIds, challengeId)) {
            return false;
        }
    }

    return true;
}

bool ChallengeRules::isCompleted(const ChallengeDefinition& challenge, const RunState& run) {
    if (run.challengeId != challenge.id) {
        return false;
    }

    if (!run.actCompleted) {
        return false;
    }

    const ChallengeCompletionCondition& completion = challenge.completion;
    if (!floorMatches(completion, run) || !archetypeMatches(completion, run) || !statCapsMatch(completion, run)) {
        return false;
    }

    if (completion.type == "clear_floor") {
        return true;
    }

    if (completion.type == "clear_floor_without_shop") {
        return completion.maxShopsVisited >= 0 && run.stats.shopsVisited <= completion.maxShopsVisited;
    }

    if (completion.type == "clear_floor_with_elites") {
        return completion.minElitesKilled > 0 && run.stats.elitesKilled >= completion.minElitesKilled;
    }

    if (completion.type == "clear_floor_with_archetype") {
        return !completion.archetypeId.empty() && run.archetypeId.value == completion.archetypeId;
    }

    if (completion.type == "clear_floor_low_damage") {
        return completion.maxDamageTaken >= 0 && run.stats.damageTaken <= completion.maxDamageTaken;
    }

    return false;
}
