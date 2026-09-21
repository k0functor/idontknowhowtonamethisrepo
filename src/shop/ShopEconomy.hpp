#pragma once

class ShopEconomy {
public:
    static int scaledPrice(int basePrice, int floorIndex, int growthPercentPerFloor);
    static int cardRemovalPrice(
        int basePrice,
        int floorIndex,
        int previousRemovals,
        int floorIncrease,
        int repeatIncrease
    );
    static int affordableCardPriceCap(int gold, int capPercent, int minimumCardPrice);
};
