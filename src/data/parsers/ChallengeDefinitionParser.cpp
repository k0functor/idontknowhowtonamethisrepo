#include "ChallengeDefinitionParser.hpp"

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

ChallengeCompletionCondition parseCompletion(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    ChallengeCompletionCondition completion;
    completion.type = reader.requiredString("type");
    completion.floorId = reader.optionalString("floor_id", "");
    completion.archetypeId = reader.optionalString("archetype_id", "");
    completion.minElitesKilled = reader.optionalInt("min_elites_killed", 0);
    completion.minBossesKilled = reader.optionalInt("min_bosses_killed", 0);
    completion.maxShopsVisited = reader.optionalInt("max_shops_visited", -1);
    completion.maxDamageTaken = reader.optionalInt("max_damage_taken", -1);
    return completion;
}
}

ChallengeDefinition ChallengeDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    ChallengeDefinition definition;
    definition.id = reader.requiredString("id");
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.goalTextId = TextId(reader.requiredString("goal"));
    definition.rewardTextId = TextId(reader.optionalString("reward", "challenge.reward.profile_badge"));
    definition.unlockHintTextId = TextId(reader.optionalString("unlock_hint", "challenge.unlock_hint.default"));
    definition.reward = parseUnlockReward(reader.optionalObject("rewards"), sourcePath);
    definition.isAvailable = reader.optionalBool("is_available", true);
    definition.selectionOrder = reader.optionalInt("selection_order", 0);
    definition.requiredUnlockedArchetypeIds = reader.optionalStringArray("required_unlocked_archetypes");
    definition.requiredCompletedChallengeIds = reader.optionalStringArray("required_completed_challenges");
    definition.completion = parseCompletion(reader.requiredObject("completion"), sourcePath);

    const std::string defaultArchetype = definition.completion.archetypeId.empty()
        ? std::string("rusted_knight")
        : definition.completion.archetypeId;
    definition.startingArchetypeId = reader.optionalString("starting_archetype_id", defaultArchetype);
    definition.startingDifficultyId = reader.optionalString("starting_difficulty_id", "normal");
    definition.startingFloorId = reader.optionalString("starting_floor_id", "");
    definition.startingGoldOverride = reader.optionalInt("starting_gold", -1);
    definition.fixedStartingDeckCardIds = reader.optionalStringArray("fixed_starting_deck");
    definition.fixedStartingRelicIds = reader.optionalStringArray("fixed_starting_relics");
    definition.fixedStartingConsumableIds = reader.optionalStringArray("fixed_starting_consumables");

    return definition;
}
