#pragma once

#include "shop/ShopOffer.hpp"

#include <vector>

struct ShopState {
    std::vector<ShopOffer> offers;
    int cardRemovalPrice = 75;
    bool cardRemovalUsed = false;
};
