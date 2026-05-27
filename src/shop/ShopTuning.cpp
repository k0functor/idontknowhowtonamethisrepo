#include "ShopTuning.hpp"

#include "data/JsonLoader.hpp"
#include "data/JsonReader.hpp"

#include <stdexcept>
#include <string>

namespace {
void requireNonNegative(const std::filesystem::path& filePath, const std::string& key, const int value) {
    if (value < 0) {
        throw std::runtime_error(filePath.string() + ": '" + key + "' must not be negative");
    }
}
}

void ShopTuning::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadObjectFromFile(filePath);
    const JsonReader reader(root, filePath);

    cardOfferCount_ = reader.optionalInt("card_offers", cardOfferCount_);
    relicOfferCount_ = reader.optionalInt("relic_offers", relicOfferCount_);
    consumableOfferCount_ = reader.optionalInt("consumable_offers", consumableOfferCount_);
    cardRemovalPrice_ = reader.optionalInt("card_removal_price", cardRemovalPrice_);
    minimumCardPrice_ = reader.optionalInt("minimum_card_price", minimumCardPrice_);
    minimumConsumablePrice_ = reader.optionalInt("minimum_consumable_price", minimumConsumablePrice_);

    requireNonNegative(filePath, "card_offers", cardOfferCount_);
    requireNonNegative(filePath, "relic_offers", relicOfferCount_);
    requireNonNegative(filePath, "consumable_offers", consumableOfferCount_);
    requireNonNegative(filePath, "card_removal_price", cardRemovalPrice_);
    requireNonNegative(filePath, "minimum_card_price", minimumCardPrice_);
    requireNonNegative(filePath, "minimum_consumable_price", minimumConsumablePrice_);

    const Json& relicPrices = reader.optionalObject("relic_prices");
    if (!relicPrices.empty()) {
        const JsonReader prices(relicPrices, filePath);
        commonRelicPrice_ = prices.optionalInt("common", commonRelicPrice_);
        uncommonRelicPrice_ = prices.optionalInt("uncommon", uncommonRelicPrice_);
        rareRelicPrice_ = prices.optionalInt("rare", rareRelicPrice_);
        bossRelicPrice_ = prices.optionalInt("boss", bossRelicPrice_);
        specialRelicPrice_ = prices.optionalInt("special", specialRelicPrice_);

        requireNonNegative(filePath, "relic_prices.common", commonRelicPrice_);
        requireNonNegative(filePath, "relic_prices.uncommon", uncommonRelicPrice_);
        requireNonNegative(filePath, "relic_prices.rare", rareRelicPrice_);
        requireNonNegative(filePath, "relic_prices.boss", bossRelicPrice_);
        requireNonNegative(filePath, "relic_prices.special", specialRelicPrice_);
    }
}

int ShopTuning::cardOfferCount() const {
    return cardOfferCount_;
}

int ShopTuning::relicOfferCount() const {
    return relicOfferCount_;
}

int ShopTuning::consumableOfferCount() const {
    return consumableOfferCount_;
}

int ShopTuning::cardRemovalPrice() const {
    return cardRemovalPrice_;
}

int ShopTuning::minimumCardPrice() const {
    return minimumCardPrice_;
}

int ShopTuning::minimumConsumablePrice() const {
    return minimumConsumablePrice_;
}

int ShopTuning::relicPrice(const RelicRarity rarity) const {
    switch (rarity) {
        case RelicRarity::Starter:
        case RelicRarity::Common:
            return commonRelicPrice_;
        case RelicRarity::Uncommon:
            return uncommonRelicPrice_;
        case RelicRarity::Rare:
            return rareRelicPrice_;
        case RelicRarity::Boss:
            return bossRelicPrice_;
        case RelicRarity::Special:
            return specialRelicPrice_;
    }

    throw std::runtime_error("Unknown RelicRarity in ShopTuning::relicPrice");
}
