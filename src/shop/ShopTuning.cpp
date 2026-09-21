#include "ShopTuning.hpp"

#include "data/JsonLoader.hpp"
#include "data/JsonReader.hpp"
#include "shop/ShopEconomy.hpp"

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
    activeItemOfferChancePercent_ = reader.optionalInt("active_item_offer_chance_percent", activeItemOfferChancePercent_);
    merchantRestCardOfferCount_ = reader.optionalInt("merchant_rest_card_offers", merchantRestCardOfferCount_);
    merchantRestMaxCardPurchases_ = reader.optionalInt("merchant_rest_max_card_purchases", merchantRestMaxCardPurchases_);
    merchantRestCardPriceMultiplier_ = reader.optionalDouble("merchant_rest_card_price_multiplier", merchantRestCardPriceMultiplier_);
    cardPriceGrowthPercentPerFloor_ = reader.optionalInt("card_price_growth_percent_per_floor", cardPriceGrowthPercentPerFloor_);
    relicPriceGrowthPercentPerFloor_ = reader.optionalInt("relic_price_growth_percent_per_floor", relicPriceGrowthPercentPerFloor_);
    consumablePriceGrowthPercentPerFloor_ = reader.optionalInt("consumable_price_growth_percent_per_floor", consumablePriceGrowthPercentPerFloor_);
    cardRemovalPricePerFloor_ = reader.optionalInt("card_removal_price_per_floor", cardRemovalPricePerFloor_);
    cardRemovalPricePerUse_ = reader.optionalInt("card_removal_price_per_use", cardRemovalPricePerUse_);
    affordableCardOfferCount_ = reader.optionalInt("affordable_card_offers", affordableCardOfferCount_);
    affordableCardPriceCapPercent_ = reader.optionalInt("affordable_card_price_cap_percent", affordableCardPriceCapPercent_);

    requireNonNegative(filePath, "card_offers", cardOfferCount_);
    requireNonNegative(filePath, "relic_offers", relicOfferCount_);
    requireNonNegative(filePath, "consumable_offers", consumableOfferCount_);
    requireNonNegative(filePath, "card_removal_price", cardRemovalPrice_);
    requireNonNegative(filePath, "minimum_card_price", minimumCardPrice_);
    requireNonNegative(filePath, "minimum_consumable_price", minimumConsumablePrice_);
    if (activeItemOfferChancePercent_ < 0 || activeItemOfferChancePercent_ > 100) {
        throw std::runtime_error(filePath.string() + ": 'active_item_offer_chance_percent' must be between 0 and 100");
    }
    requireNonNegative(filePath, "merchant_rest_card_offers", merchantRestCardOfferCount_);
    requireNonNegative(filePath, "merchant_rest_max_card_purchases", merchantRestMaxCardPurchases_);
    requireNonNegative(filePath, "card_price_growth_percent_per_floor", cardPriceGrowthPercentPerFloor_);
    requireNonNegative(filePath, "relic_price_growth_percent_per_floor", relicPriceGrowthPercentPerFloor_);
    requireNonNegative(filePath, "consumable_price_growth_percent_per_floor", consumablePriceGrowthPercentPerFloor_);
    requireNonNegative(filePath, "card_removal_price_per_floor", cardRemovalPricePerFloor_);
    requireNonNegative(filePath, "card_removal_price_per_use", cardRemovalPricePerUse_);
    requireNonNegative(filePath, "affordable_card_offers", affordableCardOfferCount_);
    if (affordableCardPriceCapPercent_ < 1 || affordableCardPriceCapPercent_ > 100) {
        throw std::runtime_error(filePath.string() + ": 'affordable_card_price_cap_percent' must be between 1 and 100");
    }
    if (merchantRestCardPriceMultiplier_ < 0.0) {
        throw std::runtime_error(filePath.string() + ": 'merchant_rest_card_price_multiplier' must not be negative");
    }

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

int ShopTuning::cardRemovalPrice(const int floorIndex, const int previousRemovals) const {
    return ShopEconomy::cardRemovalPrice(
        cardRemovalPrice_,
        floorIndex,
        previousRemovals,
        cardRemovalPricePerFloor_,
        cardRemovalPricePerUse_
    );
}

int ShopTuning::cardPrice(const int basePrice, const int floorIndex) const {
    return std::max(
        minimumCardPrice_,
        ShopEconomy::scaledPrice(basePrice, floorIndex, cardPriceGrowthPercentPerFloor_)
    );
}

int ShopTuning::consumablePrice(const int basePrice, const int floorIndex) const {
    return std::max(
        minimumConsumablePrice_,
        ShopEconomy::scaledPrice(basePrice, floorIndex, consumablePriceGrowthPercentPerFloor_)
    );
}

int ShopTuning::minimumCardPrice() const {
    return minimumCardPrice_;
}

int ShopTuning::minimumConsumablePrice() const {
    return minimumConsumablePrice_;
}

int ShopTuning::activeItemOfferChancePercent() const {
    return activeItemOfferChancePercent_;
}

int ShopTuning::merchantRestCardOfferCount() const {
    return merchantRestCardOfferCount_;
}

int ShopTuning::merchantRestMaxCardPurchases() const {
    return merchantRestMaxCardPurchases_;
}

double ShopTuning::merchantRestCardPriceMultiplier() const {
    return merchantRestCardPriceMultiplier_;
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

int ShopTuning::relicPrice(const RelicRarity rarity, const int floorIndex) const {
    return ShopEconomy::scaledPrice(
        relicPrice(rarity),
        floorIndex,
        relicPriceGrowthPercentPerFloor_
    );
}

int ShopTuning::affordableCardOfferCount() const {
    return affordableCardOfferCount_;
}

int ShopTuning::affordableCardPriceCapPercent() const {
    return affordableCardPriceCapPercent_;
}
