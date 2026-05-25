#include "EnemyActionDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectDefinitionParser.hpp"

#include <stdexcept>

EnemyActionDefinition EnemyActionDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    EnemyActionDefinition definition;
    definition.id = reader.requiredString("id");
    definition.intentType = enemyIntentTypeFromString(
        reader.requiredString("intent")
    );

    if (definition.id.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action id must not be empty"
        );
    }

    const Json& effectsJson = reader.requiredArray("effects");
    definition.effects.reserve(effectsJson.size());

    for (const Json& effectJson : effectsJson) {
        definition.effects.push_back(
            EffectDefinitionParser::parse(effectJson, sourcePath)
        );
    }

    if (definition.effects.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            definition.id + "' must have at least one effect"
        );
    }

    return definition;
}
