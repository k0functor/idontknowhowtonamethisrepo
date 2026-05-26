#include "ConsumableDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectDefinitionParser.hpp"

#include <stdexcept>

namespace {
std::vector<EffectDefinition> parseEffects(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    const Json& effectsJson = reader.requiredArray("effects");

    std::vector<EffectDefinition> effects;
    effects.reserve(effectsJson.size());

    for (const Json& effectJson : effectsJson) {
        effects.push_back(EffectDefinitionParser::parse(effectJson, sourcePath));
    }

    return effects;
}
}

ConsumableDefinition ConsumableDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    ConsumableDefinition definition;
    definition.id = ConsumableId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.rarity = consumableRarityFromString(reader.requiredString("rarity"));
    definition.goldCost = reader.optionalInt("gold_cost", 0);
    definition.effects = parseEffects(reader, sourcePath);

    if (definition.effects.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': consumable '" +
            definition.id.value + "' must have at least one effect"
        );
    }

    return definition;
}
