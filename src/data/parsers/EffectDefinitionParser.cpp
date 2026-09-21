#include "EffectDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectValueParser.hpp"

#include <stdexcept>

EffectScalingDefinition parseScaling(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    EffectScalingDefinition scaling;
    if (!reader.has("scaling")) {
        return scaling;
    }

    const Json& json = reader.requiredObject("scaling");
    JsonReader scalingReader(json, sourcePath);

    if (scalingReader.has("status")) {
        scaling.statusId = scalingReader.requiredString("status");
        const std::string owner = scalingReader.optionalString("status_owner", "source");
        if (owner == "source") {
            scaling.statusOwner = EffectScalingStatusOwner::Source;
        } else if (owner == "target") {
            scaling.statusOwner = EffectScalingStatusOwner::Target;
        } else {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() +
                "': scaling status_owner must be 'source' or 'target'"
            );
        }
    } else if (scalingReader.has("status_owner")) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': scaling status_owner requires scaling status"
        );
    }

    scaling.bonusIfStatusPresent = scalingReader.has("bonus_if_status_present")
        ? scalingReader.requiredInt("bonus_if_status_present")
        : scalingReader.optionalInt("bonus_if_present", 0);
    scaling.bonusPerStatusStack = scalingReader.has("bonus_per_status_stack")
        ? scalingReader.requiredInt("bonus_per_status_stack")
        : scalingReader.optionalInt("bonus_per_stack", 0);
    scaling.bonusPerCardInHand = scalingReader.optionalInt("bonus_per_card_in_hand", 0);
    scaling.bonusPerCardInDiscard = scalingReader.optionalInt("bonus_per_card_in_discard", 0);
    scaling.maximumBonus = scalingReader.optionalInt("maximum_bonus", -1);

    if (scaling.bonusIfStatusPresent < 0 ||
        scaling.bonusPerStatusStack < 0 ||
        scaling.bonusPerCardInHand < 0 ||
        scaling.bonusPerCardInDiscard < 0 ||
        scaling.maximumBonus < -1) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': effect scaling values must be non-negative and maximum_bonus must be -1 or greater"
        );
    }

    if ((scaling.bonusIfStatusPresent > 0 || scaling.bonusPerStatusStack > 0) &&
        !scaling.statusId.has_value()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': status-based effect scaling requires a status id"
        );
    }

    return scaling;
}

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

    if (reader.has("output")) {
        definition.outputAmount = reader.requiredInt("output");
    }

    if (reader.has("status")) {
        definition.statusId = reader.requiredString("status");
    }

    definition.scaling = parseScaling(reader, sourcePath);

    if (definition.type == EffectType::PrimeStressBreakdown) {
        if (!definition.statusId.has_value()) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() +
                "': prime_stress_breakdown requires breakdown_type in status"
            );
        }
        const std::string& type = *definition.statusId;
        if (type != "discard" && type != "energy" && type != "status_cards" && type != "cost" && type != "frenzy") {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() +
                "': invalid prime_stress_breakdown type '" + type + "'"
            );
        }
    }

    if (isStressConversionEffect(definition.type)) {
        if (!definition.value.isFixed() || definition.value.fixedAmount() <= 0) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() +
                "': stress conversion effects require a positive fixed stress cost"
            );
        }
        if (definition.outputAmount <= 0) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() +
                "': stress conversion effects require a positive output"
            );
        }
    } else if (definition.outputAmount != 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': output is only valid for stress conversion effects"
        );
    }

    return definition;
}
