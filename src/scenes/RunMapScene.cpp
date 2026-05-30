#include "RunMapScene.hpp"

#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"
#include "ui/BasicUi.hpp"
#include "localization/TextFormatter.hpp"
#include "relics/RelicDefinition.hpp"
#include "consumables/ConsumableDefinition.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <utility>
#include <raylib.h>

namespace {
constexpr float NODE_WIDTH = 108.f;
constexpr float NODE_HEIGHT = 64.f;

struct MapRawBounds {
    float minX = 0.f;
    float maxX = 0.f;
    float minY = 0.f;
    float maxY = 0.f;
};

MapRawBounds calculateRawBounds(const RunMap& map) {
    if (map.nodes.empty()) {
        return {};
    }

    MapRawBounds bounds;
    bounds.minX = std::numeric_limits<float>::max();
    bounds.maxX = std::numeric_limits<float>::lowest();
    bounds.minY = std::numeric_limits<float>::max();
    bounds.maxY = std::numeric_limits<float>::lowest();

    for (const RunMapNode& node : map.nodes) {
        bounds.minX = std::min(bounds.minX, node.position.x);
        bounds.maxX = std::max(bounds.maxX, node.position.x);
        bounds.minY = std::min(bounds.minY, node.position.y);
        bounds.maxY = std::max(bounds.maxY, node.position.y);
    }

    return bounds;
}

float safeDimension(const float value) {
    return std::max(value, 1.f);
}

const RunMapNode* findNodeById(const RunMap& map, const int nodeId) {
    for (const RunMapNode& node : map.nodes) {
        if (node.id == nodeId) {
            return &node;
        }
    }

    return nullptr;
}
}

RunMapScene::RunMapScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const RunState& runState,
    std::function<void(int)> onNodeSelected,
    std::function<void(int)> onRestHeal,
    std::function<void(int, CardId)> onRestUpgrade,
    std::function<void(int)> onRestSkip,
    std::function<void()> onBackToHub
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      relics_(relics),
      consumables_(consumables),
      runState_(runState),
      onNodeSelected_(std::move(onNodeSelected)),
      onRestHeal_(std::move(onRestHeal)),
      onRestUpgrade_(std::move(onRestUpgrade)),
      onRestSkip_(std::move(onRestSkip)),
      onBackToHub_(std::move(onBackToHub)) {}

void RunMapScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (overlayMode_ != OverlayMode::None) {
        updateOverlay(mouse);
        return;
    }

    if (restModalNodeId_.has_value()) {
        updateRestModal(mouse);
        return;
    }

    if (IsKeyPressed(KEY_D)) {
        openOverlay(OverlayMode::Deck);
        return;
    }

    if (IsKeyPressed(KEY_R)) {
        openOverlay(OverlayMode::Relics);
        return;
    }

    if (IsKeyPressed(KEY_P)) {
        openOverlay(OverlayMode::Consumables);
        return;
    }

    const Rectangle back{32.f, 32.f, 180.f, 48.f};
    if (BasicUi::contains(back, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBackToHub_();
        return;
    }

    if (BasicUi::contains(deckButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        openOverlay(OverlayMode::Deck);
        return;
    }

    if (BasicUi::contains(relicsButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        openOverlay(OverlayMode::Relics);
        return;
    }

    if (BasicUi::contains(consumablesButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        openOverlay(OverlayMode::Consumables);
        return;
    }

    for (const RunMapNode& node : runState_.map.nodes) {
        if (node.state != RunMapNodeState::Available) {
            continue;
        }

        if (!BasicUi::contains(nodeBounds(node), mouse) || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            continue;
        }

        if (node.type == RunMapNodeType::Rest) {
            restModalNodeId_ = node.id;
            return;
        }

        onNodeSelected_(node.id);
        return;
    }
}

void RunMapScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, Rectangle{32.f, 32.f, 180.f, 48.f}, localization_.get(TextId("run.back_to_hub")), mouse);
    BasicUi::drawButton(font_, deckButtonBounds(), localization_.get(TextId("run.view_deck")), mouse);
    BasicUi::drawButton(font_, relicsButtonBounds(), localization_.get(TextId("run.view_relics")), mouse);
    BasicUi::drawButton(font_, consumablesButtonBounds(), localization_.get(TextId("run.view_consumables")), mouse);

    const float summaryX = std::max(232.f, static_cast<float>(GetScreenWidth()) - 360.f);
    BasicUi::drawText(
        font_,
        runHpSummaryText(),
        Vector2{summaryX, 36.f},
        22.f,
        Color{235, 224, 185, 255}
    );
    BasicUi::drawText(
        font_,
        runStressSummaryText(),
        Vector2{summaryX, 62.f},
        22.f,
        Color{220, 185, 230, 255}
    );

    for (const RunMapNode& node : runState_.map.nodes) {
        const Vector2 from = nodeScreenPosition(node);

        for (const int nextNodeId : node.nextNodeIds) {
            const RunMapNode* target = findNodeById(runState_.map, nextNodeId);

            if (target == nullptr) {
                continue;
            }

            const Vector2 to = nodeScreenPosition(*target);
            DrawLineEx(from, to, 3.f, Color{70, 75, 92, 255});
        }
    }

    const RunMapNode* hoveredNode = nullptr;

    for (const RunMapNode& node : runState_.map.nodes) {
        const Rectangle bounds = nodeBounds(node);
        const bool isHovered = node.state == RunMapNodeState::Available &&
            !restModalNodeId_.has_value() &&
            BasicUi::contains(bounds, mouse);

        if (isHovered) {
            hoveredNode = &node;
        }

        DrawRectangleRounded(bounds, 0.3f, 16, nodeColor(node));
        DrawRectangleRoundedLinesEx(
            bounds,
            0.3f,
            16,
            isHovered ? 4.f : nodeOutlineThickness(node),
            isHovered ? Color{245, 230, 140, 255} : nodeOutlineColor(node)
        );

        BasicUi::drawCenteredText(font_, nodeLabel(node), bounds, 18.f, Color{240, 240, 250, 255});
    }

    if (hoveredNode != nullptr) {
        renderNodePreview(*hoveredNode);
    }

    if (restModalNodeId_.has_value()) {
        renderRestModal();
    }

    if (overlayMode_ != OverlayMode::None) {
        renderOverlay();
    }
}

Vector2 RunMapScene::nodeScreenPosition(const RunMapNode& node) const {
    const MapRawBounds rawBounds = calculateRawBounds(runState_.map);

    const float rawWidth = safeDimension(rawBounds.maxX - rawBounds.minX);
    const float rawHeight = safeDimension(rawBounds.maxY - rawBounds.minY);

    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());

    const float horizontalPadding = std::max(150.f, screenWidth * 0.08f);
    const float verticalPadding = std::max(110.f, screenHeight * 0.14f);

    const float availableWidth = std::max(1.f, screenWidth - horizontalPadding * 2.f);
    const float availableHeight = std::max(1.f, screenHeight - verticalPadding * 2.f);

    const float scale = std::min(availableWidth / rawWidth, availableHeight / rawHeight);

    const Vector2 rawCenter{
        (rawBounds.minX + rawBounds.maxX) * 0.5f,
        (rawBounds.minY + rawBounds.maxY) * 0.5f
    };

    const Vector2 screenCenter{
        screenWidth * 0.5f,
        screenHeight * 0.5f
    };

    return Vector2{
        screenCenter.x + (node.position.x - rawCenter.x) * scale,
        screenCenter.y + (node.position.y - rawCenter.y) * scale
    };
}

Rectangle RunMapScene::nodeBounds(const RunMapNode& node) const {
    const Vector2 position = nodeScreenPosition(node);
    return Rectangle{
        position.x - NODE_WIDTH * 0.5f,
        position.y - NODE_HEIGHT * 0.5f,
        NODE_WIDTH,
        NODE_HEIGHT
    };
}

Rectangle RunMapScene::restModalBounds() const {
    const float width = 520.f;
    const float height = 390.f;
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        (static_cast<float>(GetScreenHeight()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapScene::restHealButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 42.f, modal.y + 118.f, modal.width - 84.f, 52.f};
}

Rectangle RunMapScene::restUpgradeButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 42.f, modal.y + 184.f, modal.width - 84.f, 52.f};
}

Rectangle RunMapScene::restSkipButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 42.f, modal.y + 250.f, modal.width - 84.f, 46.f};
}

Rectangle RunMapScene::restCancelButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 150.f, modal.y + modal.height - 58.f, 112.f, 38.f};
}


Rectangle RunMapScene::deckButtonBounds() const {
    constexpr float buttonWidth = 126.f;
    constexpr float buttonHeight = 42.f;
    constexpr float gap = 10.f;
    constexpr float relicsWidth = 126.f;
    constexpr float consumablesWidth = 156.f;
    constexpr float totalWidth = buttonWidth + relicsWidth + consumablesWidth + gap * 2.f;
    const float startX = (static_cast<float>(GetScreenWidth()) - totalWidth) * 0.5f;
    return Rectangle{startX, 32.f, buttonWidth, buttonHeight};
}

Rectangle RunMapScene::relicsButtonBounds() const {
    constexpr float buttonWidth = 126.f;
    constexpr float buttonHeight = 42.f;
    constexpr float gap = 10.f;
    const Rectangle deck = deckButtonBounds();
    return Rectangle{deck.x + deck.width + gap, 32.f, buttonWidth, buttonHeight};
}

Rectangle RunMapScene::consumablesButtonBounds() const {
    constexpr float buttonWidth = 156.f;
    constexpr float buttonHeight = 42.f;
    constexpr float gap = 10.f;
    const Rectangle relics = relicsButtonBounds();
    return Rectangle{relics.x + relics.width + gap, 32.f, buttonWidth, buttonHeight};
}

Rectangle RunMapScene::overlayBounds() const {
    const float width = std::min(1180.f, static_cast<float>(GetScreenWidth()) - 56.f);
    const float height = std::min(680.f, static_cast<float>(GetScreenHeight()) - 56.f);
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        (static_cast<float>(GetScreenHeight()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapScene::overlayCloseButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 146.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle RunMapScene::overlayGridBounds(const Rectangle modal) const {
    if (overlayMode_ == OverlayMode::Upgrade) {
        return Rectangle{modal.x + 28.f, modal.y + 82.f, modal.width - 56.f, modal.height - 290.f};
    }

    return Rectangle{modal.x + 28.f, modal.y + 86.f, modal.width - 56.f, modal.height - 166.f};
}

Rectangle RunMapScene::upgradeConfirmButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 330.f, modal.y + modal.height - 58.f, 170.f, 40.f};
}

Rectangle RunMapScene::nodePreviewBounds(const RunMapNode& node) const {
    const Rectangle nodeBox = nodeBounds(node);
    const float width = 360.f;
    const float height = 178.f;
    float x = nodeBox.x + nodeBox.width + 18.f;
    float y = nodeBox.y + nodeBox.height * 0.5f - height * 0.5f;

    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());

    if (x + width > screenWidth - 18.f) {
        x = nodeBox.x - width - 18.f;
    }

    y = std::clamp(y, 18.f, screenHeight - height - 18.f);
    return Rectangle{x, y, width, height};
}

Color RunMapScene::nodeColor(const RunMapNode& node) const {
    if (node.state == RunMapNodeState::Completed) {
        return Color{55, 95, 72, 255};
    }

    if (node.state == RunMapNodeState::Current) {
        return Color{65, 88, 92, 255};
    }

    if (node.state == RunMapNodeState::Available) {
        return Color{85, 76, 112, 255};
    }

    return Color{38, 41, 52, 255};
}

Color RunMapScene::nodeOutlineColor(const RunMapNode& node) const {
    if (node.id == runState_.map.currentNodeId) {
        return Color{255, 218, 82, 255};
    }

    if (node.state == RunMapNodeState::Available) {
        return Color{170, 176, 212, 255};
    }

    return Color{105, 110, 136, 255};
}

float RunMapScene::nodeOutlineThickness(const RunMapNode& node) const {
    if (node.id == runState_.map.currentNodeId) {
        return 5.f;
    }

    return 2.f;
}

std::string RunMapScene::nodeLabel(const RunMapNode& node) const {
    switch (node.type) {
        case RunMapNodeType::Combat:
            return localization_.get(TextId("run.node.combat"));
        case RunMapNodeType::Elite:
            return localization_.get(TextId("run.node.elite"));
        case RunMapNodeType::Event:
            return localization_.get(TextId("run.node.event"));
        case RunMapNodeType::Shop:
            return localization_.get(TextId("run.node.shop"));
        case RunMapNodeType::Chest:
            return localization_.get(TextId("run.node.chest"));
        case RunMapNodeType::Rest:
            return localization_.get(TextId("run.node.rest"));
        case RunMapNodeType::Boss:
            return localization_.get(TextId("run.node.boss"));
    }

    return "?";
}

void RunMapScene::updateRestModal(const Vector2 mousePosition) {
    if (!restModalNodeId_.has_value()) {
        return;
    }

    const Rectangle modal = restModalBounds();

    if (IsKeyPressed(KEY_ESCAPE)) {
        restModalNodeId_ = std::nullopt;
        return;
    }

    if (BasicUi::contains(restHealButtonBounds(modal), mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const int nodeId = *restModalNodeId_;
        restModalNodeId_ = std::nullopt;
        onRestHeal_(nodeId);
        return;
    }

    if (BasicUi::contains(restUpgradeButtonBounds(modal), mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        openOverlay(OverlayMode::Upgrade);
        return;
    }

    if (BasicUi::contains(restSkipButtonBounds(modal), mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const int nodeId = *restModalNodeId_;
        restModalNodeId_ = std::nullopt;
        onRestSkip_(nodeId);
        return;
    }

    if (BasicUi::contains(restCancelButtonBounds(modal), mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        restModalNodeId_ = std::nullopt;
    }
}

void RunMapScene::renderRestModal() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = restModalBounds();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 150});
    DrawRectangleRounded(modal, 0.08f, 14, Color{28, 30, 38, 245});
    DrawRectangleRoundedLinesEx(modal, 0.08f, 14, 3.f, Color{220, 190, 105, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("rest.title")),
        Rectangle{modal.x, modal.y + 24.f, modal.width, 34.f},
        30.f,
        Color{245, 232, 180, 255}
    );

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("rest.description")),
        Rectangle{modal.x + 34.f, modal.y + 70.f, modal.width - 68.f, 32.f},
        17.f,
        Color{190, 194, 210, 255}
    );

    BasicUi::drawButton(font_, restHealButtonBounds(modal), localization_.get(TextId("rest.heal")), mouse);
    BasicUi::drawButton(font_, restUpgradeButtonBounds(modal), localization_.get(TextId("rest.upgrade")), mouse);
    BasicUi::drawButton(font_, restSkipButtonBounds(modal), localization_.get(TextId("rest.skip")), mouse);
    BasicUi::drawButton(font_, restCancelButtonBounds(modal), localization_.get(TextId("rest.back")), mouse);
}

void RunMapScene::renderNodePreview(const RunMapNode& node) const {
    const Rectangle panel = nodePreviewBounds(node);
    DrawRectangleRounded(panel, 0.08f, 12, Color{26, 28, 38, 245});
    DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 2.f, Color{238, 196, 86, 255});

    BasicUi::drawText(font_, nodePreviewTitle(node), Vector2{panel.x + 18.f, panel.y + 16.f}, 23.f, Color{244, 235, 188, 255});

    const std::vector<std::string> lines = BasicUi::wrapText(font_, nodePreviewDescription(node), 16.f, panel.width - 36.f);
    float y = panel.y + 55.f;
    for (const std::string& line : lines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 18.f, y}, 16.f, Color{204, 211, 230, 255});
        y += 21.f;
        if (y > panel.y + panel.height - 16.f) {
            break;
        }
    }
}

std::string RunMapScene::nodePreviewTitle(const RunMapNode& node) const {
    switch (node.type) {
        case RunMapNodeType::Combat:
            return localization_.get(TextId("run.preview.combat.title"));
        case RunMapNodeType::Elite:
            return localization_.get(TextId("run.preview.elite.title"));
        case RunMapNodeType::Event:
            return localization_.get(TextId("run.preview.event.title"));
        case RunMapNodeType::Shop:
            return localization_.get(TextId("run.preview.shop.title"));
        case RunMapNodeType::Chest:
            return localization_.get(TextId("run.preview.chest.title"));
        case RunMapNodeType::Rest:
            return localization_.get(TextId("run.preview.rest.title"));
        case RunMapNodeType::Boss:
            return localization_.get(TextId("run.preview.boss.title"));
    }

    return "?";
}

std::string RunMapScene::nodePreviewDescription(const RunMapNode& node) const {
    switch (node.type) {
        case RunMapNodeType::Combat:
            return localization_.get(TextId("run.preview.combat.description"));
        case RunMapNodeType::Elite:
            return localization_.get(TextId("run.preview.elite.description"));
        case RunMapNodeType::Event:
            return localization_.get(TextId("run.preview.event.description"));
        case RunMapNodeType::Shop:
            return localization_.get(TextId("run.preview.shop.description"));
        case RunMapNodeType::Chest:
            return localization_.get(TextId("run.preview.chest.description"));
        case RunMapNodeType::Rest:
            return localization_.get(TextId("run.preview.rest.description"));
        case RunMapNodeType::Boss:
            return localization_.get(TextId("run.preview.boss.description"));
    }

    return "?";
}


std::string RunMapScene::runHpSummaryText() const {
    int current = 0;
    int maximum = 0;

    for (const RunActorState& actor : runState_.actorStates) {
        current += std::clamp(actor.currentHp, 0, std::max(1, actor.maxHp));
        maximum += std::max(1, actor.maxHp);
    }

    if (maximum <= 0) {
        return localization_.format(TextId("run.hp_summary"), {{"current", "?"}, {"maximum", "?"}});
    }

    return localization_.format(
        TextId("run.hp_summary"),
        {{"current", std::to_string(current)}, {"maximum", std::to_string(maximum)}}
    );
}

std::string RunMapScene::runStressSummaryText() const {
    int current = 0;
    int maximum = 0;

    for (const RunActorState& actor : runState_.actorStates) {
        const int actorMaximum = std::max(1, actor.maxStress);
        current += std::clamp(actor.stress, 0, actorMaximum);
        maximum += actorMaximum;
    }

    if (maximum <= 0) {
        return localization_.format(TextId("run.stress_summary"), {{"current", "?"}, {"maximum", "?"}});
    }

    return localization_.format(
        TextId("run.stress_summary"),
        {{"current", std::to_string(current)}, {"maximum", std::to_string(maximum)}}
    );
}

void RunMapScene::openOverlay(const OverlayMode mode) {
    overlayMode_ = mode;
    overlayScrollOffset_ = 0.f;
    selectedUpgradeCardId_.reset();
    inspectedCardId_.reset();
    inspectedRelicId_.reset();
    inspectedConsumableId_.reset();
}

void RunMapScene::closeOverlay() {
    overlayMode_ = OverlayMode::None;
    overlayScrollOffset_ = 0.f;
    selectedUpgradeCardId_.reset();
    inspectedCardId_.reset();
    inspectedRelicId_.reset();
    inspectedConsumableId_.reset();
}

void RunMapScene::updateOverlay(const Vector2 mousePosition) {
    const Rectangle modal = overlayBounds();
    const Rectangle grid = overlayGridBounds(modal);

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.f) {
        std::size_t count = 0;
        float maxScroll = 0.f;
        if (overlayMode_ == OverlayMode::Deck) {
            count = runState_.deckCardIds.size();
            maxScroll = cardGridMaxScroll(grid, count);
        } else if (overlayMode_ == OverlayMode::Upgrade) {
            count = upgradableCards().size();
            maxScroll = cardGridMaxScroll(grid, count);
        } else if (overlayMode_ == OverlayMode::Relics) {
            count = runState_.relicIds.size();
            maxScroll = listMaxScroll(grid, count);
        } else if (overlayMode_ == OverlayMode::Consumables) {
            count = runState_.consumableIds.size();
            maxScroll = listMaxScroll(grid, count);
        }
        overlayScrollOffset_ = std::clamp(
            overlayScrollOffset_ - wheel * 76.f,
            0.f,
            maxScroll
        );
    }

    if (inspectedCardId_.has_value() || inspectedRelicId_.has_value() || inspectedConsumableId_.has_value()) {
        const Rectangle inspect = cardInspectModalBounds();
        if (IsKeyPressed(KEY_ESCAPE) ||
            (BasicUi::contains(cardInspectCloseButtonBounds(inspect), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
            inspectedCardId_.reset();
            inspectedRelicId_.reset();
            inspectedConsumableId_.reset();
            return;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !BasicUi::contains(inspect, mousePosition)) {
            inspectedCardId_.reset();
            inspectedRelicId_.reset();
            inspectedConsumableId_.reset();
            return;
        }

        return;
    }

    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(overlayCloseButtonBounds(modal), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        closeOverlay();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_I)) {
        inspectedCardId_ = hoveredOverlayCardId(mousePosition);
        if (inspectedCardId_.has_value()) {
            return;
        }

        inspectedRelicId_ = hoveredOverlayRelicId(mousePosition);
        if (inspectedRelicId_.has_value()) {
            return;
        }

        inspectedConsumableId_ = hoveredOverlayConsumableId(mousePosition);
        if (inspectedConsumableId_.has_value()) {
            return;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (overlayMode_ == OverlayMode::Relics) {
            inspectedRelicId_ = hoveredOverlayRelicId(mousePosition);
            if (inspectedRelicId_.has_value()) {
                return;
            }
        } else if (overlayMode_ == OverlayMode::Consumables) {
            inspectedConsumableId_ = hoveredOverlayConsumableId(mousePosition);
            if (inspectedConsumableId_.has_value()) {
                return;
            }
        }
    }

    if (overlayMode_ != OverlayMode::Upgrade || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    const std::vector<CardId> candidates = upgradableCards();
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (BasicUi::contains(cardGridCellBounds(grid, i, overlayScrollOffset_), mousePosition)) {
            selectedUpgradeCardId_ = candidates[i];
            return;
        }
    }

    if (selectedUpgradeCardId_.has_value() &&
        restModalNodeId_.has_value() &&
        BasicUi::contains(upgradeConfirmButtonBounds(modal), mousePosition)) {
        const int nodeId = *restModalNodeId_;
        const CardId cardId = *selectedUpgradeCardId_;
        closeOverlay();
        restModalNodeId_ = std::nullopt;
        onRestUpgrade_(nodeId, cardId);
    }
}

void RunMapScene::renderOverlay() const {
    const Rectangle modal = overlayBounds();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 165});
    DrawRectangleRounded(modal, 0.04f, 16, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(modal, 0.04f, 16, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        font_,
        overlayTitle(),
        Rectangle{modal.x + 24.f, modal.y + 20.f, modal.width - 48.f, 38.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    switch (overlayMode_) {
        case OverlayMode::Deck:
            renderDeckOverlay(modal);
            break;
        case OverlayMode::Upgrade:
            renderUpgradeOverlay(modal);
            break;
        case OverlayMode::Relics:
            renderRelicsOverlay(modal);
            break;
        case OverlayMode::Consumables:
            renderConsumablesOverlay(modal);
            break;
        case OverlayMode::None:
            break;
    }

    const Vector2 mouse = GetMousePosition();
    BasicUi::drawButton(font_, overlayCloseButtonBounds(modal), localization_.get(TextId("ui.close")), mouse);
    renderCardInspectModal();
    renderRelicInspectModal();
    renderConsumableInspectModal();
}

void RunMapScene::renderDeckOverlay(const Rectangle modal) const {
    const Rectangle grid = overlayGridBounds(modal);
    renderCardGrid(grid, runState_.deckCardIds, false);

    BasicUi::drawText(
        font_,
        localization_.format(TextId("run.deck_count"), {{"count", std::to_string(runState_.deckCardIds.size())}}),
        Vector2{modal.x + 32.f, modal.y + modal.height - 48.f},
        18.f,
        Color{190, 198, 220, 255}
    );
}

void RunMapScene::renderUpgradeOverlay(const Rectangle modal) const {
    const Rectangle grid = overlayGridBounds(modal);
    const std::vector<CardId> candidates = upgradableCards();
    renderCardGrid(grid, candidates, true);

    const Rectangle preview{modal.x + 30.f, modal.y + modal.height - 198.f, modal.width - 60.f, 122.f};
    DrawRectangleRounded(preview, 0.04f, 10, Color{33, 36, 48, 255});
    DrawRectangleRoundedLinesEx(preview, 0.04f, 10, 2.f, Color{110, 120, 150, 255});

    if (!selectedUpgradeCardId_.has_value()) {
        BasicUi::drawCenteredText(
            font_,
            candidates.empty() ? localization_.get(TextId("rest.no_upgradable_cards")) : localization_.get(TextId("rest.select_upgrade_card")),
            preview,
            20.f,
            Color{205, 210, 225, 255}
        );
    } else {
        const CardDefinition& base = cards_.get(*selectedUpgradeCardId_);
        const CardDefinition upgraded = CardUpgrade::upgradedDefinition(base);
        const CardDescriptionFormatter formatter(localization_);
        const float columnWidth = (preview.width - 54.f) * 0.5f;
        const Rectangle before{preview.x + 18.f, preview.y + 16.f, columnWidth, preview.height - 32.f};
        const Rectangle after{preview.x + 36.f + columnWidth, preview.y + 16.f, columnWidth, preview.height - 32.f};

        BasicUi::drawText(font_, localization_.get(base.nameTextId), Vector2{before.x, before.y}, 18.f, Color{235, 235, 242, 255});
        BasicUi::drawText(font_, localization_.get(upgraded.nameTextId) + "+", Vector2{after.x, after.y}, 18.f, Color{255, 232, 150, 255});

        const std::vector<std::string> beforeLines = BasicUi::wrapText(font_, formatter.formatStaticDescription(base), 14.f, before.width);
        const std::vector<std::string> afterLines = BasicUi::wrapText(font_, formatter.formatStaticDescription(upgraded), 14.f, after.width);
        float y = before.y + 28.f;
        for (const std::string& line : beforeLines) {
            if (y > before.y + before.height - 18.f) break;
            BasicUi::drawText(font_, line, Vector2{before.x, y}, 14.f, Color{190, 198, 220, 255});
            y += 18.f;
        }
        y = after.y + 28.f;
        for (const std::string& line : afterLines) {
            if (y > after.y + after.height - 18.f) break;
            BasicUi::drawText(font_, line, Vector2{after.x, y}, 14.f, Color{225, 218, 170, 255});
            y += 18.f;
        }
    }

    BasicUi::drawButton(
        font_,
        upgradeConfirmButtonBounds(modal),
        localization_.get(TextId("rest.confirm_upgrade")),
        GetMousePosition(),
        selectedUpgradeCardId_.has_value()
    );
}

void RunMapScene::renderRelicsOverlay(const Rectangle modal) const {
    const Rectangle area = overlayGridBounds(modal);

    if (runState_.relicIds.empty()) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("run.no_relics")), area, 22.f, Color{205, 210, 225, 255});
        return;
    }

    DrawRectangleRounded(area, 0.02f, 8, Color{20, 22, 30, 255});
    BeginScissorMode(static_cast<int>(area.x), static_cast<int>(area.y), static_cast<int>(area.width), static_cast<int>(area.height));

    const Vector2 mouse = GetMousePosition();
    for (std::size_t i = 0; i < runState_.relicIds.size(); ++i) {
        const Rectangle row = listRowBounds(area, i, overlayScrollOffset_);
        if (row.y + row.height < area.y || row.y > area.y + area.height) {
            continue;
        }

        const std::string& relicId = runState_.relicIds[i];
        const bool hovered = BasicUi::contains(row, mouse);
        DrawRectangleRounded(row, 0.035f, 8, hovered ? Color{48, 51, 66, 255} : Color{38, 41, 54, 255});
        DrawRectangleRoundedLinesEx(row, 0.035f, 8, hovered ? 3.f : 2.f, hovered ? Color{238, 196, 86, 255} : Color{120, 130, 160, 255});

        std::string name = relicId;
        std::string description = relicId;
        std::string rarity;
        if (relics_.contains(RelicId(relicId))) {
            const RelicDefinition& relic = relics_.get(RelicId(relicId));
            name = localization_.get(relic.nameTextId);
            description = localization_.get(relic.descriptionTextId);
            rarity = relicRarityText(relic.rarity);
        }

        BasicUi::drawText(font_, name, Vector2{row.x + 18.f, row.y + 12.f}, 21.f, Color{245, 235, 190, 255});
        if (!rarity.empty()) {
            BasicUi::drawText(font_, rarity, Vector2{row.x + row.width - 170.f, row.y + 15.f}, 15.f, Color{205, 212, 230, 255});
        }

        const std::vector<std::string> lines = BasicUi::wrapText(font_, description, 15.f, row.width - 36.f);
        float y = row.y + 43.f;
        for (const std::string& line : lines) {
            if (y > row.y + row.height - 15.f) {
                break;
            }
            BasicUi::drawText(font_, line, Vector2{row.x + 18.f, y}, 15.f, Color{190, 198, 220, 255});
            y += 18.f;
        }
    }

    EndScissorMode();

    BasicUi::drawText(
        font_,
        localization_.format(TextId("run.relic_count"), {{"count", std::to_string(runState_.relicIds.size())}}),
        Vector2{modal.x + 32.f, modal.y + modal.height - 48.f},
        18.f,
        Color{190, 198, 220, 255}
    );
}

void RunMapScene::renderConsumablesOverlay(const Rectangle modal) const {
    const Rectangle area = overlayGridBounds(modal);

    if (runState_.consumableIds.empty()) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("run.no_consumables")), area, 22.f, Color{205, 210, 225, 255});
        return;
    }

    DrawRectangleRounded(area, 0.02f, 8, Color{20, 22, 30, 255});
    BeginScissorMode(static_cast<int>(area.x), static_cast<int>(area.y), static_cast<int>(area.width), static_cast<int>(area.height));

    const Vector2 mouse = GetMousePosition();
    for (std::size_t i = 0; i < runState_.consumableIds.size(); ++i) {
        const Rectangle row = listRowBounds(area, i, overlayScrollOffset_);
        if (row.y + row.height < area.y || row.y > area.y + area.height) {
            continue;
        }

        const std::string& consumableId = runState_.consumableIds[i];
        const bool hovered = BasicUi::contains(row, mouse);
        DrawRectangleRounded(row, 0.035f, 8, hovered ? Color{44, 56, 72, 255} : Color{34, 44, 58, 255});
        DrawRectangleRoundedLinesEx(row, 0.035f, 8, hovered ? 3.f : 2.f, hovered ? Color{128, 190, 230, 255} : Color{105, 150, 190, 255});

        std::string name = consumableId;
        std::string description = consumableId;
        std::string rarity;
        std::string cost;
        if (consumables_.contains(ConsumableId(consumableId))) {
            const ConsumableDefinition& consumable = consumables_.get(ConsumableId(consumableId));
            name = localization_.get(consumable.nameTextId);
            description = localization_.get(consumable.descriptionTextId);
            rarity = consumableRarityText(consumable.rarity);
            cost = localization_.format(TextId("inspect.consumable.gold_cost.value"), {{"amount", std::to_string(consumable.goldCost)}});
        }

        BasicUi::drawText(font_, name, Vector2{row.x + 18.f, row.y + 12.f}, 21.f, Color{220, 240, 250, 255});
        if (!rarity.empty()) {
            BasicUi::drawText(font_, rarity, Vector2{row.x + row.width - 210.f, row.y + 15.f}, 15.f, Color{205, 212, 230, 255});
        }
        if (!cost.empty()) {
            BasicUi::drawText(font_, cost, Vector2{row.x + row.width - 96.f, row.y + 15.f}, 15.f, Color{235, 220, 140, 255});
        }

        const std::vector<std::string> lines = BasicUi::wrapText(font_, description, 15.f, row.width - 36.f);
        float y = row.y + 43.f;
        for (const std::string& line : lines) {
            if (y > row.y + row.height - 15.f) {
                break;
            }
            BasicUi::drawText(font_, line, Vector2{row.x + 18.f, y}, 15.f, Color{190, 205, 220, 255});
            y += 18.f;
        }
    }

    EndScissorMode();

    BasicUi::drawText(
        font_,
        localization_.format(TextId("run.consumable_count"), {{"count", std::to_string(runState_.consumableIds.size())}}),
        Vector2{modal.x + 32.f, modal.y + modal.height - 48.f},
        18.f,
        Color{190, 198, 220, 255}
    );
}

void RunMapScene::renderCardGrid(const Rectangle grid, const std::vector<CardId>& cardIds, const bool selectionMode) const {
    DrawRectangleRounded(grid, 0.02f, 8, Color{20, 22, 30, 255});
    BeginScissorMode(static_cast<int>(grid.x), static_cast<int>(grid.y), static_cast<int>(grid.width), static_cast<int>(grid.height));

    const Vector2 mouse = GetMousePosition();
    for (std::size_t i = 0; i < cardIds.size(); ++i) {
        const Rectangle cell = cardGridCellBounds(grid, i, overlayScrollOffset_);
        if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
            continue;
        }

        const bool upgraded = isCardUpgraded(cardIds[i]);
        const bool selected = selectionMode && selectedUpgradeCardId_.has_value() && *selectedUpgradeCardId_ == cardIds[i];
        const bool hovered = BasicUi::contains(cell, mouse);
        const Color fill = selected ? Color{68, 58, 38, 255} : (hovered ? Color{52, 56, 73, 255} : Color{39, 42, 55, 255});
        const Color border = selected ? Color{255, 218, 90, 255} : (hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255});
        DrawRectangleRounded(cell, 0.07f, 9, fill);
        DrawRectangleRoundedLinesEx(cell, 0.07f, 9, selected ? 4.f : 2.f, border);

        BasicUi::drawText(font_, std::to_string(cards_.contains(cardIds[i]) ? CardUpgrade::effectiveDefinition(cards_.get(cardIds[i]), upgraded).energyCost : 0), Vector2{cell.x + 13.f, cell.y + 10.f}, 18.f, Color{245, 245, 250, 255});
        BasicUi::drawCenteredText(font_, cardName(cardIds[i], upgraded), Rectangle{cell.x + 38.f, cell.y + 8.f, cell.width - 48.f, 40.f}, 16.f, Color{245, 245, 250, 255});

        const std::vector<std::string> lines = BasicUi::wrapText(font_, cardDescription(cardIds[i], upgraded), 12.f, cell.width - 22.f);
        float y = cell.y + 66.f;
        for (const std::string& line : lines) {
            if (y > cell.y + cell.height - 18.f) break;
            BasicUi::drawText(font_, line, Vector2{cell.x + 12.f, y}, 12.f, Color{205, 210, 225, 255});
            y += 16.f;
        }
    }

    EndScissorMode();
}

std::optional<CardId> RunMapScene::hoveredOverlayCardId(const Vector2 mousePosition) const {
    if (overlayMode_ != OverlayMode::Deck && overlayMode_ != OverlayMode::Upgrade) {
        return std::nullopt;
    }

    const Rectangle grid = overlayGridBounds(overlayBounds());
    const std::vector<CardId> cardIds = overlayMode_ == OverlayMode::Upgrade
        ? upgradableCards()
        : runState_.deckCardIds;

    for (std::size_t i = 0; i < cardIds.size(); ++i) {
        const Rectangle cell = cardGridCellBounds(grid, i, overlayScrollOffset_);
        if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
            continue;
        }

        if (BasicUi::contains(cell, mousePosition)) {
            return cardIds[i];
        }
    }

    return std::nullopt;
}

std::optional<std::string> RunMapScene::hoveredOverlayRelicId(const Vector2 mousePosition) const {
    if (overlayMode_ != OverlayMode::Relics) {
        return std::nullopt;
    }

    const Rectangle area = overlayGridBounds(overlayBounds());
    for (std::size_t i = 0; i < runState_.relicIds.size(); ++i) {
        const Rectangle row = listRowBounds(area, i, overlayScrollOffset_);
        if (row.y + row.height < area.y || row.y > area.y + area.height) {
            continue;
        }

        if (BasicUi::contains(row, mousePosition)) {
            return runState_.relicIds[i];
        }
    }

    return std::nullopt;
}

std::optional<std::string> RunMapScene::hoveredOverlayConsumableId(const Vector2 mousePosition) const {
    if (overlayMode_ != OverlayMode::Consumables) {
        return std::nullopt;
    }

    const Rectangle area = overlayGridBounds(overlayBounds());
    for (std::size_t i = 0; i < runState_.consumableIds.size(); ++i) {
        const Rectangle row = listRowBounds(area, i, overlayScrollOffset_);
        if (row.y + row.height < area.y || row.y > area.y + area.height) {
            continue;
        }

        if (BasicUi::contains(row, mousePosition)) {
            return runState_.consumableIds[i];
        }
    }

    return std::nullopt;
}

Rectangle RunMapScene::cardInspectModalBounds() const {
    const Rectangle overlay = overlayBounds();
    const float width = std::min(520.f, overlay.width - 96.f);
    const float height = std::min(600.f, overlay.height - 96.f);
    return Rectangle{
        overlay.x + overlay.width - width - 34.f,
        overlay.y + 72.f,
        width,
        height
    };
}

Rectangle RunMapScene::cardInspectCloseButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 128.f, modal.y + modal.height - 54.f, 104.f, 36.f};
}

std::string RunMapScene::cardInspectValueText(const EffectValue& value) const {
    if (value.isDice()) {
        return toString(value.diceExpression());
    }

    return std::to_string(value.fixedAmount());
}

std::string RunMapScene::cardInspectEffectText(const EffectDefinition& effect) const {
    std::ostringstream out;
    out << localizedOrFallback(TextId("inspect.effect." + toString(effect.type)), toString(effect.type));
    out << ": " << cardInspectValueText(effect.value);

    if (effect.repeatCount > 1) {
        out << " x" << effect.repeatCount;
    }

    if (effect.statusId.has_value()) {
        out << " " << localizedOrFallback(TextId("status." + *effect.statusId + ".name"), *effect.statusId);
    }

    out << " -> " << localizedOrFallback(TextId("inspect.target." + toString(effect.target)), toString(effect.target));
    return out.str();
}

void RunMapScene::renderCardInspectModal() const {
    if (!inspectedCardId_.has_value() || !cards_.contains(*inspectedCardId_)) {
        return;
    }

    const bool upgraded = isCardUpgraded(*inspectedCardId_);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(cards_.get(*inspectedCardId_), upgraded);
    const CardDescriptionFormatter formatter(localization_);
    const Rectangle modal = cardInspectModalBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangleRounded(modal, 0.045f, 14, Color{18, 20, 28, 248});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    float y = modal.y + 22.f;
    BasicUi::drawCenteredText(
        font_,
        cardName(*inspectedCardId_, upgraded),
        Rectangle{modal.x + 28.f, y, modal.width - 56.f, 34.f},
        25.f,
        Color{255, 235, 175, 255}
    );
    y += 48.f;

    const std::string meta =
        localizedOrFallback(TextId("inspect.card.cost.name"), "Cost") + ": " + std::to_string(definition.energyCost) +
        "   " + localizedOrFallback(TextId("inspect.card.type.name"), "Type") + ": " + localizedOrFallback(TextId("card.type." + toString(definition.type)), toString(definition.type)) +
        "   " + localizedOrFallback(TextId("inspect.card.rarity.name"), "Rarity") + ": " + localizedOrFallback(TextId("card.rarity." + toString(definition.rarity)), toString(definition.rarity));
    BasicUi::drawText(font_, meta, Vector2{modal.x + 28.f, y}, 15.f, Color{218, 222, 235, 255});
    y += 26.f;

    const std::string owner = definition.ownerActorId.empty()
        ? localizedOrFallback(TextId("ui.card_owner.common"), "Common card")
        : definition.ownerActorId;
    BasicUi::drawText(font_, localizedOrFallback(TextId("inspect.card.owner.name"), "Owner") + ": " + owner, Vector2{modal.x + 28.f, y}, 15.f, Color{205, 212, 230, 255});
    y += 24.f;

    BasicUi::drawText(
        font_,
        localizedOrFallback(TextId("inspect.card.upgrade.name"), "Upgrade") + ": " +
            (upgraded ? localizedOrFallback(TextId("inspect.card.upgrade.yes.short"), "upgraded") : localizedOrFallback(TextId("inspect.card.upgrade.no.short"), "not upgraded")),
        Vector2{modal.x + 28.f, y},
        15.f,
        Color{205, 212, 230, 255}
    );
    y += 30.f;

    const std::vector<std::string> descriptionLines = BasicUi::wrapText(font_, formatter.formatStaticDescription(definition), 16.f, modal.width - 56.f);
    for (const std::string& line : descriptionLines) {
        if (y > modal.y + modal.height - 118.f) break;
        BasicUi::drawText(font_, line, Vector2{modal.x + 28.f, y}, 16.f, Color{232, 232, 240, 255});
        y += 21.f;
    }
    y += 8.f;

    if (!definition.keywords.empty()) {
        BasicUi::drawText(font_, localizedOrFallback(TextId("inspect.card.keywords.name"), "Keywords"), Vector2{modal.x + 28.f, y}, 17.f, Color{245, 220, 140, 255});
        y += 24.f;
        for (const CardKeyword keyword : definition.keywords) {
            if (y > modal.y + modal.height - 118.f) break;
            const std::string key = "keyword." + toString(keyword) + ".name";
            BasicUi::drawText(font_, "• " + localizedOrFallback(TextId(key), toString(keyword)), Vector2{modal.x + 34.f, y}, 15.f, Color{210, 218, 235, 255});
            y += 20.f;
        }
        y += 6.f;
    }

    if (!definition.effects.empty()) {
        BasicUi::drawText(font_, localizedOrFallback(TextId("inspect.card.effects.name"), "Effects"), Vector2{modal.x + 28.f, y}, 17.f, Color{245, 220, 140, 255});
        y += 24.f;
        for (const EffectDefinition& effect : definition.effects) {
            if (y > modal.y + modal.height - 118.f) break;
            const std::vector<std::string> lines = BasicUi::wrapText(font_, "• " + cardInspectEffectText(effect), 14.f, modal.width - 68.f);
            for (const std::string& line : lines) {
                if (y > modal.y + modal.height - 118.f) break;
                BasicUi::drawText(font_, line, Vector2{modal.x + 34.f, y}, 14.f, Color{205, 212, 230, 255});
                y += 18.f;
            }
        }
    }

    BasicUi::drawButton(font_, cardInspectCloseButtonBounds(modal), localization_.get(TextId("ui.close")), mouse);
}

void RunMapScene::renderRelicInspectModal() const {
    if (!inspectedRelicId_.has_value()) {
        return;
    }

    const RelicId relicId(*inspectedRelicId_);
    if (!relics_.contains(relicId)) {
        return;
    }

    const RelicDefinition& relic = relics_.get(relicId);
    const Rectangle modal = cardInspectModalBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangleRounded(modal, 0.045f, 14, Color{18, 20, 28, 248});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    float y = modal.y + 22.f;
    BasicUi::drawCenteredText(
        font_,
        localization_.get(relic.nameTextId),
        Rectangle{modal.x + 28.f, y, modal.width - 56.f, 34.f},
        25.f,
        Color{255, 235, 175, 255}
    );
    y += 48.f;

    BasicUi::drawText(
        font_,
        localizedOrFallback(TextId("inspect.relic.rarity.name"), "Rarity") + ": " + relicRarityText(relic.rarity),
        Vector2{modal.x + 28.f, y},
        15.f,
        Color{218, 222, 235, 255}
    );
    y += 26.f;

    if (!relic.mechanicId.empty() && relic.mechanicId != "default") {
        BasicUi::drawText(
            font_,
            localizedOrFallback(TextId("inspect.relic.mechanic.name"), "Mechanic") + ": " + relic.mechanicId,
            Vector2{modal.x + 28.f, y},
            15.f,
            Color{205, 212, 230, 255}
        );
        y += 24.f;
    }

    const std::vector<std::string> descriptionLines = BasicUi::wrapText(font_, localization_.get(relic.descriptionTextId), 16.f, modal.width - 56.f);
    for (const std::string& line : descriptionLines) {
        if (y > modal.y + modal.height - 118.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{modal.x + 28.f, y}, 16.f, Color{232, 232, 240, 255});
        y += 21.f;
    }
    y += 10.f;

    if (!relic.modifiers.empty()) {
        BasicUi::drawText(font_, localizedOrFallback(TextId("inspect.relic.modifiers.name"), "Modifiers"), Vector2{modal.x + 28.f, y}, 17.f, Color{245, 220, 140, 255});
        y += 24.f;

        for (const RelicModifierDefinition& modifier : relic.modifiers) {
            const std::vector<std::string> lines = BasicUi::wrapText(font_, "• " + relicModifierText(modifier), 14.f, modal.width - 68.f);
            for (const std::string& line : lines) {
                if (y > modal.y + modal.height - 118.f) {
                    break;
                }
                BasicUi::drawText(font_, line, Vector2{modal.x + 34.f, y}, 14.f, Color{205, 212, 230, 255});
                y += 18.f;
            }
        }
        y += 6.f;
    }

    if (!relic.triggers.empty()) {
        BasicUi::drawText(font_, localizedOrFallback(TextId("inspect.relic.triggers.name"), "Triggers"), Vector2{modal.x + 28.f, y}, 17.f, Color{245, 220, 140, 255});
        y += 24.f;

        for (const RelicTriggerDefinition& trigger : relic.triggers) {
            const std::vector<std::string> lines = BasicUi::wrapText(font_, "• " + relicTriggerText(trigger), 14.f, modal.width - 68.f);
            for (const std::string& line : lines) {
                if (y > modal.y + modal.height - 118.f) {
                    break;
                }
                BasicUi::drawText(font_, line, Vector2{modal.x + 34.f, y}, 14.f, Color{205, 212, 230, 255});
                y += 18.f;
            }
        }
    }

    if (relic.modifiers.empty() && relic.triggers.empty()) {
        BasicUi::drawText(
            font_,
            localizedOrFallback(TextId("inspect.relic.passive_only"), "This relic has no parsed modifiers or triggers yet."),
            Vector2{modal.x + 28.f, y},
            15.f,
            Color{205, 212, 230, 255}
        );
    }

    BasicUi::drawButton(font_, cardInspectCloseButtonBounds(modal), localization_.get(TextId("ui.close")), mouse);
}

void RunMapScene::renderConsumableInspectModal() const {
    if (!inspectedConsumableId_.has_value()) {
        return;
    }

    const ConsumableId consumableId(*inspectedConsumableId_);
    if (!consumables_.contains(consumableId)) {
        return;
    }

    const ConsumableDefinition& consumable = consumables_.get(consumableId);
    const Rectangle modal = cardInspectModalBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangleRounded(modal, 0.045f, 14, Color{18, 20, 28, 248});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{128, 190, 230, 255});

    float y = modal.y + 22.f;
    BasicUi::drawCenteredText(
        font_,
        localization_.get(consumable.nameTextId),
        Rectangle{modal.x + 28.f, y, modal.width - 56.f, 34.f},
        25.f,
        Color{220, 240, 250, 255}
    );
    y += 48.f;

    BasicUi::drawText(
        font_,
        localizedOrFallback(TextId("inspect.consumable.rarity.name"), "Rarity") + ": " + consumableRarityText(consumable.rarity),
        Vector2{modal.x + 28.f, y},
        15.f,
        Color{218, 222, 235, 255}
    );
    y += 24.f;

    BasicUi::drawText(
        font_,
        localizedOrFallback(TextId("inspect.consumable.gold_cost.name"), "Gold cost") + ": " + std::to_string(consumable.goldCost),
        Vector2{modal.x + 28.f, y},
        15.f,
        Color{235, 220, 140, 255}
    );
    y += 30.f;

    const std::vector<std::string> descriptionLines = BasicUi::wrapText(font_, localization_.get(consumable.descriptionTextId), 16.f, modal.width - 56.f);
    for (const std::string& line : descriptionLines) {
        if (y > modal.y + modal.height - 118.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{modal.x + 28.f, y}, 16.f, Color{232, 232, 240, 255});
        y += 21.f;
    }
    y += 10.f;

    if (!consumable.effects.empty()) {
        BasicUi::drawText(font_, localizedOrFallback(TextId("inspect.consumable.effects.name"), "Effects"), Vector2{modal.x + 28.f, y}, 17.f, Color{245, 220, 140, 255});
        y += 24.f;

        for (const EffectDefinition& effect : consumable.effects) {
            const std::vector<std::string> lines = BasicUi::wrapText(font_, "• " + cardInspectEffectText(effect), 14.f, modal.width - 68.f);
            for (const std::string& line : lines) {
                if (y > modal.y + modal.height - 118.f) {
                    break;
                }
                BasicUi::drawText(font_, line, Vector2{modal.x + 34.f, y}, 14.f, Color{205, 212, 230, 255});
                y += 18.f;
            }
        }
    }

    BasicUi::drawButton(font_, cardInspectCloseButtonBounds(modal), localization_.get(TextId("ui.close")), mouse);
}


std::string RunMapScene::localizedOrFallback(const TextId& textId, const std::string&) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return textId.value;
}

Rectangle RunMapScene::cardGridCellBounds(const Rectangle grid, const std::size_t index, const float scrollOffset) const {
    constexpr int columns = 5;
    constexpr float gap = 14.f;
    const float width = (grid.width - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float height = 210.f;
    const int column = static_cast<int>(index % columns);
    const int row = static_cast<int>(index / columns);
    return Rectangle{
        grid.x + static_cast<float>(column) * (width + gap),
        grid.y + static_cast<float>(row) * (height + gap) - scrollOffset,
        width,
        height
    };
}

float RunMapScene::cardGridMaxScroll(const Rectangle grid, const std::size_t count) const {
    if (count == 0) {
        return 0.f;
    }

    constexpr int columns = 5;
    constexpr float gap = 14.f;
    constexpr float height = 210.f;
    const std::size_t rows = (count + columns - 1) / columns;
    const float totalHeight = static_cast<float>(rows) * height + static_cast<float>(rows > 0 ? rows - 1 : 0) * gap;
    return std::max(0.f, totalHeight - grid.height);
}

Rectangle RunMapScene::listRowBounds(const Rectangle area, const std::size_t index, const float scrollOffset) const {
    constexpr float gap = 10.f;
    constexpr float height = 94.f;
    return Rectangle{
        area.x + 12.f,
        area.y + 12.f + static_cast<float>(index) * (height + gap) - scrollOffset,
        area.width - 24.f,
        height
    };
}

float RunMapScene::listMaxScroll(const Rectangle area, const std::size_t count) const {
    if (count == 0) {
        return 0.f;
    }

    constexpr float gap = 10.f;
    constexpr float height = 94.f;
    const float totalHeight = 24.f + static_cast<float>(count) * height + static_cast<float>(count > 0 ? count - 1 : 0) * gap;
    return std::max(0.f, totalHeight - area.height);
}

bool RunMapScene::isCardUpgraded(const CardId& cardId) const {
    return std::find(runState_.upgradedCardIds.begin(), runState_.upgradedCardIds.end(), cardId) != runState_.upgradedCardIds.end();
}

std::vector<CardId> RunMapScene::upgradableCards() const {
    std::vector<CardId> result;
    std::set<std::string> seen;

    for (const CardId& cardId : runState_.deckCardIds) {
        if (seen.count(cardId.value) != 0 || isCardUpgraded(cardId) || !cards_.contains(cardId)) {
            continue;
        }

        const CardDefinition& definition = cards_.get(cardId);
        if (!CardUpgrade::isUpgradable(definition)) {
            continue;
        }

        seen.insert(cardId.value);
        result.push_back(cardId);
    }

    return result;
}

std::string RunMapScene::cardName(const CardId& cardId, const bool upgraded) const {
    if (!cards_.contains(cardId)) {
        return cardId.value;
    }

    const CardDefinition definition = CardUpgrade::effectiveDefinition(cards_.get(cardId), upgraded);
    return localization_.get(definition.nameTextId) + (upgraded ? "+" : "");
}

std::string RunMapScene::cardDescription(const CardId& cardId, const bool upgraded) const {
    if (!cards_.contains(cardId)) {
        return cardId.value;
    }

    const CardDescriptionFormatter formatter(localization_);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(cards_.get(cardId), upgraded);
    return formatter.formatStaticDescription(definition);
}


std::string RunMapScene::relicRarityText(const RelicRarity rarity) const {
    return localizedOrFallback(TextId("relic.rarity." + toString(rarity)), toString(rarity));
}

std::string RunMapScene::consumableRarityText(const ConsumableRarity rarity) const {
    return localizedOrFallback(TextId("consumable.rarity." + toString(rarity)), toString(rarity));
}

std::string RunMapScene::relicModifierText(const RelicModifierDefinition& modifier) const {
    std::ostringstream out;
    out << localizedOrFallback(TextId("relic.modifier." + toString(modifier.type)), toString(modifier.type));

    if (modifier.amount != 0) {
        out << " " << (modifier.amount > 0 ? "+" : "") << modifier.amount;
    }

    if (modifier.multiplier != 1.0) {
        out << " x" << modifier.multiplier;
    }

    out << " · ";
    out << (modifier.playerOnly
        ? localizedOrFallback(TextId("inspect.relic.player_only"), "player only")
        : localizedOrFallback(TextId("inspect.relic.affects_all"), "affects all"));

    return out.str();
}

std::string RunMapScene::relicTriggerText(const RelicTriggerDefinition& trigger) const {
    std::ostringstream out;
    out << localizedOrFallback(TextId("game_event." + toString(trigger.eventType)), toString(trigger.eventType));

    if (trigger.everyNTurns > 0) {
        out << " · " << localization_.format(
            TextId("inspect.relic.every_n_turns"),
            {{"count", std::to_string(trigger.everyNTurns)}}
        );
    }

    if (trigger.oncePerCombat) {
        out << " · " << localizedOrFallback(TextId("inspect.relic.once_per_combat"), "once per combat");
    }

    if (!trigger.effects.empty()) {
        out << " · " << localizedOrFallback(TextId("inspect.relic.effects.name"), "Effects") << ": ";
        for (std::size_t i = 0; i < trigger.effects.size(); ++i) {
            if (i > 0) {
                out << "; ";
            }
            out << cardInspectEffectText(trigger.effects[i]);
        }
    }

    return out.str();
}


std::string RunMapScene::overlayTitle() const {
    switch (overlayMode_) {
        case OverlayMode::Deck:
            return localization_.get(TextId("run.deck_title"));
        case OverlayMode::Relics:
            return localization_.get(TextId("run.relics_title"));
        case OverlayMode::Consumables:
            return localization_.get(TextId("run.consumables_title"));
        case OverlayMode::Upgrade:
            return localization_.get(TextId("rest.upgrade_title"));
        case OverlayMode::None:
            return {};
    }

    return {};
}
