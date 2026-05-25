#include "PlayerActorParser.hpp"

#include "data/JsonReader.hpp"

#include <stdexcept>

PlayerActorDefinition PlayerActorParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    PlayerActorDefinition definition;
    definition.id = PlayerActorId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.maxHp = reader.requiredInt("max_hp");
    definition.startingEnergy = reader.optionalInt("starting_energy", 3);
    definition.startingTraitIds = reader.optionalStringArray("starting_traits");
    definition.startingRelicIds = reader.optionalStringArray("starting_relics");

    if (definition.maxHp <= 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': actor max_hp must be positive"
        );
    }

    if (definition.startingEnergy < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': actor starting_energy must not be negative"
        );
    }

    return definition;
}
