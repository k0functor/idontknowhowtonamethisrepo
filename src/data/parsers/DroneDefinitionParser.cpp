#include "DroneDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectDefinitionParser.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>

namespace {
EffectDefinition parseDroneEffect(
    const Json& json,
    const std::filesystem::path& sourcePath,
    const std::string& owner,
    const std::size_t index
) {
    if (!json.is_object()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': drone action '" +
            owner + "' effect #" + std::to_string(index) + " must be an object"
        );
    }

    if (!json.contains("type")) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': drone action '" +
            owner + "' effect #" + std::to_string(index) +
            " is missing required field 'type'"
        );
    }

    try {
        return EffectDefinitionParser::parse(json, sourcePath);
    } catch (const std::exception& exception) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': failed to parse drone action '" +
            owner + "' effect #" + std::to_string(index) + ": " + exception.what()
        );
    }
}

DroneActionDefinition parseAction(
    const Json& json,
    const std::filesystem::path& sourcePath,
    const std::string& owner
) {
    JsonReader reader(json, sourcePath);

    DroneActionDefinition action;
    action.logTextId = reader.optionalString("log_text", std::string{});
    action.fallbackLog = reader.optionalString("fallback_log", std::string{});

    const Json& effectsJson = reader.requiredArray("effects");
    action.effects.reserve(effectsJson.size());

    for (std::size_t index = 0; index < effectsJson.size(); ++index) {
        const Json& effectJson = effectsJson.at(index);
        action.effects.push_back(parseDroneEffect(effectJson, sourcePath, owner, index));
    }

    if (action.effects.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': drone action '" +
            owner + "' must have at least one effect"
        );
    }

    return action;
}
}

DroneDefinition DroneDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    DroneDefinition definition;
    definition.id = DroneId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));

    if (definition.id.value.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': drone id must not be empty"
        );
    }

    if (reader.has("manual")) {
        definition.manualAction = parseAction(
            reader.requiredObject("manual"),
            sourcePath,
            definition.id.value + ".manual"
        );
    }

    if (reader.has("end_turn")) {
        definition.endTurnAction = parseAction(
            reader.requiredObject("end_turn"),
            sourcePath,
            definition.id.value + ".end_turn"
        );
    }

    if (!definition.manualAction.has_value() && !definition.endTurnAction.has_value()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': drone '" +
            definition.id.value + "' must define at least one of 'manual' or 'end_turn'"
        );
    }

    return definition;
}
