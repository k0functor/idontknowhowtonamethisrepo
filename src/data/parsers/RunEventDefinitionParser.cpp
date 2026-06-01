#include "RunEventDefinitionParser.hpp"

#include "data/JsonReader.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
RunEventEffectType parseEffectType(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "gain_gold") return RunEventEffectType::GainGold;
    if (value == "lose_gold") return RunEventEffectType::LoseGold;
    if (value == "gain_card") return RunEventEffectType::GainCard;
    if (value == "gain_random_card") return RunEventEffectType::GainRandomCard;
    if (value == "gain_relic") return RunEventEffectType::GainRelic;
    if (value == "gain_random_relic") return RunEventEffectType::GainRandomRelic;
    if (value == "gain_consumable") return RunEventEffectType::GainConsumable;
    if (value == "gain_random_consumable") return RunEventEffectType::GainRandomConsumable;
    if (value == "remove_card") return RunEventEffectType::RemoveCard;
    if (value == "remove_random_card") return RunEventEffectType::RemoveRandomCard;
    if (value == "gain_stress") return RunEventEffectType::GainStress;
    if (value == "lose_stress") return RunEventEffectType::LoseStress;
    if (value == "lose_hp") return RunEventEffectType::LoseHp;
    if (value == "heal_all") return RunEventEffectType::HealAll;
    if (value == "skip") return RunEventEffectType::Skip;

    throw std::runtime_error(sourcePath.string() + ": Unknown run event effect type '" + value + "'");
}


std::vector<std::string> optionalStringOrArray(
    const Json& json,
    const std::filesystem::path& sourcePath,
    const std::string& singleKey,
    const std::string& arrayKey
) {
    JsonReader reader(json, sourcePath);
    std::vector<std::string> values;

    if (reader.has(singleKey)) {
        values.push_back(reader.requiredString(singleKey));
    }

    const std::vector<std::string> arrayValues = reader.optionalStringArray(arrayKey);
    values.insert(values.end(), arrayValues.begin(), arrayValues.end());
    return values;
}

RunEventChoiceRequirements parseRequirements(const Json& json, const std::filesystem::path& sourcePath) {
    JsonReader reader(json, sourcePath);
    const Json& requirementsJson = reader.optionalObject("requirements");
    if (requirementsJson.empty()) {
        return {};
    }

    JsonReader requirementsReader(requirementsJson, sourcePath);
    RunEventChoiceRequirements requirements;
    requirements.minGold = std::max(0, requirementsReader.optionalInt("min_gold", 0));
    requirements.minHp = std::max(0, requirementsReader.optionalInt("min_hp", 0));
    requirements.minDeckSize = std::max(0, requirementsReader.optionalInt("min_deck_size", 0));
    requirements.freeConsumableSlot =
        requirementsReader.optionalBool("free_consumable_slot", false) ||
        requirementsReader.optionalBool("requires_free_consumable_slot", false);
    requirements.requiredRelicIds = optionalStringOrArray(
        requirementsJson,
        sourcePath,
        "has_relic",
        "has_relics"
    );
    requirements.forbiddenRelicIds = optionalStringOrArray(
        requirementsJson,
        sourcePath,
        "missing_relic",
        "missing_relics"
    );
    requirements.requiredCardIds = optionalStringOrArray(
        requirementsJson,
        sourcePath,
        "has_card",
        "has_cards"
    );
    requirements.forbiddenCardIds = optionalStringOrArray(
        requirementsJson,
        sourcePath,
        "missing_card",
        "missing_cards"
    );
    return requirements;
}

std::string optionalContentId(
    const JsonReader& reader,
    const std::vector<std::string>& keys
) {
    for (const std::string& key : keys) {
        if (reader.has(key)) {
            return reader.requiredString(key);
        }
    }

    return {};
}

bool effectNeedsContentId(const RunEventEffectType type) {
    switch (type) {
        case RunEventEffectType::GainCard:
        case RunEventEffectType::GainRelic:
        case RunEventEffectType::GainConsumable:
        case RunEventEffectType::RemoveCard:
            return true;

        case RunEventEffectType::GainGold:
        case RunEventEffectType::LoseGold:
        case RunEventEffectType::GainRandomCard:
        case RunEventEffectType::GainRandomRelic:
        case RunEventEffectType::GainRandomConsumable:
        case RunEventEffectType::RemoveRandomCard:
        case RunEventEffectType::GainStress:
        case RunEventEffectType::LoseStress:
        case RunEventEffectType::LoseHp:
        case RunEventEffectType::HealAll:
        case RunEventEffectType::Skip:
            return false;
    }

    return false;
}

RunEventEffect parseEffect(const Json& json, const std::filesystem::path& sourcePath) {
    JsonReader reader(json, sourcePath);
    RunEventEffect effect;
    effect.type = parseEffectType(reader.requiredString("type"), sourcePath);
    effect.amount = reader.optionalInt("amount", 0);
    effect.contentId = optionalContentId(
        reader,
        {"content_id", "id", "card_id", "relic_id", "consumable_id"}
    );

    if (effectNeedsContentId(effect.type) && effect.contentId.empty()) {
        throw std::runtime_error(sourcePath.string() + ": Run event effect '" + reader.requiredString("type") + "' requires a content id");
    }

    return effect;
}

RunEventChoiceDefinition parseChoice(const Json& json, const std::filesystem::path& sourcePath) {
    JsonReader reader(json, sourcePath);
    RunEventChoiceDefinition choice;
    choice.textTextId = TextId(reader.requiredString("text"));
    choice.descriptionTextId = TextId(reader.optionalString("description", ""));

    choice.requirements = parseRequirements(json, sourcePath);

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
