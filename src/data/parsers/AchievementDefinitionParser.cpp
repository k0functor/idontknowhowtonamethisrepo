#include "AchievementDefinitionParser.hpp"

#include "data/JsonReader.hpp"

#include "profile/UnlockReward.hpp"

namespace {

UnlockReward parseUnlockReward(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    UnlockReward reward;
    reward.archetypeIds = reader.optionalStringArray("unlock_archetypes");
    reward.cardIds = reader.optionalStringArray("unlock_cards");
    reward.relicIds = reader.optionalStringArray("unlock_relics");
    return reward;
}

AchievementCompletionCondition parseCompletion(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    AchievementCompletionCondition completion;
    completion.type = reader.requiredString("type");
    completion.floorId = reader.optionalString("floor_id", "");
    completion.archetypeId = reader.optionalString("archetype_id", "");
    completion.minVictories = reader.optionalInt("min_victories", 0);
    completion.minDefeats = reader.optionalInt("min_defeats", 0);
    completion.minCompletedChallenges = reader.optionalInt("min_completed_challenges", 0);
    completion.minCompletedAchievements = reader.optionalInt("min_completed_achievements", 0);
    completion.minElitesKilled = reader.optionalInt("min_elites_killed", 0);
    completion.minBossesKilled = reader.optionalInt("min_bosses_killed", 0);
    completion.minEventsCompleted = reader.optionalInt("min_events_completed", 0);
    completion.minRelics = reader.optionalInt("min_relics", 0);
    completion.minGold = reader.optionalInt("min_gold", 0);
    completion.maxDamageTaken = reader.optionalInt("max_damage_taken", -1);
    return completion;
}
}

AchievementDefinition AchievementDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    AchievementDefinition definition;
    definition.id = reader.requiredString("id");
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.goalTextId = TextId(reader.requiredString("goal"));
    definition.rewardTextId = TextId(reader.optionalString("reward", "achievement.reward.profile_mark"));
    definition.unlockHintTextId = TextId(reader.optionalString("unlock_hint", "achievement.unlock_hint.default"));
    definition.reward = parseUnlockReward(reader.optionalObject("rewards"), sourcePath);
    definition.isAvailable = reader.optionalBool("is_available", true);
    definition.selectionOrder = reader.optionalInt("selection_order", 0);
    definition.requiredUnlockedArchetypeIds = reader.optionalStringArray("required_unlocked_archetypes");
    definition.requiredCompletedChallengeIds = reader.optionalStringArray("required_completed_challenges");
    definition.requiredCompletedAchievementIds = reader.optionalStringArray("required_completed_achievements");
    definition.completion = parseCompletion(reader.requiredObject("completion"), sourcePath);

    return definition;
}
