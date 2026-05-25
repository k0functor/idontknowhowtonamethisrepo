#include "DifficultyDefinitionParser.hpp"

#include "data/JsonReader.hpp"

DifficultyDefinition DifficultyDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    DifficultyDefinition definition;
    definition.id = DifficultyId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));

    definition.enemyHpMultiplier = static_cast<float>(reader.optionalDouble("enemy_hp_multiplier", 1.0));
    definition.enemyDamageMultiplier = static_cast<float>(reader.optionalDouble("enemy_damage_multiplier", 1.0));
    definition.goldMultiplier = static_cast<float>(reader.optionalDouble("gold_multiplier", 1.0));

    return definition;
}
