#include "ShopScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "relics/RelicDefinition.hpp"
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

namespace {
constexpr float offerSpacing = 20.f;
constexpr float textOfferHeight = 122.f;
constexpr float textOfferSpacing = 16.f;
constexpr float removeCardCellHeight = 310.f;
constexpr float removeCardCellSpacing = 16.f;
constexpr std::size_t removeCardColumns = 4u;
constexpr float merchantRestDesiredCardScale = 1.36f;
constexpr float merchantRestMinimumCardScale = 0.92f;
constexpr float merchantRestCellHorizontalPadding = 12.f;
constexpr float merchantRestPriceGap = 10.f;
constexpr float merchantRestPriceHeight = 32.f;
constexpr float merchantRestStatusHeight = 24.f;
constexpr float merchantRestMaximumCardGap = 52.f;
constexpr float merchantRestMinimumCardGap = 18.f;
}

ShopScene::ShopScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const PlayerActorDatabase& actors,
    const RunState& runState,
    ShopState shopState,
    std::function<bool(const ShopPurchase&)> onPurchase,
    std::function<void(const ShopState&)> onShopStateChanged,
    std::function<void()> onLeave
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      relics_(relics),
      consumables_(consumables),
      actors_(actors),
      runState_(runState),
      shopState_(std::move(shopState)),
      onPurchase_(std::move(onPurchase)),
      onShopStateChanged_(std::move(onShopStateChanged)),
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

    const std::string subtitle = sceneSubtitle();
    if (!subtitle.empty()) {
        BasicUi::drawCenteredText(
            font_,
            subtitle,
            Rectangle{70.f, 116.f, static_cast<float>(VirtualViewport::width()) - 140.f, 30.f},
            18.f,
            Color{184, 193, 214, 255}
        );
    }

    const Rectangle panel = panelBounds();
    DrawRectangleRounded(panel, 0.04f, 14, Color{29, 31, 41, 250});
    DrawRectangleRoundedLinesEx(panel, 0.04f, 14, 2.f, Color{111, 122, 150, 255});

    renderOffers();

    BasicUi::drawButton(font_, leaveButtonBounds(), leaveButtonText(), mouse);

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

Rectangle ShopScene::panelBounds() const {
    const float width = std::min(1580.f, static_cast<float>(VirtualViewport::width()) - 80.f);
    const float height = std::min(760.f, static_cast<float>(VirtualViewport::height()) - 230.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        150.f,
        width,
        height
    };
}

Rectangle ShopScene::cardOffersAreaBounds() const {
    const Rectangle panel = panelBounds();
    if (shopState_.isMerchantRest()) {
        return Rectangle{panel.x + 34.f, panel.y + 34.f, panel.width - 68.f, panel.height - 82.f};
    }

    const float width = std::min(930.f, panel.width * 0.60f);
    return Rectangle{panel.x + 34.f, panel.y + 34.f, width, panel.height - 82.f};
}

Rectangle ShopScene::otherOffersAreaBounds() const {
    const Rectangle panel = panelBounds();
    if (shopState_.isMerchantRest()) {
        return Rectangle{panel.x + panel.width - 34.f, panel.y + 34.f, 0.f, panel.height - 82.f};
    }

    const Rectangle cards = cardOffersAreaBounds();
    const float x = cards.x + cards.width + 34.f;
    return Rectangle{x, panel.y + 34.f, std::max(420.f, panel.x + panel.width - x - 34.f), panel.height - 82.f};
}

Rectangle ShopScene::offerBounds(const std::size_t index) const {
    if (index >= shopState_.offers.size()) {
        return Rectangle{};
    }

    const ShopOffer& offer = shopState_.offers[index];
    if (offer.type == ShopOfferType::Card) {
        const Rectangle area = cardOffersAreaBounds();
        const std::size_t columns = cardOfferColumnCount();
        const std::size_t ordinal = cardOfferOrdinal(index);

        if (shopState_.isMerchantRest()) {
            const Vector2 cardSize = CardVisualInstance::size();
            const float scale = merchantRestCardScale();
            const float cellWidth = cardSize.x * scale + merchantRestCellHorizontalPadding;
            const float cellHeight = cardSize.y * scale + merchantRestPriceGap + merchantRestPriceHeight + merchantRestStatusHeight;
            const float gap = merchantRestCardGap(cellWidth, columns);
            const float totalWidth = static_cast<float>(columns) * cellWidth +
                static_cast<float>(columns - 1u) * gap;
            const float startX = area.x + (area.width - totalWidth) * 0.5f;
            const float startY = area.y + (area.height - cellHeight) * 0.5f;

            return Rectangle{
                startX + static_cast<float>(ordinal) * (cellWidth + gap),
                startY,
                cellWidth,
                cellHeight
            };
        }

        const float columnCount = static_cast<float>(std::max<std::size_t>(1u, columns));
        const float cellWidth = (area.width - offerSpacing * (columnCount - 1.f)) / columnCount;
        const float cellHeight = cardOfferCellHeight();
        const std::size_t column = ordinal % columns;
        const std::size_t row = ordinal / columns;
        return Rectangle{
            area.x + static_cast<float>(column) * (cellWidth + offerSpacing),
            area.y + static_cast<float>(row) * (cellHeight + offerSpacing),
            cellWidth,
            cellHeight
        };
    }

    const Rectangle area = otherOffersAreaBounds();
    const std::size_t ordinal = textOfferOrdinal(index);
    return Rectangle{
        area.x,
        area.y + static_cast<float>(ordinal) * (textOfferHeight + textOfferSpacing),
        area.width,
        textOfferHeight
    };
}

Rectangle ShopScene::cardOfferVisualBounds(const Rectangle cell) const {
    if (shopState_.isMerchantRest()) {
        const Vector2 cardSize = CardVisualInstance::size();
        const float scale = merchantRestCardScale();
        const float visualWidth = cardSize.x * scale;
        const float visualHeight = cardSize.y * scale;
        return Rectangle{
            cell.x + (cell.width - visualWidth) * 0.5f,
            cell.y,
            visualWidth,
            visualHeight
        };
    }

    return Rectangle{cell.x + 8.f, cell.y + 10.f, cell.width - 16.f, cell.height - 92.f};
}

std::size_t ShopScene::cardOfferColumnCount() const {
    const Rectangle area = cardOffersAreaBounds();
    const std::size_t cardCount = std::max<std::size_t>(1u, std::count_if(
        shopState_.offers.begin(),
        shopState_.offers.end(),
        [this](const ShopOffer& offer) {
            return offer.type == ShopOfferType::Card && (shopState_.isMerchantRest() || !offer.purchased);
        }
    ));

    if (shopState_.isMerchantRest()) {
        return cardCount;
    }

    return area.width < 760.f ? 2u : 3u;
}

float ShopScene::cardOfferCellHeight() const {
    const Rectangle area = cardOffersAreaBounds();
    const std::size_t columns = cardOfferColumnCount();
    const std::size_t cardCount = std::max<std::size_t>(1u, std::count_if(
        shopState_.offers.begin(),
        shopState_.offers.end(),
        [this](const ShopOffer& offer) {
            return offer.type == ShopOfferType::Card && (shopState_.isMerchantRest() || !offer.purchased);
        }
    ));
    const std::size_t rows = std::max<std::size_t>(1u, (cardCount + columns - 1u) / columns);
    const float availableHeight = area.height - offerSpacing * static_cast<float>(rows - 1u);
    const float fittedHeight = availableHeight / static_cast<float>(rows);

    if (shopState_.isMerchantRest()) {
        const Vector2 cardSize = CardVisualInstance::size();
        const float scale = merchantRestCardScale();
        return cardSize.y * scale + merchantRestPriceGap + merchantRestPriceHeight + merchantRestStatusHeight;
    }

    return std::clamp(fittedHeight, 260.f, 376.f);
}

float ShopScene::merchantRestCardScale() const {
    const Rectangle area = cardOffersAreaBounds();
    const std::size_t cardCount = std::max<std::size_t>(1u, cardOfferColumnCount());
    const Vector2 cardSize = CardVisualInstance::size();
    const float horizontalGaps = merchantRestMaximumCardGap * static_cast<float>(cardCount - 1u);
    const float horizontalPadding = merchantRestCellHorizontalPadding * static_cast<float>(cardCount);
    const float widthScale = (area.width - horizontalGaps - horizontalPadding) /
        (cardSize.x * static_cast<float>(cardCount));
    const float heightScale = (area.height - merchantRestPriceGap - merchantRestPriceHeight - merchantRestStatusHeight) /
        cardSize.y;
    const float fittedScale = std::min({merchantRestDesiredCardScale, widthScale, heightScale});
    return std::clamp(fittedScale, merchantRestMinimumCardScale, merchantRestDesiredCardScale);
}

float ShopScene::merchantRestCardGap(const float cellWidth, const std::size_t cardCount) const {
    if (cardCount <= 1u) {
        return 0.f;
    }

    const Rectangle area = cardOffersAreaBounds();
    const float remainingWidth = area.width - cellWidth * static_cast<float>(cardCount);
    const float fittedGap = remainingWidth / static_cast<float>(cardCount - 1u);
    return std::clamp(fittedGap, merchantRestMinimumCardGap, merchantRestMaximumCardGap);
}

Rectangle ShopScene::leaveButtonBounds() const {
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - 150.f,
        static_cast<float>(VirtualViewport::height()) - 82.f,
        300.f,
        50.f
    };
}

Rectangle ShopScene::removeModeBounds() const {
    const float width = std::min(1080.f, static_cast<float>(VirtualViewport::width()) - 70.f);
    const float height = std::min(660.f, static_cast<float>(VirtualViewport::height()) - 70.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle ShopScene::removeCardBounds(const std::size_t index) const {
    const Rectangle modal = removeModeBounds();
    const float gridX = modal.x + 35.f;
    const float gridY = modal.y + 124.f;
    const float gridWidth = modal.width - 70.f;
    const float cellWidth = (gridWidth - removeCardCellSpacing * static_cast<float>(removeCardColumns - 1u)) /
        static_cast<float>(removeCardColumns);
    const std::size_t column = index % removeCardColumns;
    const std::size_t row = index / removeCardColumns;

    return Rectangle{
        gridX + static_cast<float>(column) * (cellWidth + removeCardCellSpacing),
        gridY + static_cast<float>(row) * (removeCardCellHeight + removeCardCellSpacing),
        cellWidth,
        removeCardCellHeight
    };
}

Rectangle ShopScene::removeCancelButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f - 130.f, modal.y + modal.height - 66.f, 260.f, 46.f};
}

Rectangle ShopScene::purchaseConfirmationBounds() const {
    const float width = 620.f;
    const float height = 270.f;
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle ShopScene::purchaseConfirmButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 64.f, modal.y + modal.height - 76.f, 220.f, 50.f};
}

Rectangle ShopScene::purchaseCancelButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 284.f, modal.y + modal.height - 76.f, 220.f, 50.f};
}

Rectangle ShopScene::relicOwnerModalBounds() const {
    const float width = 700.f;
    const float desiredHeight = 188.f + static_cast<float>(runState_.actorStates.size()) * 92.f + 76.f;
    const float height = std::min(std::max(420.f, desiredHeight), static_cast<float>(VirtualViewport::height()) - 72.f);
    return Rectangle{VirtualViewport::width() * 0.5f - width * 0.5f, VirtualViewport::height() * 0.5f - height * 0.5f, width, height};
}

Rectangle ShopScene::relicOwnerOptionBounds(const std::size_t index) const {
    const Rectangle modal = relicOwnerModalBounds();
    return Rectangle{modal.x + 42.f, modal.y + 140.f + static_cast<float>(index) * 92.f, modal.width - 84.f, 76.f};
}

Rectangle ShopScene::relicOwnerCancelButtonBounds() const {
    const Rectangle modal = relicOwnerModalBounds();
    return Rectangle{modal.x + modal.width * 0.5f - 120.f, modal.y + modal.height - 58.f, 240.f, 44.f};
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

Rectangle ShopScene::hoverDescriptionBounds(const Vector2 mouse, const float height) const {
    constexpr float width = 520.f;
    const float x = std::clamp(mouse.x + 28.f, 28.f, static_cast<float>(VirtualViewport::width()) - width - 28.f);
    const float y = std::clamp(mouse.y + 28.f, 28.f, static_cast<float>(VirtualViewport::height()) - height - 28.f);
    return Rectangle{x, y, width, height};
}

void ShopScene::updateShop(const Vector2 mouse) {
    if (BasicUi::contains(leaveButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onLeave_();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < shopState_.offers.size(); ++i) {
        ShopOffer& offer = shopState_.offers[i];
        if (offer.purchased || !BasicUi::contains(offerBounds(i), mouse) || !canBuy(offer)) {
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

    const Rectangle modal = purchaseConfirmationBounds();
    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(purchaseCancelButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        pendingPurchaseIndex_.reset();
        return;
    }

    if (BasicUi::contains(purchaseConfirmButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

    for (std::size_t i = 0; i < runState_.actorStates.size(); ++i) {
        if (BasicUi::contains(relicOwnerOptionBounds(i), mouse)) {
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

    if (BasicUi::contains(relicOwnerCancelButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        pendingRelicOwnerPurchaseIndex_.reset();
        selectedRelicOwnerIndex_.reset();
        relicOwnerChoiceOpen_ = false;
    }
}

void ShopScene::updateRemoveMode(const Vector2 mouse) {
    const Rectangle modal = removeModeBounds();

    const float wheel = GetMouseWheelMove();
    if (wheel > 0.f && removeScrollOffset_ > 0u) {
        --removeScrollOffset_;
    } else if (wheel < 0.f && removeScrollOffset_ + visibleRemoveCardCount() < runState_.deckCardIds.size()) {
        ++removeScrollOffset_;
    }

    if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && removeScrollOffset_ > 0u) {
        --removeScrollOffset_;
    }
    if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) && removeScrollOffset_ + visibleRemoveCardCount() < runState_.deckCardIds.size()) {
        ++removeScrollOffset_;
    }

    clampRemoveScrollOffset();

    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(removeCancelButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        removeMode_ = false;
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    const std::size_t visibleCount = visibleRemoveCardCount();
    for (std::size_t visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
        const std::size_t deckIndex = removeScrollOffset_ + visibleIndex;
        if (deckIndex >= runState_.deckCardIds.size() || !BasicUi::contains(removeCardBounds(visibleIndex), mouse)) {
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
    const std::size_t visibleCount = visibleRemoveCardCount();
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
    const Rectangle bounds = offerBounds(offerIndex);
    const bool enabled = canBuy(offer);
    const bool hovered = enabled && BasicUi::contains(bounds, mouse);

    if (!shopState_.isMerchantRest()) {
        const Color fill = !enabled ? Color{33, 35, 43, 255} : (hovered ? Color{53, 58, 76, 255} : Color{39, 42, 55, 255});
        DrawRectangleRounded(bounds, 0.06f, 10, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.06f, 10, 2.f, hovered ? Color{238, 196, 86, 255} : Color{108, 118, 145, 255});
    }

    const Rectangle visualBounds = cardOfferVisualBounds(bounds);
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
    const Rectangle bounds = offerBounds(offerIndex);
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
        if (offer.purchased || (offer.type != ShopOfferType::Relic && offer.type != ShopOfferType::Consumable)) {
            continue;
        }
        if (!BasicUi::contains(offerBounds(i), mouse)) {
            continue;
        }

        const std::string description = offerDescription(offer);
        const std::vector<std::string> lines = BasicUi::wrapText(font_, description, 19.f, 468.f);
        const float height = std::min(260.f, 92.f + static_cast<float>(lines.size()) * 24.f);
        const Rectangle bounds = hoverDescriptionBounds(mouse, height);

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
    const Rectangle modal = purchaseConfirmationBounds();

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

    BasicUi::drawText(
        font_,
        offerKind(offer),
        Vector2{modal.x + 48.f, modal.y + 158.f},
        18.f,
        Color{170, 180, 204, 255}
    );

    BasicUi::drawButton(font_, purchaseConfirmButtonBounds(modal), localization_.get(TextId("shop.confirm.buy")), mouse);
    BasicUi::drawButton(font_, purchaseCancelButtonBounds(modal), localization_.get(TextId("reward.cancel")), mouse);
}

void ShopScene::renderRelicOwnerChoice() const {
    if (!pendingRelicOwnerPurchaseIndex_.has_value() || *pendingRelicOwnerPurchaseIndex_ >= shopState_.offers.size()) {
        return;
    }

    const ShopOffer& offer = shopState_.offers[*pendingRelicOwnerPurchaseIndex_];
    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = relicOwnerModalBounds();

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
        const Rectangle row = relicOwnerOptionBounds(i);
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

    BasicUi::drawButton(font_, relicOwnerCancelButtonBounds(), localization_.get(TextId("reward.cancel")), mouse);
}

void ShopScene::renderRemoveMode() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = removeModeBounds();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 145});
    DrawRectangleRounded(modal, 0.05f, 14, Color{26, 28, 38, 252});
    DrawRectangleRoundedLinesEx(modal, 0.05f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(font_, localization_.get(TextId("shop.remove_card_title")), Rectangle{modal.x + 20.f, modal.y + 22.f, modal.width - 40.f, 42.f}, 30.f, Color{255, 235, 175, 255});
    BasicUi::drawCenteredText(font_, localization_.format(TextId("shop.remove_card_description"), {{"price", std::to_string(shopState_.cardRemovalPrice)}}), Rectangle{modal.x + 30.f, modal.y + 66.f, modal.width - 60.f, 38.f}, 20.f, Color{190, 198, 220, 255});

    const std::size_t visibleCount = visibleRemoveCardCount();
    for (std::size_t visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
        const std::size_t deckIndex = removeScrollOffset_ + visibleIndex;
        if (deckIndex >= runState_.deckCardIds.size()) {
            break;
        }

        const Rectangle cell = removeCardBounds(visibleIndex);
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

    BasicUi::drawButton(font_, removeCancelButtonBounds(modal), localization_.get(TextId("reward.cancel")), mouse);
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

std::size_t ShopScene::cardOfferOrdinal(const std::size_t offerIndex) const {
    std::size_t ordinal = 0u;
    for (std::size_t i = 0u; i < offerIndex && i < shopState_.offers.size(); ++i) {
        if (shopState_.offers[i].type == ShopOfferType::Card &&
            (shopState_.isMerchantRest() || !shopState_.offers[i].purchased)) {
            ++ordinal;
        }
    }
    return ordinal;
}

std::size_t ShopScene::textOfferOrdinal(const std::size_t offerIndex) const {
    std::size_t ordinal = 0u;
    for (std::size_t i = 0u; i < offerIndex && i < shopState_.offers.size(); ++i) {
        if (!shopState_.offers[i].purchased && shopState_.offers[i].type != ShopOfferType::Card) {
            ++ordinal;
        }
    }
    return ordinal;
}

std::size_t ShopScene::visibleRemoveCardCount() const {
    const Rectangle modal = removeModeBounds();
    const float availableHeight = std::max(removeCardCellHeight, modal.height - 206.f);
    const std::size_t rows = std::max<std::size_t>(1u, static_cast<std::size_t>((availableHeight + removeCardCellSpacing) / (removeCardCellHeight + removeCardCellSpacing)));
    return rows * removeCardColumns;
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

std::string ShopScene::sceneSubtitle() const {
    if (!shopState_.isMerchantRest()) {
        return {};
    }

    return localization_.format(
        TextId("merchant_rest.subtitle"),
        {
            {"remaining", std::to_string(shopState_.cardPurchasesRemaining())},
            {"maximum", std::to_string(shopState_.maxCardPurchases)}
        }
    );
}

std::string ShopScene::leaveButtonText() const {
    return localization_.get(TextId(shopState_.isMerchantRest() ? "merchant_rest.leave" : "shop.leave"));
}

std::string ShopScene::priceText(const int price) const {
    return localization_.format(TextId("shop.price"), {{"price", std::to_string(price)}});
}
