#include "RunEventDefinitionParser.hpp"

#include "data/JsonReader.hpp"

#include <stdexcept>
#include <string>

namespace {
RunEventEffectType parseEffectType(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "gain_gold") return RunEventEffectType::GainGold;
    if (value == "lose_gold") return RunEventEffectType::LoseGold;
    if (value == "gain_random_card") return RunEventEffectType::GainRandomCard;
    if (value == "gain_random_relic") return RunEventEffectType::GainRandomRelic;
    if (value == "gain_random_consumable") return RunEventEffectType::GainRandomConsumable;
    if (value == "gain_stress") return RunEventEffectType::GainStress;
    if (value == "lose_stress") return RunEventEffectType::LoseStress;
    if (value == "skip") return RunEventEffectType::Skip;

    throw std::runtime_error(sourcePath.string() + ": Unknown run event effect type '" + value + "'");
}

RunEventEffect parseEffect(const Json& json, const std::filesystem::path& sourcePath) {
    JsonReader reader(json, sourcePath);
    RunEventEffect effect;
    effect.type = parseEffectType(reader.requiredString("type"), sourcePath);
    effect.amount = reader.optionalInt("amount", 0);
    return effect;
}

RunEventChoiceDefinition parseChoice(const Json& json, const std::filesystem::path& sourcePath) {
    JsonReader reader(json, sourcePath);
    RunEventChoiceDefinition choice;
    choice.textTextId = TextId(reader.requiredString("text"));
    choice.descriptionTextId = TextId(reader.optionalString("description", ""));

    const Json& effectsJson = reader.optionalArray("effects");
    for (const Json& effectJson : effectsJson) {
        choice.effects.push_back(parseEffect(effectJson, sourcePath));
    }

    return choice;
}
}

RunEventDefinition RunEventDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    RunEventDefinition definition;
    definition.id = reader.requiredString("id");
    definition.titleTextId = TextId(reader.requiredString("title"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));

    const Json& choicesJson = reader.requiredArray("choices");
    for (const Json& choiceJson : choicesJson) {
        definition.choices.push_back(parseChoice(choiceJson, sourcePath));
    }

    if (definition.choices.empty()) {
        throw std::runtime_error(sourcePath.string() + ": Run event '" + definition.id + "' must have at least one choice");
    }

    return definition;
}
