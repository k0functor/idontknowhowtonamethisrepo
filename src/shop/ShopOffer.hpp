#pragma once

#include "cards/CardId.hpp"

#include <string>

#include <raylib.h>

enum class ShopOfferType {
    Card,
    Relic,
    Consumable,
    CardRemoval
};

struct ShopOffer {
    ShopOfferType type = ShopOfferType::Card;
    std::string contentId;
    int price = 0;
    bool purchased = false;
};

struct ShopPurchase {
    ShopOfferType type = ShopOfferType::Card;
    std::string contentId;
    CardId cardId;
    int price = 0;
};
