#pragma once

#include "active_items/ActiveItemDatabase.hpp"
#include "actors/PlayerActorDatabase.hpp"
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
        const ActiveItemDatabase& activeItems,
        const PlayerActorDatabase& actors,
        const RunState& runState,
        ShopState shopState,
        std::function<bool(const ShopPurchase&)> onPurchase,
        std::function<void(const ShopState&)> onShopStateChanged,
        std::function<bool(ShopState&)> onReroll,
        std::function<bool(const CardId&)> onCopyCard,
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
    std::size_t cardOfferColumnCount() const;
    float cardOfferCellHeight() const;
    float merchantRestCardScale() const;
    float merchantRestCardGap(float cellWidth, std::size_t cardCount) const;
    Rectangle leaveButtonBounds() const;
    Rectangle removeModeBounds() const;
    Rectangle removeCardBounds(std::size_t visibleIndex) const;
    Rectangle removeCancelButtonBounds(Rectangle modal) const;
    Rectangle purchaseConfirmationBounds() const;
    Rectangle purchaseConfirmButtonBounds(Rectangle modal) const;
    Rectangle purchaseCancelButtonBounds(Rectangle modal) const;
    Rectangle relicOwnerModalBounds() const;
    Rectangle relicOwnerOptionBounds(std::size_t index) const;
    Rectangle relicOwnerCancelButtonBounds() const;

    void moveRelicOwnerSelection(int delta);
    Rectangle hoverDescriptionBounds(Vector2 mouse, float height) const;

    void updateShop(Vector2 mouse);
    void updatePurchaseConfirmation(Vector2 mouse);
    void updateRelicOwnerChoice(Vector2 mouse);
    void updateRemoveMode(Vector2 mouse);
    void clampRemoveScrollOffset();

    void renderOffers() const;
    void renderCardOffer(const ShopOffer& offer, std::size_t offerIndex) const;
    void renderTextOffer(const ShopOffer& offer, std::size_t offerIndex) const;
    void renderHoverDescription() const;
    void renderPurchaseConfirmation() const;
    void renderRelicOwnerChoice() const;
    void renderRemoveMode() const;
    bool copyCardHintVisible(const ShopOffer& offer) const;

    bool canBuy(const ShopOffer& offer) const;
    void purchaseOfferAtIndex(std::size_t offerIndex);
    void purchaseOfferAtIndex(std::size_t offerIndex, const std::string& actorDefinitionId);
    bool hasMultipleRelicOwners() const;
    std::string actorName(const std::string& actorDefinitionId) const;
    std::string actorHealthSummary(const RunActorState& actor) const;
    std::string actorRelicSummary(const RunActorState& actor) const;
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

    std::string sceneTitle() const;
    std::string leaveButtonText() const;
    std::string priceText(int price) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    const ConsumableDatabase& consumables_;
    const ActiveItemDatabase& activeItems_;
    const PlayerActorDatabase& actors_;
    const RunState& runState_;
    ShopState shopState_;
    std::function<bool(const ShopPurchase&)> onPurchase_;
    std::function<void(const ShopState&)> onShopStateChanged_;
    std::function<bool(ShopState&)> onReroll_;
    std::function<bool(const CardId&)> onCopyCard_;
    std::function<void()> onLeave_;
    bool removeMode_ = false;
    bool relicOwnerChoiceOpen_ = false;
    std::optional<std::size_t> pendingPurchaseIndex_;
    std::optional<std::size_t> pendingRelicOwnerPurchaseIndex_;
    std::optional<std::size_t> selectedRelicOwnerIndex_;
    std::size_t removeScrollOffset_ = 0;
};
