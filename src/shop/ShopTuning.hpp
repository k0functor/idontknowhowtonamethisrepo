#pragma once

#include "consumables/ConsumableRarity.hpp"
#include "relics/RelicRarity.hpp"

#include <filesystem>

class ShopTuning {
public:
    void loadFromFile(const std::filesystem::path& filePath);

    int cardOfferCount() const;
    int relicOfferCount() const;
    int consumableOfferCount() const;
    int cardRemovalPrice() const;
    int minimumCardPrice() const;
    int minimumConsumablePrice() const;
    int relicPrice(RelicRarity rarity) const;

private:
    int cardOfferCount_ = 3;
    int relicOfferCount_ = 2;
    int consumableOfferCount_ = 2;
    int cardRemovalPrice_ = 75;
    int minimumCardPrice_ = 20;
    int minimumConsumablePrice_ = 25;
    int commonRelicPrice_ = 150;
    int uncommonRelicPrice_ = 175;
    int rareRelicPrice_ = 220;
    int bossRelicPrice_ = 300;
    int specialRelicPrice_ = 999;
};
