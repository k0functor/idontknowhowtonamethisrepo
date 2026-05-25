#include "EnemyDefinitionParser.hpp"

#include "data/JsonReader.hpp"

#include <stdexcept>

EnemyDefinition EnemyDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    EnemyDefinition definition;
    definition.id = EnemyId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.maxHp = reader.requiredInt("max_hp");
    definition.startingBlock = reader.optionalInt("starting_block", 0);

    if (definition.id.value.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy id must not be empty"
        );
    }

    if (definition.maxHp <= 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy max_hp must be positive"
        );
    }

    if (definition.startingBlock < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy starting_block must not be negative"
        );
    }

    return definition;
}
