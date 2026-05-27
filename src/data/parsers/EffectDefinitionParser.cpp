#include "EffectDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectValueParser.hpp"

#include <stdexcept>

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

    if (reader.has("repeat_count")) {
        definition.repeatCount = reader.requiredInt("repeat_count");
    } else if (reader.has("times")) {
        definition.repeatCount = reader.requiredInt("times");
    }

    if (definition.repeatCount <= 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': effect repeat_count must be positive"
        );
    }

    if (reader.has("status")) {
        definition.statusId = reader.requiredString("status");
    }

    return definition;
}
