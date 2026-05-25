#include "EffectValueParser.hpp"

#include "data/JsonReader.hpp"
#include "dice/DieType.hpp"

#include <stdexcept>

EffectValue EffectValueParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    const std::string type = reader.requiredString("type");

    if (type == "fixed") {
        return EffectValue::fixed(reader.requiredInt("amount"));
    }

    if (type == "dice") {
        DiceExpression expression;
        expression.count = reader.optionalInt("count", 1);
        expression.dieType = dieTypeFromString(reader.requiredString("die"));
        expression.bonus = reader.optionalInt("bonus", 0);

        if (expression.count <= 0) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() +
                "': dice count must be positive"
            );
        }

        return EffectValue::dice(expression);
    }

    throw std::runtime_error(
        "JSON error in '" + sourcePath.string() +
        "': unknown effect value type '" + type + "'"
    );
}
