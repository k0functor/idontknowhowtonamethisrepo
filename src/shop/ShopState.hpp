#pragma once

#include "shop/ShopOffer.hpp"

#include <algorithm>
#include <vector>

enum class ShopStateMode {
    Shop,
    MerchantRest
};

struct ShopState {
    ShopStateMode mode = ShopStateMode::Shop;
    std::vector<ShopOffer> offers;
    int cardRemovalPrice = 75;
    bool cardRemovalUsed = false;
    int maxCardPurchases = 0;
    int cardPurchasesMade = 0;
    bool merchantRestCardShopOpen = false;

    bool isMerchantRest() const {
        return mode == ShopStateMode::MerchantRest;
    }

    int cardPurchasesRemaining() const {
        if (maxCardPurchases <= 0) {
            return 0;
        }

        return std::max(0, maxCardPurchases - cardPurchasesMade);
    }
};
