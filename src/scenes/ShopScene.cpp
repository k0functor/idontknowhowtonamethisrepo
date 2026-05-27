#include "ShopScene.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr float offerHeight = 112.f;
constexpr float offerSpacing = 18.f;
}

ShopScene::ShopScene(
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
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      relics_(relics),
      consumables_(consumables),
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

    updateShop(mouse);
}

void ShopScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("shop.title")),
        Rectangle{0.f, 54.f, static_cast<float>(GetScreenWidth()), 54.f},
        40.f,
        Color{244, 233, 188, 255}
    );

    BasicUi::drawCenteredText(
        font_,
        localization_.format(TextId("shop.gold"), {{"gold", std::to_string(runState_.gold)}}),
        Rectangle{0.f, 104.f, static_cast<float>(GetScreenWidth()), 34.f},
        23.f,
        Color{219, 205, 130, 255}
    );

    const Rectangle panel = panelBounds();
    DrawRectangleRounded(panel, 0.04f, 14, Color{29, 31, 41, 250});
    DrawRectangleRoundedLinesEx(panel, 0.04f, 14, 2.f, Color{111, 122, 150, 255});

    renderOffers();

    BasicUi::drawButton(font_, leaveButtonBounds(), localization_.get(TextId("shop.leave")), mouse);

    if (removeMode_) {
        renderRemoveMode();
    }
}

Rectangle ShopScene::panelBounds() const {
    const float width = std::min(1100.f, static_cast<float>(GetScreenWidth()) - 88.f);
    const float height = std::min(610.f, static_cast<float>(GetScreenHeight()) - 180.f);
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        150.f,
        width,
        height
    };
}

Rectangle ShopScene::offerBounds(const std::size_t index) const {
    const Rectangle panel = panelBounds();
    const float columns = 2.f;
    const float columnSpacing = 22.f;
    const float rowSpacing = offerSpacing;
    const float width = (panel.width - 70.f - columnSpacing) / columns;
    const float x = panel.x + 35.f + static_cast<float>(index % 2) * (width + columnSpacing);
    const float y = panel.y + 34.f + static_cast<float>(index / 2) * (offerHeight + rowSpacing);
    return Rectangle{x, y, width, offerHeight};
}

Rectangle ShopScene::leaveButtonBounds() const {
    return Rectangle{
        static_cast<float>(GetScreenWidth()) * 0.5f - 150.f,
        static_cast<float>(GetScreenHeight()) - 82.f,
        300.f,
        50.f
    };
}

Rectangle ShopScene::removeModeBounds() const {
    const float width = std::min(980.f, static_cast<float>(GetScreenWidth()) - 90.f);
    const float height = std::min(560.f, static_cast<float>(GetScreenHeight()) - 90.f);
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        (static_cast<float>(GetScreenHeight()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle ShopScene::removeCardBounds(const std::size_t index) const {
    const Rectangle modal = removeModeBounds();
    const float width = modal.width - 70.f;
    return Rectangle{modal.x + 35.f, modal.y + 104.f + static_cast<float>(index) * 52.f, width, 42.f};
}

Rectangle ShopScene::removeCancelButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f - 130.f, modal.y + modal.height - 66.f, 260.f, 46.f};
}

void ShopScene::updateShop(const Vector2 mouse) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        onLeave_();
        return;
    }

    if (BasicUi::contains(leaveButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onLeave_();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < shopState_.offers.size(); ++i) {
        ShopOffer& offer = shopState_.offers[i];
        if (!BasicUi::contains(offerBounds(i), mouse) || !canBuy(offer)) {
            continue;
        }

        if (offer.type == ShopOfferType::CardRemoval) {
            removeMode_ = true;
            return;
        }

        ShopPurchase purchase;
        purchase.type = offer.type;
        purchase.contentId = offer.contentId;
        purchase.price = offer.price;

        if (onPurchase_(purchase)) {
            offer.purchased = true;
            if (onShopStateChanged_) {
                onShopStateChanged_(shopState_);
            }
        }
        return;
    }
}

void ShopScene::updateRemoveMode(const Vector2 mouse) {
    const Rectangle modal = removeModeBounds();

    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(removeCancelButtonBounds(modal), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        removeMode_ = false;
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < runState_.deckCardIds.size(); ++i) {
        if (!BasicUi::contains(removeCardBounds(i), mouse)) {
            continue;
        }

        ShopPurchase purchase;
        purchase.type = ShopOfferType::CardRemoval;
        purchase.cardId = runState_.deckCardIds[i];
        purchase.price = shopState_.cardRemovalPrice;

        if (onPurchase_(purchase)) {
            shopState_.cardRemovalUsed = true;
            for (ShopOffer& offer : shopState_.offers) {
                if (offer.type == ShopOfferType::CardRemoval) {
                    offer.purchased = true;
                    break;
                }
            }
            if (onShopStateChanged_) {
                onShopStateChanged_(shopState_);
            }
            removeMode_ = false;
        }
        return;
    }
}

void ShopScene::renderOffers() const {
    const Vector2 mouse = GetMousePosition();

    for (std::size_t i = 0; i < shopState_.offers.size(); ++i) {
        const ShopOffer& offer = shopState_.offers[i];
        const Rectangle bounds = offerBounds(i);
        const bool enabled = canBuy(offer);
        const bool hovered = enabled && BasicUi::contains(bounds, mouse);
        const Color fill = !enabled ? Color{34, 36, 44, 255} : (hovered ? Color{55, 60, 78, 255} : Color{41, 44, 58, 255});

        DrawRectangleRounded(bounds, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, 2.f, hovered ? Color{238, 196, 86, 255} : Color{108, 118, 145, 255});

        BasicUi::drawText(font_, offerKind(offer), Vector2{bounds.x + 16.f, bounds.y + 11.f}, 15.f, Color{180, 188, 210, 255});
        BasicUi::drawText(font_, offerName(offer), Vector2{bounds.x + 16.f, bounds.y + 34.f}, 22.f, enabled ? Color{244, 244, 250, 255} : Color{135, 139, 154, 255});
        BasicUi::drawText(font_, priceText(offer.price), Vector2{bounds.x + bounds.width - 104.f, bounds.y + 13.f}, 18.f, enabled ? Color{236, 214, 126, 255} : Color{130, 125, 96, 255});

        const std::string description = offerDescription(offer);
        const std::vector<std::string> lines = BasicUi::wrapText(font_, description, 14.f, bounds.width - 34.f);
        float y = bounds.y + 67.f;
        for (const std::string& line : lines) {
            if (y > bounds.y + bounds.height - 18.f) {
                break;
            }
            BasicUi::drawText(font_, line, Vector2{bounds.x + 16.f, y}, 14.f, Color{190, 198, 220, 255});
            y += 18.f;
        }

        if (offer.purchased) {
            BasicUi::drawCenteredText(font_, localization_.get(TextId("shop.sold")), bounds, 26.f, Color{245, 220, 145, 220});
        }
    }
}

void ShopScene::renderRemoveMode() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = removeModeBounds();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 145});
    DrawRectangleRounded(modal, 0.05f, 14, Color{26, 28, 38, 252});
    DrawRectangleRoundedLinesEx(modal, 0.05f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(font_, localization_.get(TextId("shop.remove_card_title")), Rectangle{modal.x + 20.f, modal.y + 22.f, modal.width - 40.f, 42.f}, 30.f, Color{255, 235, 175, 255});
    BasicUi::drawCenteredText(font_, localization_.format(TextId("shop.remove_card_description"), {{"price", std::to_string(shopState_.cardRemovalPrice)}}), Rectangle{modal.x + 30.f, modal.y + 64.f, modal.width - 60.f, 30.f}, 17.f, Color{190, 198, 220, 255});

    const std::size_t maxVisible = std::min<std::size_t>(runState_.deckCardIds.size(), 7);
    for (std::size_t i = 0; i < maxVisible; ++i) {
        const Rectangle row = removeCardBounds(i);
        const bool hovered = BasicUi::contains(row, mouse);
        DrawRectangleRounded(row, 0.08f, 8, hovered ? Color{55, 60, 78, 255} : Color{40, 43, 56, 255});
        DrawRectangleRoundedLinesEx(row, 0.08f, 8, 2.f, hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255});
        BasicUi::drawText(font_, cardName(runState_.deckCardIds[i]), Vector2{row.x + 16.f, row.y + 10.f}, 19.f, Color{238, 238, 245, 255});
    }

    if (runState_.deckCardIds.size() > maxVisible) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("shop.remove_only_first_cards")), Rectangle{modal.x + 30.f, modal.y + modal.height - 112.f, modal.width - 60.f, 28.f}, 15.f, Color{174, 180, 202, 255});
    }

    BasicUi::drawButton(font_, removeCancelButtonBounds(modal), localization_.get(TextId("reward.cancel")), mouse);
}

bool ShopScene::canBuy(const ShopOffer& offer) const {
    if (offer.purchased) {
        return false;
    }

    if (runState_.gold < offer.price) {
        return false;
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

std::string ShopScene::priceText(const int price) {
    return std::to_string(price) + "g";
}
