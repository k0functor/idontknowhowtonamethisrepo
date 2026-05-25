#include "PlayableArchetypeParser.hpp"

#include "data/JsonReader.hpp"

#include <stdexcept>

namespace {
std::vector<TextId> parseTextIds(
    const JsonReader& reader,
    const std::string& key
) {
    const std::vector<std::string> rawValues = reader.optionalStringArray(key);

    std::vector<TextId> result;
    result.reserve(rawValues.size());

    for (const std::string& value : rawValues) {
        result.emplace_back(value);
    }

    return result;
}
}

PlayableArchetypeDefinition PlayableArchetypeParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    PlayableArchetypeDefinition definition;
    definition.id = PlayableArchetypeId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.shortDescriptionTextId = TextId(reader.requiredString("short_description"));
    definition.detailsDescriptionTextId = TextId(reader.requiredString("details_description"));
    definition.uniqueMechanicTextId = TextId(reader.requiredString("unique_mechanic"));

    definition.actorDefinitionIds = reader.requiredStringArray("actors");
    definition.startingDeckCardIds = reader.requiredStringArray("starting_deck");
    definition.startingRelicIds = reader.optionalStringArray("starting_relics");
    definition.startingGold = reader.requiredInt("starting_gold");
    definition.strengthTextIds = parseTextIds(reader, "strengths");
    definition.weaknessTextIds = parseTextIds(reader, "weaknesses");
    definition.mechanicId = reader.optionalString("mechanic_id", "default");

    if (definition.actorDefinitionIds.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': archetype '" + definition.id.value + "' must define at least one actor"
        );
    }

    if (definition.startingDeckCardIds.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': archetype '" + definition.id.value + "' must define a starting deck"
        );
    }

    if (definition.startingGold < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': archetype starting_gold must not be negative"
        );
    }

    return definition;
}
