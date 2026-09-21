#pragma once

#include "shop/ShopState.hpp"

#include <raylib.h>

#include <cstddef>

class ShopLayout {
public:
    static Rectangle panelBounds();
    static Rectangle cardOffersAreaBounds(bool merchantRest);
    static Rectangle otherOffersAreaBounds(bool merchantRest);
    static Rectangle offerBounds(const ShopState& shopState, std::size_t index);
    static Rectangle cardOfferVisualBounds(const ShopState& shopState, Rectangle cell);

    static Rectangle leaveButtonBounds();
    static Rectangle removeModeBounds();
    static Rectangle removeCardBounds(std::size_t visibleIndex);
    static Rectangle removeCancelButtonBounds(Rectangle modal);
    static std::size_t visibleRemoveCardCount();

    static Rectangle purchaseConfirmationBounds(bool activeItem);
    static Rectangle purchaseConfirmButtonBounds(Rectangle modal);
    static Rectangle purchaseCancelButtonBounds(Rectangle modal);

    static Rectangle relicOwnerModalBounds(std::size_t actorCount);
    static Rectangle relicOwnerOptionBounds(Rectangle modal, std::size_t index);
    static Rectangle relicOwnerCancelButtonBounds(Rectangle modal);

    static Rectangle hoverDescriptionBounds(Vector2 mouse, float height);

private:
    static std::size_t cardOfferColumnCount(const ShopState& shopState);
    static float cardOfferCellHeight(const ShopState& shopState);
    static float merchantRestCardScale(const ShopState& shopState);
    static float merchantRestCardGap(float areaWidth, float cellWidth, std::size_t cardCount);
    static std::size_t cardOfferOrdinal(const ShopState& shopState, std::size_t offerIndex);
    static std::size_t textOfferOrdinal(const ShopState& shopState, std::size_t offerIndex);
};
