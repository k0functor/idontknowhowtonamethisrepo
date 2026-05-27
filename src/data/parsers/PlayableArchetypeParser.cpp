#include "PlayableArchetypeParser.hpp"

#include "data/JsonReader.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace {
std::uint8_t parseColorChannel(
    const JsonReader& reader,
    const std::string& key
) {
    const int value = reader.requiredInt(key);
    if (value < 0 || value > 255) {
        throw std::runtime_error("Archetype palette color channel '" + key + "' must be between 0 and 255");
    }

    return static_cast<std::uint8_t>(value);
}

ArchetypePaletteDefinition parsePalette(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    ArchetypePaletteDefinition palette;
    palette.nameTextId = TextId("archetype.palette.neutral");

    if (!reader.has("palette")) {
        return palette;
    }

    JsonReader paletteReader(reader.requiredObject("palette"), sourcePath);
    palette.nameTextId = TextId(paletteReader.optionalString("name", "archetype.palette.neutral"));

    if (paletteReader.has("accent")) {
        JsonReader accentReader(paletteReader.requiredObject("accent"), sourcePath);
        palette.accentR = parseColorChannel(accentReader, "r");
        palette.accentG = parseColorChannel(accentReader, "g");
        palette.accentB = parseColorChannel(accentReader, "b");
    }

    return palette;
}

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
    definition.visualIdentityTextId = TextId(reader.optionalString("visual_identity", ""));
    definition.palette = parsePalette(reader, sourcePath);

    definition.actorDefinitionIds = reader.requiredStringArray("actors");
    definition.startingDeckCardIds = reader.requiredStringArray("starting_deck");
    definition.startingRelicIds = reader.optionalStringArray("starting_relics");
    definition.startingConsumableIds = reader.optionalStringArray("starting_consumables");
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
