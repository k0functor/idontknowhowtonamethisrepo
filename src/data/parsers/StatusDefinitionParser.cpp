#include "StatusDefinitionParser.hpp"

#include "data/JsonReader.hpp"

StatusDefinition StatusDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    StatusDefinition definition;
    definition.id = StatusId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.type = statusTypeFromString(reader.optionalString("type", "neutral"));
    definition.durationRule = statusDurationRuleFromString(
        reader.optionalString("duration_rule", "persistent_combat")
    );
    definition.endTurnEffect = reader.optionalString("end_turn_effect", "");
    definition.decreaseAfterTrigger = reader.optionalBool("decrease_after_trigger", false);

    return definition;
}
