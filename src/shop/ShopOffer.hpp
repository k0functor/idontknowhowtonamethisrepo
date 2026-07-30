#pragma once

#include "cards/CardId.hpp"

#include <cstddef>
#include <string>

enum class ShopOfferType {
    Card,
    Relic,
    Consumable,
    ActiveItem,
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
    std::string actorDefinitionId;
    std::size_t deckIndex = 0;
    bool hasDeckIndex = false;
    int price = 0;
};
