#include "RelicDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectDefinitionParser.hpp"

#include <stdexcept>

namespace {
RelicModifierDefinition parseModifier(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    RelicModifierDefinition modifier;
    modifier.type = relicModifierTypeFromString(reader.requiredString("type"));
    modifier.amount = reader.optionalInt("amount", 0);
    modifier.multiplier = reader.optionalDouble("multiplier", 1.0);
    modifier.playerOnly = reader.optionalBool("player_only", true);
    modifier.priority = reader.optionalInt("priority", 500);

    return modifier;
}

std::vector<RelicModifierDefinition> parseModifiers(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    const Json& modifiersJson = reader.optionalArray("modifiers");

    std::vector<RelicModifierDefinition> result;
    result.reserve(modifiersJson.size());

    for (const Json& modifierJson : modifiersJson) {
        result.push_back(parseModifier(modifierJson, sourcePath));
    }

    return result;
}

RelicTriggerDefinition parseTrigger(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    RelicTriggerDefinition trigger;
    trigger.eventType = gameEventTypeFromString(reader.requiredString("event"));
    trigger.everyNTurns = reader.optionalInt("every_n_turns", 0);
    trigger.oncePerCombat = reader.optionalBool("once_per_combat", false);

    if (trigger.everyNTurns < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': every_n_turns must not be negative"
        );
    }

    const Json& effectsJson = reader.optionalArray("effects");
    trigger.effects.reserve(effectsJson.size());

    for (const Json& effectJson : effectsJson) {
        trigger.effects.push_back(EffectDefinitionParser::parse(effectJson, sourcePath));
    }

    return trigger;
}

std::vector<RelicTriggerDefinition> parseTriggers(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    const Json& triggersJson = reader.optionalArray("triggers");

    std::vector<RelicTriggerDefinition> result;
    result.reserve(triggersJson.size());

    for (const Json& triggerJson : triggersJson) {
        result.push_back(parseTrigger(triggerJson, sourcePath));
    }

    return result;
}
}

RelicDefinition RelicDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    RelicDefinition definition;
    definition.id = RelicId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.rarity = relicRarityFromString(reader.requiredString("rarity"));
    definition.mechanicId = reader.optionalString("mechanic_id", "default");
    definition.modifiers = parseModifiers(reader, sourcePath);
    definition.triggers = parseTriggers(reader, sourcePath);

    return definition;
}
