#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstanceId.hpp"
#include "data/CardDatabase.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "shop/ShopState.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>
#include <optional>
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
    Rectangle cardOffersAreaBounds() const;
    Rectangle otherOffersAreaBounds() const;
    Rectangle cardOfferVisualBounds(Rectangle cell) const;
    Rectangle leaveButtonBounds() const;
    Rectangle removeModeBounds() const;
    Rectangle removeCardBounds(std::size_t visibleIndex) const;
    Rectangle removeCancelButtonBounds(Rectangle modal) const;
    Rectangle purchaseConfirmationBounds() const;
    Rectangle purchaseConfirmButtonBounds(Rectangle modal) const;
    Rectangle purchaseCancelButtonBounds(Rectangle modal) const;
    Rectangle hoverDescriptionBounds(Vector2 mouse, float height) const;

    void updateShop(Vector2 mouse);
    void updatePurchaseConfirmation(Vector2 mouse);
    void updateRemoveMode(Vector2 mouse);
    void clampRemoveScrollOffset();

    void renderOffers() const;
    void renderCardOffer(const ShopOffer& offer, std::size_t offerIndex) const;
    void renderTextOffer(const ShopOffer& offer, std::size_t offerIndex) const;
    void renderHoverDescription() const;
    void renderPurchaseConfirmation() const;
    void renderRemoveMode() const;

    bool canBuy(const ShopOffer& offer) const;
    void purchaseOfferAtIndex(std::size_t offerIndex);
    std::string offerName(const ShopOffer& offer) const;
    std::string offerDescription(const ShopOffer& offer) const;
    std::string offerKind(const ShopOffer& offer) const;
    std::string offerStatus(const ShopOffer& offer) const;
    Color offerStatusColor(const ShopOffer& offer) const;
    std::size_t cardOfferOrdinal(std::size_t offerIndex) const;
    std::size_t textOfferOrdinal(std::size_t offerIndex) const;
    std::size_t visibleRemoveCardCount() const;
    bool isDeckIndexUpgraded(std::size_t deckIndex) const;
    CardViewModel cardViewModel(CardId cardId, CardInstanceId instanceId, bool upgraded, bool playable, bool selected) const;
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
    std::optional<std::size_t> pendingPurchaseIndex_;
    std::size_t removeScrollOffset_ = 0;
};
