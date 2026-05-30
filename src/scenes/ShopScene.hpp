#pragma once

#include "cards/CardId.hpp"
#include "data/CardDatabase.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "shop/ShopState.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>
#include <string>

#include <raylib.h>

class ShopScene final : public Scene {
public:
    ShopScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const RunState& runState,
        ShopState shopState,
        std::function<bool(const ShopPurchase&)> onPurchase,
        std::function<void(const ShopState&)> onShopStateChanged,
        std::function<void()> onLeave
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle panelBounds() const;
    Rectangle offerBounds(std::size_t index) const;
    Rectangle leaveButtonBounds() const;
    Rectangle removeModeBounds() const;
    Rectangle removeCardBounds(std::size_t visibleIndex) const;
    Rectangle removeCancelButtonBounds(Rectangle modal) const;

    void updateShop(Vector2 mouse);
    void updateRemoveMode(Vector2 mouse);
    void clampRemoveScrollOffset();

    void renderOffers() const;
    void renderRemoveMode() const;

    bool canBuy(const ShopOffer& offer) const;
    std::string offerName(const ShopOffer& offer) const;
    std::string offerDescription(const ShopOffer& offer) const;
    std::string offerKind(const ShopOffer& offer) const;
    std::string offerStatus(const ShopOffer& offer) const;
    Color offerStatusColor(const ShopOffer& offer) const;
    std::size_t visibleRemoveCardCount() const;
    std::string cardName(const CardId& cardId) const;
    std::string cardDescription(const CardId& cardId) const;

    std::string priceText(int price) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    const ConsumableDatabase& consumables_;
    const RunState& runState_;
    ShopState shopState_;
    std::function<bool(const ShopPurchase&)> onPurchase_;
    std::function<void(const ShopState&)> onShopStateChanged_;
    std::function<void()> onLeave_;
    bool removeMode_ = false;
    std::size_t removeScrollOffset_ = 0;
};
