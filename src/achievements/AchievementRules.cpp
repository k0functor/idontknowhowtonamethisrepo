#include "AchievementRules.hpp"

#include <algorithm>

namespace {
bool containsId(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool floorMatches(const AchievementCompletionCondition& completion, const RunState& run) {
    return completion.floorId.empty() || completion.floorId == run.currentFloorId;
}

bool archetypeMatches(const AchievementCompletionCondition& completion, const RunState& run) {
    return completion.archetypeId.empty() || completion.archetypeId == run.archetypeId.value;
}

bool runStatsMatch(const AchievementCompletionCondition& completion, const RunState& run) {
    if (completion.minElitesKilled > 0 && run.stats.elitesKilled < completion.minElitesKilled) {
        return false;
    }

    if (completion.minBossesKilled > 0 && run.stats.bossesKilled < completion.minBossesKilled) {
        return false;
    }

    if (completion.minEventsCompleted > 0 && run.stats.eventsCompleted < completion.minEventsCompleted) {
        return false;
    }

    if (completion.minRelics > 0 && static_cast<int>(run.relicIds.size()) < completion.minRelics) {
        return false;
    }

    if (completion.minGold > 0 && run.gold < completion.minGold) {
        return false;
    }

    if (completion.maxDamageTaken >= 0 && run.stats.damageTaken > completion.maxDamageTaken) {
        return false;
    }

    return true;
}

bool floorClearConditionMatches(const AchievementCompletionCondition& completion, const RunState* run) {
    if (run == nullptr || !run->actCompleted) {
        return false;
    }

    return floorMatches(completion, *run) && archetypeMatches(completion, *run) && runStatsMatch(completion, *run);
}
}

bool AchievementRules::isUnlocked(const AchievementDefinition& achievement, const ProfileData& profile) {
    if (!achievement.isAvailable) {
        return false;
    }

    for (const std::string& archetypeId : achievement.requiredUnlockedArchetypeIds) {
        if (!containsId(profile.unlockedArchetypeIds, archetypeId)) {
            return false;
        }
    }

    for (const std::string& challengeId : achievement.requiredCompletedChallengeIds) {
        if (!containsId(profile.completedChallengeIds, challengeId)) {
            return false;
        }
    }

    for (const std::string& achievementId : achievement.requiredCompletedAchievementIds) {
        if (!containsId(profile.completedAchievementIds, achievementId)) {
            return false;
        }
    }

    return true;
}

bool AchievementRules::isCompleted(
    const AchievementDefinition& achievement,
    const ProfileData& profile,
    const RunState* run
) {
    const AchievementCompletionCondition& completion = achievement.completion;

    if (completion.type == "clear_floor") {
        return floorClearConditionMatches(completion, run);
    }

    if (completion.type == "clear_floor_with_archetype") {
        return !completion.archetypeId.empty() && floorClearConditionMatches(completion, run);
    }

    if (completion.type == "clear_floor_with_elites") {
        return completion.minElitesKilled > 0 && floorClearConditionMatches(completion, run);
    }

    if (completion.type == "clear_floor_low_damage") {
        return completion.maxDamageTaken >= 0 && floorClearConditionMatches(completion, run);
    }

    if (completion.type == "complete_challenges") {
        return completion.minCompletedChallenges > 0 &&
               static_cast<int>(profile.completedChallengeIds.size()) >= completion.minCompletedChallenges;
    }

    if (completion.type == "complete_achievements") {
        return completion.minCompletedAchievements > 0 &&
               static_cast<int>(profile.completedAchievementIds.size()) >= completion.minCompletedAchievements;
    }

    if (completion.type == "win_runs") {
        return completion.minVictories > 0 && profile.victories >= completion.minVictories;
    }

    if (completion.type == "lose_runs") {
        return completion.minDefeats > 0 && profile.defeats >= completion.minDefeats;
    }

    return false;
}
