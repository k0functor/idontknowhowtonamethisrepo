#include "shop/ShopEconomy.hpp"

#include <algorithm>
#include <cmath>

int ShopEconomy::scaledPrice(
    const int basePrice,
    const int floorIndex,
    const int growthPercentPerFloor
) {
    const int safeBase = std::max(0, basePrice);
    const int completedFloors = std::max(0, floorIndex - 1);
    const double multiplier = 1.0 + static_cast<double>(completedFloors * std::max(0, growthPercentPerFloor)) / 100.0;
    return static_cast<int>(std::lround(static_cast<double>(safeBase) * multiplier));
}

int ShopEconomy::cardRemovalPrice(
    const int basePrice,
    const int floorIndex,
    const int previousRemovals,
    const int floorIncrease,
    const int repeatIncrease
) {
    return std::max(0, basePrice) +
        std::max(0, floorIndex - 1) * std::max(0, floorIncrease) +
        std::max(0, previousRemovals) * std::max(0, repeatIncrease);
}

int ShopEconomy::affordableCardPriceCap(
    const int gold,
    const int capPercent,
    const int minimumCardPrice
) {
    if (gold < minimumCardPrice) {
        return gold;
    }

    const int proportionalCap = static_cast<int>(
        std::floor(static_cast<double>(gold) * static_cast<double>(std::clamp(capPercent, 1, 100)) / 100.0)
    );
    return std::clamp(proportionalCap, minimumCardPrice, gold);
}
