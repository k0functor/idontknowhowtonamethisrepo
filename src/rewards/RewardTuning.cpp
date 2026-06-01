#include "RewardTuning.hpp"

#include "cards/CardRarity.hpp"
#include "data/JsonLoader.hpp"
#include "data/JsonReader.hpp"

#include <stdexcept>
#include <string>

namespace {
RunMapNodeType nodeTypeFromConfigKey(const std::string& key) {
    if (key == "combat") return RunMapNodeType::Combat;
    if (key == "elite") return RunMapNodeType::Elite;
    if (key == "event") return RunMapNodeType::Event;
    if (key == "shop") return RunMapNodeType::Shop;
    if (key == "chest") return RunMapNodeType::Chest;
    if (key == "rest") return RunMapNodeType::Rest;
    if (key == "boss") return RunMapNodeType::Boss;

    throw std::runtime_error("Unknown reward node type in reward tuning: '" + key + "'");
}

NodeRewardTuning parseNodeTuning(const Json& json, const std::filesystem::path& filePath) {
    const JsonReader reader(json, filePath);

    NodeRewardTuning result;
    result.gold = reader.optionalInt("gold", 0);
    result.offerCards = reader.optionalBool("offer_cards", false);
    result.cardChoices = reader.optionalInt("card_choices", result.offerCards ? 3 : 0);
    result.guaranteedRelic = reader.optionalBool("guaranteed_relic", false);
    result.consumableChancePercent = reader.optionalInt("consumable_chance_percent", 0);

    const std::string minimumCardRarity = reader.optionalString("minimum_card_rarity", "");
    if (!minimumCardRarity.empty()) {
        result.minimumCardRarity = cardRarityFromString(minimumCardRarity);
    }

    if (result.gold < 0) {
        throw std::runtime_error(filePath.string() + ": reward gold must not be negative");
    }

    if (result.cardChoices < 0) {
        throw std::runtime_error(filePath.string() + ": card_choices must not be negative");
    }

    if (result.consumableChancePercent < 0 || result.consumableChancePercent > 100) {
        throw std::runtime_error(filePath.string() + ": consumable_chance_percent must be between 0 and 100");
    }

    return result;
}
}

RewardTuning::RewardTuning() {
    combat_ = NodeRewardTuning{18, true, 3, false, 30, std::nullopt};
    elite_ = NodeRewardTuning{35, true, 3, true, 45, std::nullopt};
    boss_ = NodeRewardTuning{75, true, 3, true, 0, CardRarity::Uncommon};
    chest_ = NodeRewardTuning{0, false, 0, false, 0, std::nullopt};
    event_ = NodeRewardTuning{0, false, 0, false, 0, std::nullopt};
    shop_ = NodeRewardTuning{0, false, 0, false, 0, std::nullopt};
    rest_ = NodeRewardTuning{0, false, 0, false, 0, std::nullopt};
}

void RewardTuning::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadObjectFromFile(filePath);
    const JsonReader reader(root, filePath);

    merchantGoldMultiplier_ = reader.optionalDouble("merchant_gold_multiplier", merchantGoldMultiplier_);
    if (merchantGoldMultiplier_ < 0.0) {
        throw std::runtime_error(filePath.string() + ": merchant_gold_multiplier must not be negative");
    }

    const Json& nodes = reader.requiredObject("nodes");
    for (const auto& [key, value] : nodes.items()) {
        if (!value.is_object()) {
            throw std::runtime_error(filePath.string() + ": node reward tuning '" + key + "' must be an object");
        }

        mutableNode(nodeTypeFromConfigKey(key)) = parseNodeTuning(value, filePath);
    }
}

const NodeRewardTuning& RewardTuning::node(const RunMapNodeType nodeType) const {
    switch (nodeType) {
        case RunMapNodeType::Combat: return combat_;
        case RunMapNodeType::Elite: return elite_;
        case RunMapNodeType::Event: return event_;
        case RunMapNodeType::Shop: return shop_;
        case RunMapNodeType::Chest: return chest_;
        case RunMapNodeType::Rest: return rest_;
        case RunMapNodeType::Boss: return boss_;
    }

    throw std::runtime_error("Unknown RunMapNodeType in RewardTuning::node");
}

NodeRewardTuning& RewardTuning::mutableNode(const RunMapNodeType nodeType) {
    switch (nodeType) {
        case RunMapNodeType::Combat: return combat_;
        case RunMapNodeType::Elite: return elite_;
        case RunMapNodeType::Event: return event_;
        case RunMapNodeType::Shop: return shop_;
        case RunMapNodeType::Chest: return chest_;
        case RunMapNodeType::Rest: return rest_;
        case RunMapNodeType::Boss: return boss_;
    }

    throw std::runtime_error("Unknown RunMapNodeType in RewardTuning::mutableNode");
}

double RewardTuning::merchantGoldMultiplier() const {
    return merchantGoldMultiplier_;
}
