#include "EffectDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectValueParser.hpp"

EffectDefinition EffectDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    EffectDefinition definition;

    definition.type = effectTypeFromString(reader.requiredString("type"));
    definition.target = effectTargetFromString(
        reader.optionalString("target", "self")
    );

    if (reader.has("value")) {
        definition.value = EffectValueParser::parse(
            reader.requiredObject("value"),
            sourcePath
        );
    }

    if (reader.has("status")) {
        definition.statusId = reader.requiredString("status");
    }

    return definition;
}
