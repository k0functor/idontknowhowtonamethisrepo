#include "MerchantRestScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardUpgrade.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr float rowHeight = 70.f;
constexpr float rowGap = 10.f;

Color actionBorderColor(const bool selected) {
    return selected ? Color{235, 196, 92, 255} : Color{112, 122, 150, 230};
}
}

MerchantRestScene::MerchantRestScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const RunState& runState,
    ShopState shopState,
    std::function<void(ShopState)> onBuyCards,
    std::function<void()> onHeal,
    std::function<void(std::size_t)> onUpgrade,
    std::function<void()> onSkip
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      runState_(runState),
      shopState_(std::move(shopState)),
      onBuyCards_(std::move(onBuyCards)),
      onHeal_(std::move(onHeal)),
      onUpgrade_(std::move(onUpgrade)),
      onSkip_(std::move(onSkip)) {}

void MerchantRestScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (mode_ == Mode::UpgradeList) {
        updateUpgradeList(mouse);
        return;
    }

    updateActions(mouse);
}

void MerchantRestScene::render() const {
    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{10, 12, 18, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("merchant_rest.title")),
        Rectangle{0.f, 24.f, static_cast<float>(VirtualViewport::width()), 54.f},
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

    if (mode_ == Mode::UpgradeList) {
        renderUpgradeList();
        return;
    }

    renderActions();
}

Rectangle MerchantRestScene::panelBounds() const {
    const float width = std::min(980.f, static_cast<float>(VirtualViewport::width()) - 96.f);
    const float height = std::min(610.f, static_cast<float>(VirtualViewport::height()) - 150.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        136.f,
        width,
        height
    };
}

Rectangle MerchantRestScene::actionButtonBounds(const std::size_t index) const {
    const Rectangle panel = panelBounds();
    constexpr float gap = 24.f;
    const float width = (panel.width - 96.f - 2.f * gap) / 3.f;
    return Rectangle{
        panel.x + 48.f + static_cast<float>(index) * (width + gap),
        panel.y + 190.f,
        width,
        188.f
    };
}

Rectangle MerchantRestScene::skipButtonBounds() const {
    const Rectangle panel = panelBounds();
    return Rectangle{panel.x + panel.width - 244.f, panel.y + panel.height - 70.f, 198.f, 44.f};
}

Rectangle MerchantRestScene::upgradeListBounds() const {
    const Rectangle panel = panelBounds();
    return Rectangle{panel.x + 44.f, panel.y + 132.f, panel.width * 0.52f, panel.height - 222.f};
}

Rectangle MerchantRestScene::upgradeBackButtonBounds(const Rectangle panel) const {
    return Rectangle{panel.x + 44.f, panel.y + panel.height - 70.f, 170.f, 44.f};
}

Rectangle MerchantRestScene::upgradeRowBounds(const Rectangle list, const std::size_t rowIndex) const {
    return Rectangle{
        list.x + 10.f,
        list.y + 10.f + static_cast<float>(rowIndex) * (rowHeight + rowGap) - upgradeScrollOffset_,
        list.width - 20.f,
        rowHeight
    };
}

Rectangle MerchantRestScene::upgradePreviewBounds(const Rectangle panel) const {
    return Rectangle{panel.x + panel.width * 0.60f, panel.y + 132.f, panel.width * 0.34f, panel.height - 222.f};
}

Rectangle MerchantRestScene::upgradeConfirmButtonBounds(const Rectangle panel) const {
    return Rectangle{panel.x + panel.width - 244.f, panel.y + panel.height - 70.f, 198.f, 44.f};
}

Rectangle MerchantRestScene::upgradeCancelButtonBounds(const Rectangle panel) const {
    return Rectangle{panel.x + panel.width - 462.f, panel.y + panel.height - 70.f, 198.f, 44.f};
}

void MerchantRestScene::updateActions(const Vector2 mousePosition) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        onSkip_();
        return;
    }

    if ((IsKeyPressed(KEY_ONE) ||
         (BasicUi::contains(actionButtonBounds(0u), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) &&
        canOpenCardShop()) {
        shopState_.merchantRestCardShopOpen = true;
        onBuyCards_(shopState_);
        return;
    }

    const bool canUpgrade = !upgradableDeckIndices().empty();
    if ((IsKeyPressed(KEY_TWO) ||
         (BasicUi::contains(actionButtonBounds(1u), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) &&
        canUpgrade) {
        mode_ = Mode::UpgradeList;
        upgradeScrollOffset_ = 0.f;
        selectedUpgradeDeckIndex_.reset();
        return;
    }

    if (IsKeyPressed(KEY_THREE) ||
        (BasicUi::contains(actionButtonBounds(2u), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        onHeal_();
        return;
    }

    if (IsKeyPressed(KEY_FOUR) ||
        (BasicUi::contains(skipButtonBounds(), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        onSkip_();
    }
}

void MerchantRestScene::updateUpgradeList(const Vector2 mousePosition) {
    const Rectangle panel = panelBounds();
    const Rectangle list = upgradeListBounds();
    const std::vector<std::size_t> deckIndices = upgradableDeckIndices();

    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(upgradeBackButtonBounds(panel), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        mode_ = Mode::Actions;
        selectedUpgradeDeckIndex_.reset();
        return;
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.f && BasicUi::contains(list, mousePosition)) {
        const float totalHeight = 20.f + static_cast<float>(deckIndices.size()) * rowHeight +
            static_cast<float>(deckIndices.empty() ? 0u : deckIndices.size() - 1u) * rowGap;
        const float maxScroll = std::max(0.f, totalHeight - list.height);
        upgradeScrollOffset_ = std::clamp(upgradeScrollOffset_ - wheel * 38.f, 0.f, maxScroll);
    }

    for (std::size_t row = 0u; row < deckIndices.size(); ++row) {
        const Rectangle rowBounds = upgradeRowBounds(list, row);
        if (rowBounds.y + rowBounds.height < list.y || rowBounds.y > list.y + list.height) {
            continue;
        }
        if (BasicUi::contains(rowBounds, mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedUpgradeDeckIndex_ = deckIndices[row];
            return;
        }
    }

    if (selectedUpgradeDeckIndex_.has_value() &&
        (IsKeyPressed(KEY_ENTER) ||
         (BasicUi::contains(upgradeConfirmButtonBounds(panel), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))) {
        onUpgrade_(*selectedUpgradeDeckIndex_);
        return;
    }

    if (selectedUpgradeDeckIndex_.has_value() &&
        BasicUi::contains(upgradeCancelButtonBounds(panel), mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        selectedUpgradeDeckIndex_.reset();
    }
}

void MerchantRestScene::renderActions() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = panelBounds();

    DrawRectangleRounded(panel, 0.045f, 14, Color{29, 31, 41, 250});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 2.f, Color{111, 122, 150, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("merchant_rest.choose_action")),
        Rectangle{panel.x + 40.f, panel.y + 30.f, panel.width - 80.f, 34.f},
        28.f,
        Color{245, 226, 166, 255}
    );

    const std::vector<std::string> copy = BasicUi::wrapText(
        font_,
        localization_.get(TextId("merchant_rest.choose_action_description")),
        17.f,
        panel.width - 120.f
    );
    float y = panel.y + 78.f;
    for (const std::string& line : copy) {
        BasicUi::drawCenteredText(font_, line, Rectangle{panel.x + 60.f, y, panel.width - 120.f, 22.f}, 17.f, Color{184, 193, 214, 255});
        y += 22.f;
    }

    struct ActionView {
        const char* titleKey;
        const char* hotkeyKey;
        std::string description;
        bool enabled;
    };

    const std::vector<std::size_t> upgradable = upgradableDeckIndices();
    const ActionView actions[3] = {
        ActionView{"merchant_rest.buy_cards", "merchant_rest.hotkey_1", buyCardsSummary(), canOpenCardShop()},
        ActionView{"merchant_rest.upgrade_card", "merchant_rest.hotkey_2", localization_.format(TextId("merchant_rest.upgrade_summary"), {{"count", std::to_string(upgradable.size())}}), !upgradable.empty()},
        ActionView{"merchant_rest.heal", "merchant_rest.hotkey_3", healPreviewText() + " " + stressPreviewText(), true}
    };

    for (std::size_t i = 0u; i < 3u; ++i) {
        const Rectangle bounds = actionButtonBounds(i);
        const bool hovered = actions[i].enabled && BasicUi::contains(bounds, mouse);
        DrawRectangleRounded(bounds, 0.065f, 12, hovered ? Color{48, 52, 66, 255} : Color{36, 39, 51, 255});
        DrawRectangleRoundedLinesEx(bounds, 0.065f, 12, hovered ? 3.f : 2.f, actionBorderColor(hovered));

        BasicUi::drawCenteredText(
            font_,
            localization_.get(TextId(actions[i].titleKey)),
            Rectangle{bounds.x + 18.f, bounds.y + 18.f, bounds.width - 36.f, 32.f},
            23.f,
            actions[i].enabled ? Color{245, 226, 166, 255} : Color{126, 132, 150, 255}
        );
        BasicUi::drawCenteredText(
            font_,
            localization_.get(TextId(actions[i].hotkeyKey)),
            Rectangle{bounds.x + 18.f, bounds.y + 52.f, bounds.width - 36.f, 24.f},
            16.f,
            actions[i].enabled ? Color{164, 178, 214, 255} : Color{100, 106, 124, 255}
        );

        const std::vector<std::string> lines = BasicUi::wrapText(font_, actions[i].description, 15.f, bounds.width - 36.f);
        y = bounds.y + 88.f;
        for (const std::string& line : lines) {
            if (y > bounds.y + bounds.height - 22.f) {
                break;
            }
            BasicUi::drawText(font_, line, Vector2{bounds.x + 18.f, y}, 15.f, actions[i].enabled ? Color{205, 214, 232, 255} : Color{120, 126, 142, 255});
            y += 19.f;
        }
    }

    BasicUi::drawButton(font_, skipButtonBounds(), localization_.get(TextId("rest.skip")), mouse);
}

void MerchantRestScene::renderUpgradeList() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = panelBounds();
    const Rectangle list = upgradeListBounds();
    const std::vector<std::size_t> deckIndices = upgradableDeckIndices();

    DrawRectangleRounded(panel, 0.045f, 14, Color{29, 31, 41, 250});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 2.f, Color{111, 122, 150, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("rest.upgrade_title")),
        Rectangle{panel.x + 40.f, panel.y + 28.f, panel.width - 80.f, 34.f},
        29.f,
        Color{245, 226, 166, 255}
    );

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("merchant_rest.upgrade_action_hint")),
        Rectangle{panel.x + 54.f, panel.y + 70.f, panel.width - 108.f, 24.f},
        16.f,
        Color{184, 193, 214, 255}
    );

    DrawRectangleRounded(list, 0.035f, 10, Color{22, 24, 32, 255});
    DrawRectangleRoundedLinesEx(list, 0.035f, 10, 2.f, Color{86, 96, 126, 220});

    if (deckIndices.empty()) {
        BasicUi::drawCenteredText(
            font_,
            localization_.get(TextId("rest.no_upgradable_cards")),
            list,
            18.f,
            Color{205, 210, 225, 255}
        );
    }

    for (std::size_t row = 0u; row < deckIndices.size(); ++row) {
        const Rectangle bounds = upgradeRowBounds(list, row);
        if (bounds.y + bounds.height < list.y || bounds.y > list.y + list.height) {
            continue;
        }

        const std::size_t deckIndex = deckIndices[row];
        const bool selected = selectedUpgradeDeckIndex_.has_value() && *selectedUpgradeDeckIndex_ == deckIndex;
        const bool hovered = BasicUi::contains(bounds, mouse);
        DrawRectangleRounded(bounds, 0.075f, 8, selected ? Color{58, 49, 30, 255} : (hovered ? Color{42, 46, 60, 255} : Color{32, 35, 46, 255}));
        DrawRectangleRoundedLinesEx(bounds, 0.075f, 8, selected ? 3.f : 1.5f, selected ? Color{238, 196, 86, 255} : Color{91, 101, 130, 210});

        const CardId& cardId = runState_.deckCardIds[deckIndex];
        BasicUi::drawText(
            font_,
            localization_.format(TextId("merchant_rest.deck_card_row"), {{"index", std::to_string(deckIndex + 1)}, {"name", cardName(cardId)}}),
            Vector2{bounds.x + 18.f, bounds.y + 13.f},
            18.f,
            Color{235, 238, 246, 255}
        );
        BasicUi::drawText(
            font_,
            cardUpgradeSummary(cardId),
            Vector2{bounds.x + 18.f, bounds.y + 39.f},
            14.f,
            Color{174, 184, 206, 255}
        );
    }

    renderUpgradePreview(panel);

    BasicUi::drawButton(font_, upgradeBackButtonBounds(panel), localization_.get(TextId("rest.back")), mouse);
    BasicUi::drawButton(
        font_,
        upgradeCancelButtonBounds(panel),
        localization_.get(TextId("ui.cancel")),
        mouse,
        selectedUpgradeDeckIndex_.has_value()
    );
    BasicUi::drawButton(
        font_,
        upgradeConfirmButtonBounds(panel),
        localization_.get(TextId("rest.confirm_upgrade")),
        mouse,
        selectedUpgradeDeckIndex_.has_value()
    );
}

void MerchantRestScene::renderUpgradePreview(const Rectangle panel) const {
    const Rectangle preview = upgradePreviewBounds(panel);
    DrawRectangleRounded(preview, 0.04f, 10, Color{22, 24, 32, 255});
    DrawRectangleRoundedLinesEx(preview, 0.04f, 10, 2.f, Color{86, 96, 126, 220});

    if (!selectedUpgradeDeckIndex_.has_value()) {
        const std::vector<std::string> lines = BasicUi::wrapText(
            font_,
            localization_.get(TextId("rest.select_upgrade_card")),
            17.f,
            preview.width - 42.f
        );
        float y = preview.y + 34.f;
        for (const std::string& line : lines) {
            BasicUi::drawText(font_, line, Vector2{preview.x + 22.f, y}, 17.f, Color{184, 193, 214, 255});
            y += 22.f;
        }
        return;
    }

    const std::size_t deckIndex = *selectedUpgradeDeckIndex_;
    if (deckIndex >= runState_.deckCardIds.size()) {
        return;
    }

    const CardId& cardId = runState_.deckCardIds[deckIndex];
    BasicUi::drawText(
        font_,
        localization_.get(TextId("rest.upgrade_summary")),
        Vector2{preview.x + 22.f, preview.y + 24.f},
        20.f,
        Color{245, 220, 140, 255}
    );

    BasicUi::drawText(
        font_,
        cardName(cardId),
        Vector2{preview.x + 22.f, preview.y + 58.f},
        22.f,
        Color{235, 238, 246, 255}
    );

    const std::vector<std::string> summaryLines = BasicUi::wrapText(font_, cardUpgradeSummary(cardId), 17.f, preview.width - 44.f);
    float y = preview.y + 98.f;
    for (const std::string& line : summaryLines) {
        if (y > preview.y + preview.height - 36.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{preview.x + 22.f, y}, 17.f, Color{205, 214, 232, 255});
        y += 22.f;
    }
}

bool MerchantRestScene::isDeckIndexUpgraded(const std::size_t deckIndex) const {
    if (deckIndex > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    const int value = static_cast<int>(deckIndex);
    return std::find(runState_.upgradedDeckIndices.begin(), runState_.upgradedDeckIndices.end(), value) !=
        runState_.upgradedDeckIndices.end();
}

bool MerchantRestScene::canUpgradeDeckIndex(const std::size_t deckIndex) const {
    if (deckIndex >= runState_.deckCardIds.size() || isDeckIndexUpgraded(deckIndex)) {
        return false;
    }

    const CardId& cardId = runState_.deckCardIds[deckIndex];
    return cards_.contains(cardId) && CardUpgrade::isUpgradable(cards_.get(cardId));
}

std::vector<std::size_t> MerchantRestScene::upgradableDeckIndices() const {
    std::vector<std::size_t> result;
    for (std::size_t index = 0u; index < runState_.deckCardIds.size(); ++index) {
        if (canUpgradeDeckIndex(index)) {
            result.push_back(index);
        }
    }
    return result;
}

std::string MerchantRestScene::cardName(const CardId& cardId) const {
    if (!cards_.contains(cardId)) {
        return cardId.value;
    }
    return localization_.get(cards_.get(cardId).nameTextId);
}

std::string MerchantRestScene::cardUpgradeSummary(const CardId& cardId) const {
    if (!cards_.contains(cardId)) {
        return cardId.value;
    }

    const CardDefinition& base = cards_.get(cardId);
    if (!CardUpgrade::isUpgradable(base)) {
        return localization_.get(TextId("rest.card_not_upgradable"));
    }

    const CardDefinition upgraded = CardUpgrade::upgradedDefinition(base);
    return CardUpgrade::summary(base, upgraded, localization_);
}

std::string MerchantRestScene::healPreviewText() const {
    int current = 0;
    int after = 0;
    int maximum = 0;

    for (const RunActorState& actor : runState_.actorStates) {
        const int actorMaximum = std::max(1, actor.maxHp);
        const int actorCurrent = std::clamp(actor.currentHp, 0, actorMaximum);
        const int amount = std::max(1, static_cast<int>(static_cast<float>(actorMaximum) * 0.30f + 0.5f));
        current += actorCurrent;
        after += std::clamp(actorCurrent + amount, 0, actorMaximum);
        maximum += actorMaximum;
    }

    return localization_.format(
        TextId("rest.heal_preview"),
        {{"current", std::to_string(current)}, {"after", std::to_string(after)}, {"maximum", std::to_string(maximum)}}
    );
}

std::string MerchantRestScene::stressPreviewText() const {
    int current = 0;
    int after = 0;
    int maximum = 0;

    for (const RunActorState& actor : runState_.actorStates) {
        const int actorMaximum = std::max(1, actor.maxStress);
        const int actorCurrent = std::clamp(actor.stress, 0, actorMaximum);
        current += actorCurrent;
        after += std::max(0, actorCurrent - 30);
        maximum += actorMaximum;
    }

    return localization_.format(
        TextId("rest.stress_preview"),
        {{"current", std::to_string(current)}, {"after", std::to_string(after)}, {"maximum", std::to_string(maximum)}}
    );
}

std::string MerchantRestScene::buyCardsSummary() const {
    return localization_.format(
        TextId("merchant_rest.buy_cards_summary"),
        {
            {"offers", std::to_string(shopState_.offers.size())},
            {"remaining", std::to_string(shopState_.cardPurchasesRemaining())},
            {"maximum", std::to_string(shopState_.maxCardPurchases)}
        }
    );
}

bool MerchantRestScene::canOpenCardShop() const {
    return shopState_.cardPurchasesRemaining() > 0 && !shopState_.offers.empty();
}
