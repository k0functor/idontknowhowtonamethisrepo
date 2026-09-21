#include "ShopScene.hpp"
#include "ShopLayout.hpp"
#include "ui/VirtualViewport.hpp"

#include "active_items/ActiveItemSystem.hpp"
#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/ActiveItemComparisonView.hpp"
#include "ui/BasicUi.hpp"
#include "ui/CardViewModelFactory.hpp"
#include "ui/CardVisualInstance.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

ShopScene::ShopScene(
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
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      relics_(relics),
      consumables_(consumables),
      activeItems_(activeItems),
      actors_(actors),
      runState_(runState),
      shopState_(std::move(shopState)),
      onPurchase_(std::move(onPurchase)),
      onShopStateChanged_(std::move(onShopStateChanged)),
      onReroll_(std::move(onReroll)),
      onCopyCard_(std::move(onCopyCard)),
      onLeave_(std::move(onLeave)) {}

void ShopScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (removeMode_) {
        updateRemoveMode(mouse);
        return;
    }

    if (relicOwnerChoiceOpen_) {
        updateRelicOwnerChoice(mouse);
        return;
    }

    if (pendingPurchaseIndex_.has_value()) {
        updatePurchaseConfirmation(mouse);
        return;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        if (!runState_.activeItem.empty()) {
            const ActiveItemId itemId(runState_.activeItem.itemId);
            if (activeItems_.contains(itemId) &&
                ActiveItemSystem::hasEffect(activeItems_.get(itemId), ActiveItemEffectType::CopyCard) &&
                onCopyCard_) {
                (void)onCopyCard_(CardId{});
                return;
            }
        }
        if (onReroll_) {
            (void)onReroll_(shopState_);
        }
        return;
    }

    updateShop(mouse);
}

void ShopScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawCenteredText(
        font_,
        sceneTitle(),
        Rectangle{0.f, 26.f, static_cast<float>(VirtualViewport::width()), 54.f},
        40.f,
        Color{244, 233, 188, 255}
    );

    BasicUi::drawCenteredText(
        font_,
        localization_.format(TextId("shop.gold"), {{"gold", std::to_string(runState_.gold)}}),
        Rectangle{0.f, 82.f, static_cast<float>(VirtualViewport::width()), 34.f},
        23.f,
        Color{219, 205, 130, 255}
    );

    const Rectangle panel = ShopLayout::panelBounds();
    DrawRectangleRounded(panel, 0.04f, 14, Color{29, 31, 41, 250});
    DrawRectangleRoundedLinesEx(panel, 0.04f, 14, 2.f, Color{111, 122, 150, 255});

    renderOffers();

    BasicUi::drawButton(font_, ShopLayout::leaveButtonBounds(), leaveButtonText(), mouse);

    if (!removeMode_ && !pendingPurchaseIndex_.has_value() && !relicOwnerChoiceOpen_) {
        renderHoverDescription();
    }

    if (pendingPurchaseIndex_.has_value()) {
        renderPurchaseConfirmation();
    }

    if (relicOwnerChoiceOpen_) {
        renderRelicOwnerChoice();
    }

    if (removeMode_) {
        renderRemoveMode();
    }
}

void ShopScene::moveRelicOwnerSelection(const int delta) {
    if (runState_.actorStates.empty()) {
        selectedRelicOwnerIndex_.reset();
        return;
    }

    const int count = static_cast<int>(runState_.actorStates.size());
    int index = selectedRelicOwnerIndex_.has_value() ? static_cast<int>(*selectedRelicOwnerIndex_) : 0;
    index = (index + delta + count) % count;
    selectedRelicOwnerIndex_ = static_cast<std::size_t>(index);
}

void ShopScene::updateShop(const Vector2 mouse) {
    if (BasicUi::contains(ShopLayout::leaveButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onLeave_();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < shopState_.offers.size(); ++i) {
        ShopOffer& offer = shopState_.offers[i];
        if (offer.purchased || !BasicUi::contains(ShopLayout::offerBounds(shopState_, i), mouse) || !canBuy(offer)) {
            continue;
        }

        if (offer.type == ShopOfferType::CardRemoval) {
            removeMode_ = true;
            removeScrollOffset_ = 0;
            return;
        }

        pendingPurchaseIndex_ = i;
        return;
    }
}

bool ShopScene::pendingPurchaseIsActiveItem() const {
    return pendingPurchaseIndex_.has_value() &&
        *pendingPurchaseIndex_ < shopState_.offers.size() &&
        shopState_.offers[*pendingPurchaseIndex_].type == ShopOfferType::ActiveItem;
}

void ShopScene::updatePurchaseConfirmation(const Vector2 mouse) {
    if (!pendingPurchaseIndex_.has_value() || *pendingPurchaseIndex_ >= shopState_.offers.size()) {
        pendingPurchaseIndex_.reset();
        return;
    }

    const ShopOffer& offer = shopState_.offers[*pendingPurchaseIndex_];
    if (offer.purchased || !canBuy(offer)) {
        pendingPurchaseIndex_.reset();
        return;
    }

    const Rectangle modal = ShopLayout::purchaseConfirmationBounds(pendingPurchaseIsActiveItem());
    if (IsKeyPressed(KEY_SPACE) && offer.type == ShopOfferType::Card && onCopyCard_) {
        (void)onCopyCard_(CardId(offer.contentId));
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(ShopLayout::purchaseCancelButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        pendingPurchaseIndex_.reset();
        return;
    }

    if (BasicUi::contains(ShopLayout::purchaseConfirmButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (offer.type == ShopOfferType::Relic && hasMultipleRelicOwners()) {
            pendingRelicOwnerPurchaseIndex_ = *pendingPurchaseIndex_;
            selectedRelicOwnerIndex_ = runState_.actorStates.empty() ? std::optional<std::size_t>{} : std::optional<std::size_t>{0u};
            pendingPurchaseIndex_.reset();
            relicOwnerChoiceOpen_ = true;
            return;
        }

        purchaseOfferAtIndex(*pendingPurchaseIndex_);
        pendingPurchaseIndex_.reset();
    }
}

void ShopScene::updateRelicOwnerChoice(const Vector2 mouse) {
    if (!pendingRelicOwnerPurchaseIndex_.has_value() || *pendingRelicOwnerPurchaseIndex_ >= shopState_.offers.size()) {
        pendingRelicOwnerPurchaseIndex_.reset();
        selectedRelicOwnerIndex_.reset();
        relicOwnerChoiceOpen_ = false;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        pendingRelicOwnerPurchaseIndex_.reset();
        selectedRelicOwnerIndex_.reset();
        relicOwnerChoiceOpen_ = false;
        return;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        moveRelicOwnerSelection(-1);
        return;
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        moveRelicOwnerSelection(1);
        return;
    }

    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) && selectedRelicOwnerIndex_.has_value()) {
        purchaseOfferAtIndex(*pendingRelicOwnerPurchaseIndex_, runState_.actorStates[*selectedRelicOwnerIndex_].definitionId);
        pendingRelicOwnerPurchaseIndex_.reset();
        selectedRelicOwnerIndex_.reset();
        relicOwnerChoiceOpen_ = false;
        return;
    }

    const Rectangle modal = ShopLayout::relicOwnerModalBounds(runState_.actorStates.size());
    for (std::size_t i = 0; i < runState_.actorStates.size(); ++i) {
        if (BasicUi::contains(ShopLayout::relicOwnerOptionBounds(modal, i), mouse)) {
            selectedRelicOwnerIndex_ = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                purchaseOfferAtIndex(*pendingRelicOwnerPurchaseIndex_, runState_.actorStates[i].definitionId);
                pendingRelicOwnerPurchaseIndex_.reset();
                selectedRelicOwnerIndex_.reset();
                relicOwnerChoiceOpen_ = false;
            }
            return;
        }
    }

    if (BasicUi::contains(ShopLayout::relicOwnerCancelButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        pendingRelicOwnerPurchaseIndex_.reset();
        selectedRelicOwnerIndex_.reset();
        relicOwnerChoiceOpen_ = false;
    }
}

void ShopScene::updateRemoveMode(const Vector2 mouse) {
    const Rectangle modal = ShopLayout::removeModeBounds();

    const float wheel = GetMouseWheelMove();
    if (wheel > 0.f && removeScrollOffset_ > 0u) {
        --removeScrollOffset_;
    } else if (wheel < 0.f && removeScrollOffset_ + ShopLayout::visibleRemoveCardCount() < runState_.deckCardIds.size()) {
        ++removeScrollOffset_;
    }

    if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && removeScrollOffset_ > 0u) {
        --removeScrollOffset_;
    }
    if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) && removeScrollOffset_ + ShopLayout::visibleRemoveCardCount() < runState_.deckCardIds.size()) {
        ++removeScrollOffset_;
    }

    clampRemoveScrollOffset();

    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(ShopLayout::removeCancelButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        removeMode_ = false;
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    const std::size_t visibleCount = ShopLayout::visibleRemoveCardCount();
    for (std::size_t visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
        const std::size_t deckIndex = removeScrollOffset_ + visibleIndex;
        if (deckIndex >= runState_.deckCardIds.size() || !BasicUi::contains(ShopLayout::removeCardBounds(visibleIndex), mouse)) {
            continue;
        }

        ShopPurchase purchase;
        purchase.type = ShopOfferType::CardRemoval;
        purchase.cardId = runState_.deckCardIds[deckIndex];
        purchase.deckIndex = deckIndex;
        purchase.hasDeckIndex = true;
        purchase.price = shopState_.cardRemovalPrice;

        if (onPurchase_(purchase)) {
            shopState_.cardRemovalUsed = true;
            const auto iterator = std::find_if(
                shopState_.offers.begin(),
                shopState_.offers.end(),
                [](const ShopOffer& offer) { return offer.type == ShopOfferType::CardRemoval; }
            );
            if (iterator != shopState_.offers.end()) {
                shopState_.offers.erase(iterator);
            }
            if (onShopStateChanged_) {
                onShopStateChanged_(shopState_);
            }
            removeMode_ = false;
        }
        return;
    }
}

void ShopScene::clampRemoveScrollOffset() {
    const std::size_t visibleCount = ShopLayout::visibleRemoveCardCount();
    if (runState_.deckCardIds.size() <= visibleCount) {
        removeScrollOffset_ = 0;
        return;
    }

    const std::size_t maximumOffset = runState_.deckCardIds.size() - visibleCount;
    removeScrollOffset_ = std::min(removeScrollOffset_, maximumOffset);
}

void ShopScene::renderOffers() const {
    for (std::size_t i = 0; i < shopState_.offers.size(); ++i) {
        const ShopOffer& offer = shopState_.offers[i];
        if (offer.purchased) {
            continue;
        }

        if (offer.type == ShopOfferType::Card) {
            renderCardOffer(offer, i);
        } else {
            renderTextOffer(offer, i);
        }
    }
}

void ShopScene::renderCardOffer(const ShopOffer& offer, const std::size_t offerIndex) const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle bounds = ShopLayout::offerBounds(shopState_, offerIndex);
    const bool enabled = canBuy(offer);
    const bool hovered = enabled && BasicUi::contains(bounds, mouse);

    if (!shopState_.isMerchantRest()) {
        const Color fill = !enabled ? Color{33, 35, 43, 255} : (hovered ? Color{53, 58, 76, 255} : Color{39, 42, 55, 255});
        DrawRectangleRounded(bounds, 0.06f, 10, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.06f, 10, 2.f, hovered ? Color{238, 196, 86, 255} : Color{108, 118, 145, 255});
    }

    const Rectangle visualBounds = ShopLayout::cardOfferVisualBounds(shopState_, bounds);
    const CardId cardId(offer.contentId);
    if (cards_.contains(cardId)) {
        CardViewModel model = cardViewModel(
            cardId,
            CardInstanceId{static_cast<std::uint64_t>(offerIndex + 1u)},
            false,
            enabled,
            hovered
        );
        const CardTransform transform = shopState_.isMerchantRest()
            ? CardVisualInstance::transformForBounds(visualBounds, static_cast<int>(offerIndex), 1.0f)
            : CardVisualInstance::transformForStandardSlot(visualBounds, static_cast<int>(offerIndex));
        CardVisualInstance::renderStatic(model, font_.available() ? &font_.font() : nullptr, transform);
    } else {
        BasicUi::drawCenteredText(font_, offer.contentId, visualBounds, 18.f, Color{230, 230, 235, 255});
    }

    const Rectangle priceBounds = shopState_.isMerchantRest()
        ? Rectangle{bounds.x, visualBounds.y + visualBounds.height + 8.f, bounds.width, 28.f}
        : Rectangle{bounds.x + 16.f, bounds.y + bounds.height - 56.f, bounds.width - 32.f, 28.f};

    if (shopState_.isMerchantRest()) {
        BasicUi::drawCenteredTextFitted(
            font_,
            priceText(offer.price),
            priceBounds,
            20.f,
            14.f,
            enabled ? Color{236, 214, 126, 255} : Color{130, 125, 96, 255}
        );
    } else {
        BasicUi::drawText(
            font_,
            priceText(offer.price),
            Vector2{priceBounds.x, priceBounds.y},
            20.f,
            enabled ? Color{236, 214, 126, 255} : Color{130, 125, 96, 255}
        );
    }

    if (!enabled) {
        const Rectangle statusBounds = shopState_.isMerchantRest()
            ? Rectangle{bounds.x, priceBounds.y + 28.f, bounds.width, 24.f}
            : Rectangle{bounds.x + 16.f, bounds.y + bounds.height - 28.f, bounds.width - 32.f, 20.f};
        if (shopState_.isMerchantRest()) {
            BasicUi::drawCenteredTextFitted(
                font_,
                offerStatus(offer),
                statusBounds,
                14.f,
                11.f,
                offerStatusColor(offer)
            );
        } else {
            BasicUi::drawTextFitted(
                font_,
                offerStatus(offer),
                Vector2{statusBounds.x, statusBounds.y},
                statusBounds.width,
                15.f,
                12.f,
                offerStatusColor(offer)
            );
        }
    }
}

void ShopScene::renderTextOffer(const ShopOffer& offer, const std::size_t offerIndex) const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle bounds = ShopLayout::offerBounds(shopState_, offerIndex);
    const bool enabled = canBuy(offer);
    const bool hovered = enabled && BasicUi::contains(bounds, mouse);
    const Color fill = !enabled ? Color{34, 36, 44, 255} : (hovered ? Color{55, 60, 78, 255} : Color{41, 44, 58, 255});

    DrawRectangleRounded(bounds, 0.08f, 10, fill);
    DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, 2.f, hovered ? Color{238, 196, 86, 255} : Color{108, 118, 145, 255});

    BasicUi::drawText(font_, offerKind(offer), Vector2{bounds.x + 18.f, bounds.y + 14.f}, 18.f, Color{180, 188, 210, 255});
    BasicUi::drawText(font_, priceText(offer.price), Vector2{bounds.x + bounds.width - 142.f, bounds.y + 16.f}, 18.f, enabled ? Color{236, 214, 126, 255} : Color{130, 125, 96, 255});
    BasicUi::drawTextFitted(
        font_,
        offerName(offer),
        Vector2{bounds.x + 18.f, bounds.y + 50.f},
        bounds.width - 36.f,
        24.f,
        16.f,
        enabled ? Color{244, 244, 250, 255} : Color{135, 139, 154, 255}
    );

    if (!enabled) {
        BasicUi::drawTextFitted(
            font_,
            offerStatus(offer),
            Vector2{bounds.x + 18.f, bounds.y + bounds.height - 28.f},
            bounds.width - 36.f,
            15.f,
            12.f,
            offerStatusColor(offer)
        );
    }
}

void ShopScene::renderHoverDescription() const {
    const Vector2 mouse = GetMousePosition();

    for (std::size_t i = 0; i < shopState_.offers.size(); ++i) {
        const ShopOffer& offer = shopState_.offers[i];
        if (offer.purchased || (offer.type != ShopOfferType::Relic && offer.type != ShopOfferType::Consumable && offer.type != ShopOfferType::ActiveItem)) {
            continue;
        }
        if (!BasicUi::contains(ShopLayout::offerBounds(shopState_, i), mouse)) {
            continue;
        }

        const std::string description = offerDescription(offer);
        const std::vector<std::string> lines = BasicUi::wrapText(font_, description, 19.f, 468.f);
        const float height = std::min(260.f, 92.f + static_cast<float>(lines.size()) * 24.f);
        const Rectangle bounds = ShopLayout::hoverDescriptionBounds(mouse, height);

        DrawRectangleRounded(bounds, 0.06f, 12, Color{24, 26, 35, 250});
        DrawRectangleRoundedLinesEx(bounds, 0.06f, 12, 2.f, Color{142, 154, 188, 255});
        BasicUi::drawText(font_, offerName(offer), Vector2{bounds.x + 22.f, bounds.y + 18.f}, 24.f, Color{244, 244, 250, 255});
        BasicUi::drawText(font_, offerKind(offer), Vector2{bounds.x + 22.f, bounds.y + 50.f}, 16.f, Color{180, 188, 210, 255});

        float y = bounds.y + 78.f;
        for (const std::string& line : lines) {
            if (y > bounds.y + bounds.height - 28.f) {
                break;
            }
            BasicUi::drawText(font_, line, Vector2{bounds.x + 22.f, y}, 19.f, Color{202, 209, 230, 255});
            y += 24.f;
        }
        return;
    }
}

void ShopScene::renderPurchaseConfirmation() const {
    if (!pendingPurchaseIndex_.has_value() || *pendingPurchaseIndex_ >= shopState_.offers.size()) {
        return;
    }

    const ShopOffer& offer = shopState_.offers[*pendingPurchaseIndex_];
    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = ShopLayout::purchaseConfirmationBounds(pendingPurchaseIsActiveItem());

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 140});
    DrawRectangleRounded(modal, 0.05f, 14, Color{27, 29, 39, 252});
    DrawRectangleRoundedLinesEx(modal, 0.05f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("shop.confirm.title")),
        Rectangle{modal.x + 32.f, modal.y + 24.f, modal.width - 64.f, 40.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    if (offer.type == ShopOfferType::ActiveItem) {
        BasicUi::drawCenteredTextFitted(
            font_,
            localization_.format(TextId("active_item.compare.shop_price"), {{"price", priceText(offer.price)}}),
            Rectangle{modal.x + 42.f, modal.y + 66.f, modal.width - 84.f, 32.f},
            20.f,
            15.f,
            Color{214, 220, 238, 255}
        );
        ActiveItemComparisonView::render(
            font_,
            localization_,
            activeItems_,
            runState_,
            offer.contentId,
            Rectangle{modal.x + 42.f, modal.y + 108.f, modal.width - 84.f, modal.height - 206.f}
        );
        BasicUi::drawButton(font_, ShopLayout::purchaseConfirmButtonBounds(modal), localization_.get(TextId("active_item.compare.buy_equip")), mouse);
        BasicUi::drawButton(font_, ShopLayout::purchaseCancelButtonBounds(modal), localization_.get(TextId("active_item.compare.keep")), mouse);
        return;
    }

    BasicUi::drawCenteredTextFitted(
        font_,
        localization_.format(
            TextId("shop.confirm.prompt"),
            {
                {"item", offerName(offer)},
                {"price", priceText(offer.price)}
            }
        ),
        Rectangle{modal.x + 42.f, modal.y + 84.f, modal.width - 84.f, 64.f},
        24.f,
        16.f,
        Color{214, 220, 238, 255}
    );

    if (copyCardHintVisible(offer)) {
        BasicUi::drawCenteredTextFitted(
            font_,
            localization_.get(TextId("active_item.hint.copy_selected")),
            Rectangle{modal.x + 42.f, modal.y + 132.f, modal.width - 84.f, 24.f},
            16.f,
            12.f,
            Color{196, 176, 235, 255}
        );
    }

    BasicUi::drawText(
        font_,
        offerKind(offer),
        Vector2{modal.x + 48.f, modal.y + 158.f},
        18.f,
        Color{170, 180, 204, 255}
    );

    BasicUi::drawButton(font_, ShopLayout::purchaseConfirmButtonBounds(modal), localization_.get(TextId("shop.confirm.buy")), mouse);
    BasicUi::drawButton(font_, ShopLayout::purchaseCancelButtonBounds(modal), localization_.get(TextId("reward.cancel")), mouse);
}

void ShopScene::renderRelicOwnerChoice() const {
    if (!pendingRelicOwnerPurchaseIndex_.has_value() || *pendingRelicOwnerPurchaseIndex_ >= shopState_.offers.size()) {
        return;
    }

    const ShopOffer& offer = shopState_.offers[*pendingRelicOwnerPurchaseIndex_];
    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = ShopLayout::relicOwnerModalBounds(runState_.actorStates.size());

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 140});
    DrawRectangleRounded(modal, 0.06f, 14, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(modal, 0.06f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("reward.relic_owner_title")),
        Rectangle{modal.x + 24.f, modal.y + 18.f, modal.width - 48.f, 36.f},
        28.f,
        Color{255, 235, 175, 255}
    );

    BasicUi::drawCenteredTextFitted(
        font_,
        localization_.format(TextId("reward.relic_owner_hint"), {{"relic", offerName(offer)}}),
        Rectangle{modal.x + 42.f, modal.y + 62.f, modal.width - 84.f, 44.f},
        18.f,
        14.f,
        Color{195, 202, 225, 255}
    );

    for (std::size_t i = 0; i < runState_.actorStates.size(); ++i) {
        const RunActorState& actor = runState_.actorStates[i];
        const Rectangle row = ShopLayout::relicOwnerOptionBounds(modal, i);
        const bool hovered = BasicUi::contains(row, mouse);
        const bool selected = selectedRelicOwnerIndex_.has_value() && *selectedRelicOwnerIndex_ == i;
        DrawRectangleRounded(row, 0.08f, 10, hovered || selected ? Color{55, 61, 80, 255} : Color{40, 43, 56, 255});
        DrawRectangleRoundedLinesEx(row, 0.08f, 10, selected ? 3.f : 2.f, selected ? Color{255, 218, 96, 255} : (hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255}));

        BasicUi::drawTextFitted(
            font_,
            actorName(actor.definitionId),
            Vector2{row.x + 18.f, row.y + 10.f},
            row.width * 0.48f,
            21.f,
            15.f,
            Color{245, 245, 250, 255}
        );
        BasicUi::drawTextFitted(
            font_,
            actorHealthSummary(actor),
            Vector2{row.x + row.width * 0.52f, row.y + 12.f},
            row.width * 0.42f,
            17.f,
            13.f,
            Color{210, 218, 238, 255}
        );
        BasicUi::drawTextFitted(
            font_,
            actorRelicSummary(actor),
            Vector2{row.x + 18.f, row.y + 44.f},
            row.width - 36.f,
            16.f,
            12.f,
            Color{176, 184, 208, 255}
        );
    }

    BasicUi::drawCenteredTextFitted(
        font_,
        localization_.get(TextId("reward.relic_owner_controls")),
        Rectangle{modal.x + 42.f, modal.y + modal.height - 100.f, modal.width - 84.f, 24.f},
        15.f,
        12.f,
        Color{150, 158, 184, 255}
    );

    BasicUi::drawButton(font_, ShopLayout::relicOwnerCancelButtonBounds(modal), localization_.get(TextId("reward.cancel")), mouse);
}

void ShopScene::renderRemoveMode() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = ShopLayout::removeModeBounds();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 145});
    DrawRectangleRounded(modal, 0.05f, 14, Color{26, 28, 38, 252});
    DrawRectangleRoundedLinesEx(modal, 0.05f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(font_, localization_.get(TextId("shop.remove_card_title")), Rectangle{modal.x + 20.f, modal.y + 22.f, modal.width - 40.f, 42.f}, 30.f, Color{255, 235, 175, 255});
    BasicUi::drawCenteredText(font_, localization_.format(TextId("shop.remove_card_description"), {{"price", std::to_string(shopState_.cardRemovalPrice)}}), Rectangle{modal.x + 30.f, modal.y + 66.f, modal.width - 60.f, 38.f}, 20.f, Color{190, 198, 220, 255});

    const std::size_t visibleCount = ShopLayout::visibleRemoveCardCount();
    for (std::size_t visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
        const std::size_t deckIndex = removeScrollOffset_ + visibleIndex;
        if (deckIndex >= runState_.deckCardIds.size()) {
            break;
        }

        const Rectangle cell = ShopLayout::removeCardBounds(visibleIndex);
        const bool hovered = BasicUi::contains(cell, mouse);
        DrawRectangleRounded(cell, 0.06f, 10, hovered ? Color{55, 60, 78, 255} : Color{40, 43, 56, 255});
        DrawRectangleRoundedLinesEx(cell, 0.06f, 10, 2.f, hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255});

        const CardId cardId = runState_.deckCardIds[deckIndex];
        if (cards_.contains(cardId)) {
            const CardViewModel model = cardViewModel(
                cardId,
                CardInstanceId{static_cast<std::uint64_t>(deckIndex + 1u)},
                isDeckIndexUpgraded(deckIndex),
                true,
                hovered
            );
            const Rectangle visualBounds{cell.x + 8.f, cell.y + 10.f, cell.width - 16.f, cell.height - 46.f};
            const CardTransform transform = CardVisualInstance::transformForStandardSlot(visualBounds, static_cast<int>(visibleIndex));
            CardVisualInstance::renderStatic(model, font_.available() ? &font_.font() : nullptr, transform);
        } else {
            BasicUi::drawCenteredText(font_, cardId.value, Rectangle{cell.x + 8.f, cell.y + 10.f, cell.width - 16.f, cell.height - 58.f}, 18.f, Color{230, 230, 235, 255});
        }

        BasicUi::drawCenteredText(
            font_,
            localization_.format(TextId("shop.deck_index"), {{"index", std::to_string(deckIndex + 1)}}),
            Rectangle{cell.x + 8.f, cell.y + cell.height - 42.f, cell.width - 16.f, 28.f},
            15.f,
            Color{164, 172, 196, 255}
        );
    }

    if (runState_.deckCardIds.size() > visibleCount) {
        BasicUi::drawCenteredText(
            font_,
            localization_.format(
                TextId("shop.remove_scroll_hint"),
                {
                    {"from", std::to_string(removeScrollOffset_ + 1)},
                    {"to", std::to_string(std::min(runState_.deckCardIds.size(), removeScrollOffset_ + visibleCount))},
                    {"total", std::to_string(runState_.deckCardIds.size())}
                }
            ),
            Rectangle{modal.x + 30.f, modal.y + modal.height - 112.f, modal.width - 60.f, 28.f},
            15.f,
            Color{174, 180, 202, 255}
        );
    }

    BasicUi::drawButton(font_, ShopLayout::removeCancelButtonBounds(modal), localization_.get(TextId("reward.cancel")), mouse);
}

void ShopScene::purchaseOfferAtIndex(const std::size_t offerIndex) {
    purchaseOfferAtIndex(offerIndex, {});
}

void ShopScene::purchaseOfferAtIndex(const std::size_t offerIndex, const std::string& actorDefinitionId) {
    if (offerIndex >= shopState_.offers.size()) {
        return;
    }

    const ShopOffer offer = shopState_.offers[offerIndex];
    if (offer.purchased || !canBuy(offer) || offer.type == ShopOfferType::CardRemoval) {
        return;
    }

    ShopPurchase purchase;
    purchase.type = offer.type;
    purchase.contentId = offer.contentId;
    purchase.price = offer.price;
    purchase.actorDefinitionId = actorDefinitionId;

    if (!onPurchase_(purchase)) {
        return;
    }

    if (shopState_.isMerchantRest() && offer.type == ShopOfferType::Card) {
        ++shopState_.cardPurchasesMade;
    }

    shopState_.offers.erase(shopState_.offers.begin() + static_cast<std::ptrdiff_t>(offerIndex));
    if (onShopStateChanged_) {
        onShopStateChanged_(shopState_);
    }
}

bool ShopScene::hasMultipleRelicOwners() const {
    return runState_.actorStates.size() > 1;
}

std::string ShopScene::actorName(const std::string& actorDefinitionId) const {
    if (actorDefinitionId.empty()) {
        return localization_.get(TextId("debug.player.name"));
    }

    const PlayerActorId id(actorDefinitionId);
    if (!actors_.contains(id)) {
        return actorDefinitionId;
    }

    return localization_.get(actors_.get(id).nameTextId);
}

std::string ShopScene::actorHealthSummary(const RunActorState& actor) const {
    return localization_.format(
        TextId("reward.relic_owner_hp"),
        {{"current", std::to_string(std::max(0, actor.currentHp))}, {"maximum", std::to_string(std::max(1, actor.maxHp))}}
    );
}

std::string ShopScene::actorRelicSummary(const RunActorState& actor) const {
    if (actor.relicIds.empty()) {
        return localization_.get(TextId("reward.relic_owner_no_relics"));
    }

    std::string names;
    const std::size_t visible = std::min<std::size_t>(2u, actor.relicIds.size());
    for (std::size_t i = 0; i < visible; ++i) {
        if (!names.empty()) {
            names += ", ";
        }
        names += offerName(ShopOffer{ShopOfferType::Relic, actor.relicIds[i], 0, false});
    }
    if (actor.relicIds.size() > visible) {
        names += localization_.format(TextId("reward.relic_owner_more_relics"), {{"count", std::to_string(actor.relicIds.size() - visible)}});
    }

    return localization_.format(
        TextId("reward.relic_owner_relics"),
        {{"count", std::to_string(actor.relicIds.size())}, {"relics", names}}
    );
}

bool ShopScene::copyCardHintVisible(const ShopOffer& offer) const {
    if (offer.type != ShopOfferType::Card || !onCopyCard_ || runState_.activeItem.empty()) {
        return false;
    }
    const ActiveItemId id(runState_.activeItem.itemId);
    return activeItems_.contains(id) &&
        ActiveItemSystem::hasEffect(activeItems_.get(id), ActiveItemEffectType::CopyCard);
}

bool ShopScene::canBuy(const ShopOffer& offer) const {
    if (offer.purchased) {
        return false;
    }

    if (runState_.gold < offer.price) {
        return false;
    }

    if (shopState_.isMerchantRest()) {
        return offer.type == ShopOfferType::Card && shopState_.cardPurchasesRemaining() > 0;
    }

    if (offer.type == ShopOfferType::Consumable && static_cast<int>(runState_.consumableIds.size()) >= runState_.maxConsumables) {
        return false;
    }

    if (offer.type == ShopOfferType::ActiveItem && offer.contentId == runState_.activeItem.itemId) {
        return false;
    }

    if (offer.type == ShopOfferType::CardRemoval && (shopState_.cardRemovalUsed || runState_.deckCardIds.empty())) {
        return false;
    }

    return true;
}

std::string ShopScene::offerName(const ShopOffer& offer) const {
    switch (offer.type) {
        case ShopOfferType::Card:
            return cardName(CardId(offer.contentId));
        case ShopOfferType::Relic:
            if (relics_.contains(RelicId(offer.contentId))) {
                return localization_.get(relics_.get(RelicId(offer.contentId)).nameTextId);
            }
            return offer.contentId;
        case ShopOfferType::Consumable:
            if (consumables_.contains(ConsumableId(offer.contentId))) {
                return localization_.get(consumables_.get(ConsumableId(offer.contentId)).nameTextId);
            }
            return offer.contentId;
        case ShopOfferType::ActiveItem:
            if (activeItems_.contains(ActiveItemId(offer.contentId))) {
                return localization_.get(activeItems_.get(ActiveItemId(offer.contentId)).nameTextId);
            }
            return offer.contentId;
        case ShopOfferType::CardRemoval:
            return localization_.get(TextId("shop.remove_card"));
    }

    return {};
}

std::string ShopScene::offerDescription(const ShopOffer& offer) const {
    switch (offer.type) {
        case ShopOfferType::Card:
            return cardDescription(CardId(offer.contentId));
        case ShopOfferType::Relic:
            if (relics_.contains(RelicId(offer.contentId))) {
                return localization_.get(relics_.get(RelicId(offer.contentId)).descriptionTextId);
            }
            return offer.contentId;
        case ShopOfferType::Consumable:
            if (consumables_.contains(ConsumableId(offer.contentId))) {
                return localization_.get(consumables_.get(ConsumableId(offer.contentId)).descriptionTextId);
            }
            return offer.contentId;
        case ShopOfferType::ActiveItem:
            if (activeItems_.contains(ActiveItemId(offer.contentId))) {
                return localization_.get(activeItems_.get(ActiveItemId(offer.contentId)).descriptionTextId);
            }
            return offer.contentId;
        case ShopOfferType::CardRemoval:
            return localization_.get(TextId("shop.remove_card_short_description"));
    }

    return {};
}

std::string ShopScene::offerKind(const ShopOffer& offer) const {
    switch (offer.type) {
        case ShopOfferType::Card:
            return localization_.get(TextId("shop.kind.card"));
        case ShopOfferType::Relic:
            return localization_.get(TextId("shop.kind.relic"));
        case ShopOfferType::Consumable:
            return localization_.get(TextId("shop.kind.consumable"));
        case ShopOfferType::ActiveItem:
            return localization_.get(TextId("shop.kind.active_item"));
        case ShopOfferType::CardRemoval:
            return localization_.get(TextId("shop.kind.service"));
    }

    return {};
}

std::string ShopScene::offerStatus(const ShopOffer& offer) const {
    if (offer.purchased) {
        return localization_.get(TextId("shop.status.sold"));
    }

    if (shopState_.isMerchantRest() && shopState_.cardPurchasesRemaining() <= 0) {
        return localization_.get(TextId("merchant_rest.status.limit_reached"));
    }

    if (offer.type == ShopOfferType::Consumable && static_cast<int>(runState_.consumableIds.size()) >= runState_.maxConsumables) {
        return localization_.get(TextId("shop.status.consumables_full"));
    }

    if (offer.type == ShopOfferType::ActiveItem && offer.contentId == runState_.activeItem.itemId) {
        return localization_.get(TextId("shop.status.active_item_equipped"));
    }

    if (offer.type == ShopOfferType::CardRemoval) {
        if (shopState_.cardRemovalUsed) {
            return localization_.get(TextId("shop.status.removal_used"));
        }
        if (runState_.deckCardIds.empty()) {
            return localization_.get(TextId("shop.status.deck_empty"));
        }
    }

    if (runState_.gold < offer.price) {
        return localization_.format(TextId("shop.status.not_enough_gold"), {{"missing", std::to_string(offer.price - runState_.gold)}});
    }

    return localization_.get(TextId("shop.status.click_to_buy"));
}

Color ShopScene::offerStatusColor(const ShopOffer& offer) const {
    if (canBuy(offer)) {
        return Color{156, 210, 158, 255};
    }

    if (offer.purchased) {
        return Color{236, 214, 126, 255};
    }

    return Color{210, 145, 135, 255};
}

bool ShopScene::isDeckIndexUpgraded(const std::size_t deckIndex) const {
    return std::find(
        runState_.upgradedDeckIndices.begin(),
        runState_.upgradedDeckIndices.end(),
        static_cast<int>(deckIndex)
    ) != runState_.upgradedDeckIndices.end();
}

CardViewModel ShopScene::cardViewModel(
    const CardId cardId,
    const CardInstanceId instanceId,
    const bool upgraded,
    const bool playable,
    const bool selected
) const {
    if (!cards_.contains(cardId)) {
        CardViewModel model;
        model.instanceId = instanceId;
        model.name = cardId.value;
        model.description = cardId.value;
        model.playable = playable;
        model.selected = selected;
        return model;
    }

    return CardViewModelFactory::buildStatic(cards_.get(cardId), localization_, instanceId, upgraded, selected, playable);
}

std::string ShopScene::cardName(const CardId& cardId) const {
    if (!cards_.contains(cardId)) {
        return cardId.value;
    }

    return localization_.get(cards_.get(cardId).nameTextId);
}

std::string ShopScene::cardDescription(const CardId& cardId) const {
    if (!cards_.contains(cardId)) {
        return cardId.value;
    }

    const CardDescriptionFormatter descriptionFormatter(localization_);
    return descriptionFormatter.formatStaticDescription(cards_.get(cardId));
}

std::string ShopScene::sceneTitle() const {
    return localization_.get(TextId(shopState_.isMerchantRest() ? "merchant_rest.title" : "shop.title"));
}


std::string ShopScene::leaveButtonText() const {
    return localization_.get(TextId(shopState_.isMerchantRest() ? "merchant_rest.leave" : "shop.leave"));
}

std::string ShopScene::priceText(const int price) const {
    return localization_.format(TextId("shop.price"), {{"price", std::to_string(price)}});
}
