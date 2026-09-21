#include "ShopLayout.hpp"

#include "ui/CardVisualInstance.hpp"
#include "ui/VirtualViewport.hpp"

#include <algorithm>

namespace {
constexpr float OFFER_SPACING = 20.f;
constexpr float TEXT_OFFER_HEIGHT = 122.f;
constexpr float TEXT_OFFER_SPACING = 16.f;
constexpr float REMOVE_CARD_CELL_HEIGHT = 310.f;
constexpr float REMOVE_CARD_CELL_SPACING = 16.f;
constexpr std::size_t REMOVE_CARD_COLUMNS = 4u;
constexpr float MERCHANT_REST_DESIRED_CARD_SCALE = 1.36f;
constexpr float MERCHANT_REST_MINIMUM_CARD_SCALE = 0.92f;
constexpr float MERCHANT_REST_CELL_HORIZONTAL_PADDING = 12.f;
constexpr float MERCHANT_REST_PRICE_GAP = 10.f;
constexpr float MERCHANT_REST_PRICE_HEIGHT = 32.f;
constexpr float MERCHANT_REST_STATUS_HEIGHT = 24.f;
constexpr float MERCHANT_REST_MAXIMUM_CARD_GAP = 52.f;
constexpr float MERCHANT_REST_MINIMUM_CARD_GAP = 18.f;
}

Rectangle ShopLayout::panelBounds() {
    const float width = std::min(1580.f, static_cast<float>(VirtualViewport::width()) - 80.f);
    const float height = std::min(790.f, static_cast<float>(VirtualViewport::height()) - 196.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        118.f,
        width,
        height
    };
}

Rectangle ShopLayout::cardOffersAreaBounds(const bool merchantRest) {
    const Rectangle panel = panelBounds();
    if (merchantRest) {
        return Rectangle{panel.x + 34.f, panel.y + 34.f, panel.width - 68.f, panel.height - 82.f};
    }

    const float width = std::min(930.f, panel.width * 0.60f);
    return Rectangle{panel.x + 34.f, panel.y + 34.f, width, panel.height - 82.f};
}

Rectangle ShopLayout::otherOffersAreaBounds(const bool merchantRest) {
    const Rectangle panel = panelBounds();
    if (merchantRest) {
        return Rectangle{panel.x + panel.width - 34.f, panel.y + 34.f, 0.f, panel.height - 82.f};
    }

    const Rectangle cards = cardOffersAreaBounds(false);
    const float x = cards.x + cards.width + 34.f;
    return Rectangle{x, panel.y + 34.f, std::max(420.f, panel.x + panel.width - x - 34.f), panel.height - 82.f};
}

Rectangle ShopLayout::offerBounds(const ShopState& shopState, const std::size_t index) {
    if (index >= shopState.offers.size()) {
        return Rectangle{};
    }

    const ShopOffer& offer = shopState.offers[index];
    if (offer.type == ShopOfferType::Card) {
        const Rectangle area = cardOffersAreaBounds(shopState.isMerchantRest());
        const std::size_t columns = cardOfferColumnCount(shopState);
        const std::size_t ordinal = cardOfferOrdinal(shopState, index);

        if (shopState.isMerchantRest()) {
            const Vector2 cardSize = CardVisualInstance::size();
            const float scale = merchantRestCardScale(shopState);
            const float cellWidth = cardSize.x * scale + MERCHANT_REST_CELL_HORIZONTAL_PADDING;
            const float cellHeight = cardSize.y * scale + MERCHANT_REST_PRICE_GAP + MERCHANT_REST_PRICE_HEIGHT + MERCHANT_REST_STATUS_HEIGHT;
            const float gap = merchantRestCardGap(area.width, cellWidth, columns);
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
        const float cellWidth = (area.width - OFFER_SPACING * (columnCount - 1.f)) / columnCount;
        const float cellHeight = cardOfferCellHeight(shopState);
        const std::size_t column = ordinal % columns;
        const std::size_t row = ordinal / columns;
        return Rectangle{
            area.x + static_cast<float>(column) * (cellWidth + OFFER_SPACING),
            area.y + static_cast<float>(row) * (cellHeight + OFFER_SPACING),
            cellWidth,
            cellHeight
        };
    }

    const Rectangle area = otherOffersAreaBounds(shopState.isMerchantRest());
    const std::size_t ordinal = textOfferOrdinal(shopState, index);
    return Rectangle{
        area.x,
        area.y + static_cast<float>(ordinal) * (TEXT_OFFER_HEIGHT + TEXT_OFFER_SPACING),
        area.width,
        TEXT_OFFER_HEIGHT
    };
}

Rectangle ShopLayout::cardOfferVisualBounds(const ShopState& shopState, const Rectangle cell) {
    if (shopState.isMerchantRest()) {
        const Vector2 cardSize = CardVisualInstance::size();
        const float scale = merchantRestCardScale(shopState);
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

std::size_t ShopLayout::cardOfferColumnCount(const ShopState& shopState) {
    const Rectangle area = cardOffersAreaBounds(shopState.isMerchantRest());
    const std::size_t cardCount = std::max<std::size_t>(1u, std::count_if(
        shopState.offers.begin(),
        shopState.offers.end(),
        [&shopState](const ShopOffer& offer) {
            return offer.type == ShopOfferType::Card && (shopState.isMerchantRest() || !offer.purchased);
        }
    ));

    if (shopState.isMerchantRest()) {
        return cardCount;
    }

    return area.width < 760.f ? 2u : 3u;
}

float ShopLayout::cardOfferCellHeight(const ShopState& shopState) {
    const Rectangle area = cardOffersAreaBounds(shopState.isMerchantRest());
    const std::size_t columns = cardOfferColumnCount(shopState);
    const std::size_t cardCount = std::max<std::size_t>(1u, std::count_if(
        shopState.offers.begin(),
        shopState.offers.end(),
        [&shopState](const ShopOffer& offer) {
            return offer.type == ShopOfferType::Card && (shopState.isMerchantRest() || !offer.purchased);
        }
    ));
    const std::size_t rows = std::max<std::size_t>(1u, (cardCount + columns - 1u) / columns);
    const float availableHeight = area.height - OFFER_SPACING * static_cast<float>(rows - 1u);
    const float fittedHeight = availableHeight / static_cast<float>(rows);

    if (shopState.isMerchantRest()) {
        const Vector2 cardSize = CardVisualInstance::size();
        const float scale = merchantRestCardScale(shopState);
        return cardSize.y * scale + MERCHANT_REST_PRICE_GAP + MERCHANT_REST_PRICE_HEIGHT + MERCHANT_REST_STATUS_HEIGHT;
    }

    return std::clamp(fittedHeight, 260.f, 376.f);
}

float ShopLayout::merchantRestCardScale(const ShopState& shopState) {
    const Rectangle area = cardOffersAreaBounds(true);
    const std::size_t cardCount = std::max<std::size_t>(1u, cardOfferColumnCount(shopState));
    const Vector2 cardSize = CardVisualInstance::size();
    const float horizontalGaps = MERCHANT_REST_MAXIMUM_CARD_GAP * static_cast<float>(cardCount - 1u);
    const float horizontalPadding = MERCHANT_REST_CELL_HORIZONTAL_PADDING * static_cast<float>(cardCount);
    const float widthScale = (area.width - horizontalGaps - horizontalPadding) /
        (cardSize.x * static_cast<float>(cardCount));
    const float heightScale = (area.height - MERCHANT_REST_PRICE_GAP - MERCHANT_REST_PRICE_HEIGHT - MERCHANT_REST_STATUS_HEIGHT) /
        cardSize.y;
    const float fittedScale = std::min({MERCHANT_REST_DESIRED_CARD_SCALE, widthScale, heightScale});
    return std::clamp(fittedScale, MERCHANT_REST_MINIMUM_CARD_SCALE, MERCHANT_REST_DESIRED_CARD_SCALE);
}

float ShopLayout::merchantRestCardGap(
    const float areaWidth,
    const float cellWidth,
    const std::size_t cardCount
) {
    if (cardCount <= 1u) {
        return 0.f;
    }

    const float remainingWidth = areaWidth - cellWidth * static_cast<float>(cardCount);
    const float fittedGap = remainingWidth / static_cast<float>(cardCount - 1u);
    return std::clamp(fittedGap, MERCHANT_REST_MINIMUM_CARD_GAP, MERCHANT_REST_MAXIMUM_CARD_GAP);
}

Rectangle ShopLayout::leaveButtonBounds() {
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - 150.f,
        static_cast<float>(VirtualViewport::height()) - 82.f,
        300.f,
        50.f
    };
}

Rectangle ShopLayout::removeModeBounds() {
    const float width = std::min(1080.f, static_cast<float>(VirtualViewport::width()) - 70.f);
    const float height = std::min(660.f, static_cast<float>(VirtualViewport::height()) - 70.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle ShopLayout::removeCardBounds(const std::size_t index) {
    const Rectangle modal = removeModeBounds();
    const float gridX = modal.x + 35.f;
    const float gridY = modal.y + 124.f;
    const float gridWidth = modal.width - 70.f;
    const float cellWidth = (gridWidth - REMOVE_CARD_CELL_SPACING * static_cast<float>(REMOVE_CARD_COLUMNS - 1u)) /
        static_cast<float>(REMOVE_CARD_COLUMNS);
    const std::size_t column = index % REMOVE_CARD_COLUMNS;
    const std::size_t row = index / REMOVE_CARD_COLUMNS;

    return Rectangle{
        gridX + static_cast<float>(column) * (cellWidth + REMOVE_CARD_CELL_SPACING),
        gridY + static_cast<float>(row) * (REMOVE_CARD_CELL_HEIGHT + REMOVE_CARD_CELL_SPACING),
        cellWidth,
        REMOVE_CARD_CELL_HEIGHT
    };
}

Rectangle ShopLayout::removeCancelButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width * 0.5f - 130.f, modal.y + modal.height - 66.f, 260.f, 46.f};
}

std::size_t ShopLayout::visibleRemoveCardCount() {
    const Rectangle modal = removeModeBounds();
    const float availableHeight = std::max(REMOVE_CARD_CELL_HEIGHT, modal.height - 206.f);
    const std::size_t rows = std::max<std::size_t>(
        1u,
        static_cast<std::size_t>((availableHeight + REMOVE_CARD_CELL_SPACING) /
            (REMOVE_CARD_CELL_HEIGHT + REMOVE_CARD_CELL_SPACING))
    );
    return rows * REMOVE_CARD_COLUMNS;
}

Rectangle ShopLayout::purchaseConfirmationBounds(const bool activeItem) {
    const float width = activeItem ? std::min(920.f, static_cast<float>(VirtualViewport::width()) - 72.f) : 620.f;
    const float height = activeItem ? std::min(590.f, static_cast<float>(VirtualViewport::height()) - 72.f) : 270.f;
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle ShopLayout::purchaseConfirmButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + 64.f, modal.y + modal.height - 76.f, 220.f, 50.f};
}

Rectangle ShopLayout::purchaseCancelButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width - 284.f, modal.y + modal.height - 76.f, 220.f, 50.f};
}

Rectangle ShopLayout::relicOwnerModalBounds(const std::size_t actorCount) {
    const float width = 700.f;
    const float desiredHeight = 188.f + static_cast<float>(actorCount) * 92.f + 76.f;
    const float height = std::min(std::max(420.f, desiredHeight), static_cast<float>(VirtualViewport::height()) - 72.f);
    return Rectangle{
        VirtualViewport::width() * 0.5f - width * 0.5f,
        VirtualViewport::height() * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle ShopLayout::relicOwnerOptionBounds(const Rectangle modal, const std::size_t index) {
    return Rectangle{modal.x + 42.f, modal.y + 140.f + static_cast<float>(index) * 92.f, modal.width - 84.f, 76.f};
}

Rectangle ShopLayout::relicOwnerCancelButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width * 0.5f - 120.f, modal.y + modal.height - 58.f, 240.f, 44.f};
}

Rectangle ShopLayout::hoverDescriptionBounds(const Vector2 mouse, const float height) {
    constexpr float width = 520.f;
    const float x = std::clamp(mouse.x + 28.f, 28.f, static_cast<float>(VirtualViewport::width()) - width - 28.f);
    const float y = std::clamp(mouse.y + 28.f, 28.f, static_cast<float>(VirtualViewport::height()) - height - 28.f);
    return Rectangle{x, y, width, height};
}

std::size_t ShopLayout::cardOfferOrdinal(const ShopState& shopState, const std::size_t offerIndex) {
    std::size_t ordinal = 0u;
    for (std::size_t i = 0u; i < offerIndex && i < shopState.offers.size(); ++i) {
        if (shopState.offers[i].type == ShopOfferType::Card &&
            (shopState.isMerchantRest() || !shopState.offers[i].purchased)) {
            ++ordinal;
        }
    }
    return ordinal;
}

std::size_t ShopLayout::textOfferOrdinal(const ShopState& shopState, const std::size_t offerIndex) {
    std::size_t ordinal = 0u;
    for (std::size_t i = 0u; i < offerIndex && i < shopState.offers.size(); ++i) {
        if (!shopState.offers[i].purchased && shopState.offers[i].type != ShopOfferType::Card) {
            ++ordinal;
        }
    }
    return ordinal;
}
