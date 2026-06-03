#include "CardDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectDefinitionParser.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<CardKeyword> parseKeywords(const JsonReader& reader) {
    std::vector<CardKeyword> result;

    const std::vector<std::string> keywordStrings =
        reader.optionalStringArray("keywords");

    result.reserve(keywordStrings.size());

    for (const std::string& keyword : keywordStrings) {
        result.push_back(cardKeywordFromString(keyword));
    }

    return result;
}

DiceCorruption parseDiceCorruption(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    if (!reader.has("dice_corruption")) {
        return DiceCorruption{};
    }

    const Json& json = reader.requiredObject("dice_corruption");
    JsonReader corruptionReader(json, sourcePath);

    DiceCorruption corruption;
    corruption.type = diceCorruptionTypeFromString(
        corruptionReader.requiredString("type")
    );

    return corruption;
}

std::vector<EffectDefinition> parseEffects(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    const Json& effectsJson = reader.requiredArray("effects");

    std::vector<EffectDefinition> effects;
    effects.reserve(effectsJson.size());

    for (const Json& effectJson : effectsJson) {
        effects.push_back(
            EffectDefinitionParser::parse(effectJson, sourcePath)
        );
    }

    return effects;
}


CardUpgradeDefinition parseUpgrade(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    CardUpgradeDefinition upgrade;

    if (!reader.has("upgrade")) {
        return upgrade;
    }

    const Json& json = reader.requiredObject("upgrade");
    JsonReader upgradeReader(json, sourcePath);

    if (upgradeReader.has("name")) {
        upgrade.nameTextId = TextId(upgradeReader.requiredString("name"));
    }
    if (upgradeReader.has("description")) {
        upgrade.descriptionTextId = TextId(upgradeReader.requiredString("description"));
    }
    if (upgradeReader.has("energy_cost")) {
        upgrade.energyCost = upgradeReader.requiredInt("energy_cost");
    }
    if (upgradeReader.has("gold_cost")) {
        upgrade.goldCost = upgradeReader.requiredInt("gold_cost");
    }
    if (upgradeReader.has("keywords")) {
        upgrade.keywords = parseKeywords(upgradeReader);
    }
    if (upgradeReader.has("dice_corruption")) {
        upgrade.diceCorruption = parseDiceCorruption(upgradeReader, sourcePath);
    }
    if (upgradeReader.has("effects")) {
        upgrade.effects = parseEffects(upgradeReader, sourcePath);
    }

    if (upgrade.energyCost.has_value() && *upgrade.energyCost < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': card upgrade energy_cost must not be negative"
        );
    }

    if (upgrade.goldCost.has_value() && *upgrade.goldCost < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': card upgrade gold_cost must not be negative"
        );
    }

    return upgrade;
}
}

CardDefinition CardDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    CardDefinition definition;

    definition.id = CardId(reader.requiredString("id"));

    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));

    definition.rarity = cardRarityFromString(reader.requiredString("rarity"));
    definition.type = cardTypeFromString(reader.requiredString("type"));

    definition.energyCost = reader.requiredInt("energy_cost");
    definition.goldCost = reader.requiredInt("gold_cost");
    definition.ownerActorId = reader.optionalString("owner_actor", reader.optionalString("energy_pool", ""));
    definition.rewardPoolId = reader.optionalString("card_pool", reader.optionalString("reward_pool", definition.ownerActorId));

    if (definition.energyCost < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': card energy_cost must not be negative"
        );
    }

    if (definition.goldCost < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': card gold_cost must not be negative"
        );
    }

    definition.keywords = parseKeywords(reader);
    definition.diceCorruption = parseDiceCorruption(reader, sourcePath);
    definition.effects = parseEffects(reader, sourcePath);
    definition.upgrade = parseUpgrade(reader, sourcePath);

    if (definition.effects.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() +
            "': card '" + definition.id.value + "' must have at least one effect"
        );
    }

    return definition;
}
