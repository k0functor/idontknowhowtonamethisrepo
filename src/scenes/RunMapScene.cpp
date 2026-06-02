#include "RunMapScene.hpp"
#include "ui/VirtualViewport.hpp"
#include "ui/CardTransform.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/CardViewModelFactory.hpp"
#include "ui/CardVisualInstance.hpp"

#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "localization/TextFormatter.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/BasicUi.hpp"
#include "ui/CardTransform.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/CardViewModelFactory.hpp"
#include "ui/CardVisualInstance.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <iterator>
#include <set>
#include <sstream>
#include <utility>

#include <raylib.h>

namespace {
constexpr float NODE_WIDTH = 108.f;
constexpr float NODE_HEIGHT = 64.f;
constexpr float CARD_GRID_GAP = 20.f;
constexpr int CARD_GRID_MAX_COLUMNS = 5;

Vector2 standardCardSlotSize() {
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    return Vector2{cardSize.x + 18.f, cardSize.y + 26.f};
}

int cardGridColumns(const float gridWidth) {
    const Vector2 slotSize = standardCardSlotSize();
    const int fitting = static_cast<int>((gridWidth + CARD_GRID_GAP) / (slotSize.x + CARD_GRID_GAP));
    return std::max(1, std::min(CARD_GRID_MAX_COLUMNS, fitting));
}

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

std::vector<std::size_t> allDeckIndices(const RunState& runState) {
    std::vector<std::size_t> result;
    result.reserve(runState.deckCardIds.size());

    for (std::size_t index = 0; index < runState.deckCardIds.size(); ++index) {
        result.push_back(index);
    }

    return result;
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
    std::function<void(int, std::size_t)> onRestUpgrade,
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

    const Rectangle back{32.f, 32.f, 230.f, 48.f};
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
        if (!isSelectableMapNode(node)) {
            continue;
        }

        if (!BasicUi::contains(nodeBounds(node), mouse) || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            continue;
        }

        if (node.type == RunMapNodeType::Rest) {
            if (runState_.archetypeMechanicId == "merchant_progression") {
                onNodeSelected_(node.id);
                return;
            }

            restModalNodeId_ = node.id;
            return;
        }

        onNodeSelected_(node.id);
        return;
    }
}

void RunMapScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, Rectangle{32.f, 32.f, 230.f, 48.f}, localization_.get(TextId("settings.save_and_exit")), mouse);
    BasicUi::drawButton(font_, deckButtonBounds(), localization_.get(TextId("run.view_deck")), mouse);
    BasicUi::drawButton(font_, relicsButtonBounds(), localization_.get(TextId("run.view_relics")), mouse);
    BasicUi::drawButton(font_, consumablesButtonBounds(), localization_.get(TextId("run.view_consumables")), mouse);

    const float summaryX = std::max(232.f, static_cast<float>(VirtualViewport::width()) - 360.f);
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

    const RunMapNode* hoveredNode = hoveredMapNode(mouse);

    for (const RunMapNode& node : runState_.map.nodes) {
        const Vector2 from = nodeScreenPosition(node);

        for (const int nextNodeId : node.nextNodeIds) {
            const RunMapNode* target = findNodeById(runState_.map, nextNodeId);

            if (target == nullptr) {
                continue;
            }

            const Vector2 to = nodeScreenPosition(*target);
            const bool hoveredConnection = hoveredNode != nullptr &&
                (hoveredNode->id == node.id || hoveredNode->id == target->id);
            const float thickness = connectionThickness(node, *target) + (hoveredConnection ? 1.25f : 0.f);
            DrawLineEx(from, to, thickness, connectionColor(node, *target));
        }
    }

    for (const RunMapNode& node : runState_.map.nodes) {
        const Rectangle bounds = nodeBounds(node);
        const bool isHovered = hoveredNode != nullptr && hoveredNode->id == node.id;

        DrawRectangleRounded(bounds, 0.3f, 16, nodeColor(node));
        DrawRectangleRoundedLinesEx(
            bounds,
            0.3f,
            16,
            isHovered ? 4.f : nodeOutlineThickness(node),
            isHovered ? Color{245, 230, 140, 255} : nodeOutlineColor(node)
        );

        BasicUi::drawCenteredText(font_, nodeLabel(node), bounds, 18.f, nodeTextColor(node));
    }

    renderMapLegend();


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

    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

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
    const float width = std::min(660.f, static_cast<float>(VirtualViewport::width()) - 48.f);
    const float height = std::min(500.f, static_cast<float>(VirtualViewport::height()) - 48.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapScene::restHealButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 42.f, modal.y + modal.height - 214.f, modal.width - 84.f, 52.f};
}

Rectangle RunMapScene::restUpgradeButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 42.f, modal.y + modal.height - 150.f, modal.width - 84.f, 52.f};
}

Rectangle RunMapScene::restSkipButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 42.f, modal.y + modal.height - 88.f, modal.width - 220.f, 46.f};
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
    const float startX = (static_cast<float>(VirtualViewport::width()) - totalWidth) * 0.5f;
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
    const float width = std::min(1180.f, static_cast<float>(VirtualViewport::width()) - 56.f);
    const float height = std::min(680.f, static_cast<float>(VirtualViewport::height()) - 56.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapScene::overlayCloseButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 146.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle RunMapScene::overlayGridBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 28.f, modal.y + 86.f, modal.width - 56.f, modal.height - 166.f};
}

Rectangle RunMapScene::upgradePreviewModalBounds() const {
    const float width = 760.f;
    const float height = 520.f;
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapScene::upgradePreviewBeforeCardBounds(const Rectangle modal) const {
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    return Rectangle{modal.x + 72.f, modal.y + 104.f, cardSize.x, cardSize.y};
}

Rectangle RunMapScene::upgradePreviewAfterCardBounds(const Rectangle modal) const {
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    return Rectangle{modal.x + modal.width - 72.f - cardSize.x, modal.y + 104.f, cardSize.x, cardSize.y};
}

Rectangle RunMapScene::upgradePreviewCancelButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f - 224.f, modal.y + modal.height - 72.f, 192.f, 44.f};
}

Rectangle RunMapScene::upgradePreviewConfirmButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f + 32.f, modal.y + modal.height - 72.f, 192.f, 44.f};
}

const RunMapNode* RunMapScene::hoveredMapNode(const Vector2 mousePosition) const {
    if (isMapInteractionBlocked()) {
        return nullptr;
    }

    for (const RunMapNode& node : runState_.map.nodes) {
        if (BasicUi::contains(nodeBounds(node), mousePosition)) {
            return &node;
        }
    }

    return nullptr;
}

const RunMapNode* RunMapScene::currentMapNode() const {
    if (runState_.map.currentNodeId < 0) {
        return nullptr;
    }

    return findNodeById(runState_.map, runState_.map.currentNodeId);
}

bool RunMapScene::isMapInteractionBlocked() const {
    return overlayMode_ != OverlayMode::None || restModalNodeId_.has_value();
}

bool RunMapScene::isSelectableMapNode(const RunMapNode& node) const {
    return node.state == RunMapNodeState::Available || node.state == RunMapNodeState::Current;
}

bool RunMapScene::isPastLockedAlternative(const RunMapNode& node) const {
    if (node.state != RunMapNodeState::Locked) {
        return false;
    }

    const RunMapNode* current = currentMapNode();
    if (current == nullptr) {
        return false;
    }

    return node.position.x <= current->position.x + 1.f;
}

bool RunMapScene::isNextAvailableConnection(const RunMapNode& from, const RunMapNode& to) const {
    if (to.state != RunMapNodeState::Available) {
        return false;
    }

    if (from.state == RunMapNodeState::Current) {
        return true;
    }

    return from.state == RunMapNodeState::Completed && from.id == runState_.map.currentNodeId;
}

bool RunMapScene::isChosenPathConnection(const RunMapNode& from, const RunMapNode& to) const {
    if (from.state != RunMapNodeState::Completed) {
        return false;
    }

    return to.state == RunMapNodeState::Completed || to.state == RunMapNodeState::Current;
}

Color RunMapScene::connectionColor(const RunMapNode& from, const RunMapNode& to) const {
    if (isChosenPathConnection(from, to)) {
        return Color{95, 180, 125, 255};
    }

    if (isNextAvailableConnection(from, to)) {
        return Color{238, 196, 86, 255};
    }

    if (from.state == RunMapNodeState::Available && to.state == RunMapNodeState::Locked) {
        return Color{92, 96, 124, 185};
    }

    if (isPastLockedAlternative(from) || isPastLockedAlternative(to)) {
        return Color{72, 56, 62, 120};
    }

    if (from.state == RunMapNodeState::Locked || to.state == RunMapNodeState::Locked) {
        return Color{54, 58, 74, 135};
    }

    return Color{78, 84, 108, 190};
}

float RunMapScene::connectionThickness(const RunMapNode& from, const RunMapNode& to) const {
    if (isChosenPathConnection(from, to) || isNextAvailableConnection(from, to)) {
        return 4.f;
    }

    if (from.state == RunMapNodeState::Available && to.state == RunMapNodeState::Locked) {
        return 3.f;
    }

    return 2.f;
}

Color RunMapScene::nodeColor(const RunMapNode& node) const {
    if (node.state == RunMapNodeState::Completed) {
        return Color{48, 102, 72, 255};
    }

    if (node.state == RunMapNodeState::Current) {
        return Color{92, 74, 45, 255};
    }

    if (node.state == RunMapNodeState::Available) {
        return Color{92, 78, 128, 255};
    }

    if (isPastLockedAlternative(node)) {
        return Color{48, 38, 44, 235};
    }

    return Color{34, 37, 48, 225};
}

Color RunMapScene::nodeOutlineColor(const RunMapNode& node) const {
    if (node.id == runState_.map.currentNodeId || node.state == RunMapNodeState::Current) {
        return Color{255, 218, 82, 255};
    }

    if (node.state == RunMapNodeState::Completed) {
        return Color{120, 208, 145, 255};
    }

    if (node.state == RunMapNodeState::Available) {
        return Color{225, 205, 116, 255};
    }

    if (isPastLockedAlternative(node)) {
        return Color{104, 72, 82, 210};
    }

    return Color{83, 88, 112, 170};
}

Color RunMapScene::nodeTextColor(const RunMapNode& node) const {
    if (node.state == RunMapNodeState::Locked && !isPastLockedAlternative(node)) {
        return Color{142, 148, 168, 235};
    }

    if (isPastLockedAlternative(node)) {
        return Color{150, 128, 135, 235};
    }

    return Color{245, 245, 250, 255};
}

float RunMapScene::nodeOutlineThickness(const RunMapNode& node) const {
    if (node.id == runState_.map.currentNodeId || node.state == RunMapNodeState::Current) {
        return 5.f;
    }

    if (node.state == RunMapNodeState::Available) {
        return 3.f;
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

void RunMapScene::renderMapLegend() const {
    const float width = 520.f;
    const float height = 112.f;
    const Rectangle panel{
        32.f,
        static_cast<float>(VirtualViewport::height()) - height - 24.f,
        std::min(width, static_cast<float>(VirtualViewport::width()) - 64.f),
        height
    };

    DrawRectangleRounded(panel, 0.08f, 12, Color{20, 22, 30, 205});
    DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 1.5f, Color{92, 98, 122, 180});

    BasicUi::drawText(
        font_,
        localization_.get(TextId("run.map_hint.hover")),
        Vector2{panel.x + 16.f, panel.y + 12.f},
        15.f,
        Color{196, 204, 224, 255}
    );
    BasicUi::drawText(
        font_,
        localization_.get(TextId("run.map_hint.available_path")),
        Vector2{panel.x + 16.f, panel.y + 33.f},
        15.f,
        Color{196, 204, 224, 255}
    );

    struct LegendItem {
        const char* key;
        Color fill;
        Color border;
    };

    const LegendItem items[] = {
        {"run.node_state.available", Color{92, 78, 128, 255}, Color{225, 205, 116, 255}},
        {"run.node_state.completed", Color{48, 102, 72, 255}, Color{120, 208, 145, 255}},
        {"run.node_state.current", Color{92, 74, 45, 255}, Color{255, 218, 82, 255}},
        {"run.node_state.blocked_alternative", Color{48, 38, 44, 235}, Color{104, 72, 82, 210}}
    };

    float x = panel.x + 16.f;
    const float y = panel.y + 72.f;
    for (const LegendItem& item : items) {
        const Rectangle swatch{x, y + 2.f, 16.f, 16.f};
        DrawRectangleRounded(swatch, 0.25f, 6, item.fill);
        DrawRectangleRoundedLinesEx(swatch, 0.25f, 6, 1.5f, item.border);
        BasicUi::drawText(
            font_,
            localization_.get(TextId(item.key)),
            Vector2{x + 23.f, y - 1.f},
            14.f,
            Color{210, 216, 232, 255}
        );
        x += 118.f;
    }
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

    if (!upgradableDeckIndices().empty() &&
        BasicUi::contains(restUpgradeButtonBounds(modal), mousePosition) &&
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

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 150});
    DrawRectangleRounded(modal, 0.08f, 14, Color{28, 30, 38, 245});
    DrawRectangleRoundedLinesEx(modal, 0.08f, 14, 3.f, Color{220, 190, 105, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("rest.title")),
        Rectangle{modal.x, modal.y + 22.f, modal.width, 34.f},
        30.f,
        Color{245, 232, 180, 255}
    );

    const float textWidth = modal.width - 68.f;
    float y = modal.y + 72.f;
    const std::vector<std::string> descriptionLines = BasicUi::wrapText(
        font_,
        localization_.get(TextId("rest.description")),
        17.f,
        textWidth
    );
    for (const std::string& line : descriptionLines) {
        if (y > modal.y + 128.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{modal.x + 34.f, y}, 17.f, Color{190, 194, 210, 255});
        y += 22.f;
    }

    const Rectangle preview{modal.x + 34.f, modal.y + 132.f, modal.width - 68.f, 116.f};
    DrawRectangleRounded(preview, 0.055f, 10, Color{35, 38, 50, 255});
    DrawRectangleRoundedLinesEx(preview, 0.055f, 10, 2.f, Color{96, 108, 140, 210});

    BasicUi::drawText(
        font_,
        localization_.get(TextId("rest.heal_preview_title")),
        Vector2{preview.x + 18.f, preview.y + 14.f},
        18.f,
        Color{244, 226, 170, 255}
    );

    const std::vector<std::string> healLines = BasicUi::wrapText(font_, restHealPreviewText(), 16.f, preview.width - 36.f);
    y = preview.y + 42.f;
    for (const std::string& line : healLines) {
        if (y > preview.y + preview.height - 42.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{preview.x + 18.f, y}, 16.f, Color{214, 222, 238, 255});
        y += 20.f;
    }

    BasicUi::drawText(
        font_,
        restStressPreviewText(),
        Vector2{preview.x + 18.f, preview.y + preview.height - 28.f},
        16.f,
        Color{220, 190, 230, 255}
    );

    const std::vector<std::string> hintLines = BasicUi::wrapText(
        font_,
        localization_.get(TextId("rest.upgrade_preview_hint")),
        15.f,
        textWidth
    );
    y = preview.y + preview.height + 16.f;
    for (const std::string& line : hintLines) {
        if (y > restHealButtonBounds(modal).y - 10.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{modal.x + 42.f, y}, 15.f, Color{170, 178, 198, 255});
        y += 19.f;
    }

    BasicUi::drawButton(font_, restHealButtonBounds(modal), localization_.get(TextId("rest.heal")), mouse);
    BasicUi::drawButton(
        font_,
        restUpgradeButtonBounds(modal),
        localization_.get(TextId("rest.upgrade")),
        mouse,
        !upgradableDeckIndices().empty()
    );
    BasicUi::drawButton(font_, restSkipButtonBounds(modal), localization_.get(TextId("rest.skip")), mouse);
    BasicUi::drawButton(font_, restCancelButtonBounds(modal), localization_.get(TextId("rest.back")), mouse);
}

std::string RunMapScene::restHealPreviewText() const {
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
        {
            {"current", std::to_string(current)},
            {"after", std::to_string(after)},
            {"maximum", std::to_string(maximum)}
        }
    );
}

std::string RunMapScene::restStressPreviewText() const {
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
        {
            {"current", std::to_string(current)},
            {"after", std::to_string(after)},
            {"maximum", std::to_string(maximum)}
        }
    );
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
    selectedUpgradeDeckIndex_.reset();
    inspectedCardDeckIndex_.reset();
    inspectedRelicId_.reset();
    inspectedConsumableId_.reset();
}

void RunMapScene::closeOverlay() {
    overlayMode_ = OverlayMode::None;
    overlayScrollOffset_ = 0.f;
    selectedUpgradeDeckIndex_.reset();
    inspectedCardDeckIndex_.reset();
    inspectedRelicId_.reset();
    inspectedConsumableId_.reset();
}

void RunMapScene::updateOverlay(const Vector2 mousePosition) {
    const Rectangle modal = overlayBounds();
    const Rectangle grid = overlayGridBounds(modal);

    if (overlayMode_ == OverlayMode::Upgrade && selectedUpgradeDeckIndex_.has_value()) {
        updateUpgradePreviewModal(mousePosition);
        return;
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.f) {
        std::size_t count = 0;
        float maxScroll = 0.f;
        if (overlayMode_ == OverlayMode::Deck) {
            count = runState_.deckCardIds.size();
            maxScroll = cardGridMaxScroll(grid, count);
        } else if (overlayMode_ == OverlayMode::Upgrade) {
            count = upgradableDeckIndices().size();
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

    if (inspectedCardDeckIndex_.has_value() || inspectedRelicId_.has_value() || inspectedConsumableId_.has_value()) {
        const Rectangle inspect = cardInspectModalBounds();

        if (IsKeyPressed(KEY_ESCAPE)) {
            inspectedCardDeckIndex_.reset();
            inspectedRelicId_.reset();
            inspectedConsumableId_.reset();
            return;
        }

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            inspectPreviousOverlayItem();
            return;
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            inspectNextOverlayItem();
            return;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (BasicUi::contains(cardInspectCloseButtonBounds(inspect), mousePosition)) {
                inspectedCardDeckIndex_.reset();
                inspectedRelicId_.reset();
                inspectedConsumableId_.reset();
                return;
            }

            if (BasicUi::contains(cardInspectPreviousButtonBounds(inspect), mousePosition)) {
                inspectPreviousOverlayItem();
                return;
            }

            if (BasicUi::contains(cardInspectNextButtonBounds(inspect), mousePosition)) {
                inspectNextOverlayItem();
                return;
            }

            if (!BasicUi::contains(inspect, mousePosition)) {
                inspectedCardDeckIndex_.reset();
                inspectedRelicId_.reset();
                inspectedConsumableId_.reset();
                return;
            }
        }

        return;
    }

    if (IsKeyPressed(KEY_ESCAPE) ||
        (BasicUi::contains(overlayCloseButtonBounds(modal), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        closeOverlay();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_I)) {
        inspectedCardDeckIndex_ = hoveredOverlayDeckIndex(mousePosition);
        if (inspectedCardDeckIndex_.has_value()) {
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
        if (overlayMode_ == OverlayMode::Deck) {
            inspectedCardDeckIndex_ = hoveredOverlayDeckIndex(mousePosition);
            if (inspectedCardDeckIndex_.has_value()) {
                return;
            }
        } else if (overlayMode_ == OverlayMode::Relics) {
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

    const std::optional<std::size_t> hoveredDeckIndex = hoveredOverlayDeckIndex(mousePosition);
    if (hoveredDeckIndex.has_value() && canUpgradeDeckIndex(*hoveredDeckIndex)) {
        selectedUpgradeDeckIndex_ = *hoveredDeckIndex;
        return;
    }
}

void RunMapScene::updateUpgradePreviewModal(const Vector2 mousePosition) {
    const Rectangle modal = upgradePreviewModalBounds();

    if (IsKeyPressed(KEY_ESCAPE)) {
        selectedUpgradeDeckIndex_.reset();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    if (BasicUi::contains(upgradePreviewCancelButtonBounds(modal), mousePosition)) {
        selectedUpgradeDeckIndex_.reset();
        return;
    }

    if (selectedUpgradeDeckIndex_.has_value() &&
        restModalNodeId_.has_value() &&
        canUpgradeDeckIndex(*selectedUpgradeDeckIndex_) &&
        BasicUi::contains(upgradePreviewConfirmButtonBounds(modal), mousePosition)) {
        const int nodeId = *restModalNodeId_;
        const std::size_t deckIndex = *selectedUpgradeDeckIndex_;
        closeOverlay();
        restModalNodeId_ = std::nullopt;
        onRestUpgrade_(nodeId, deckIndex);
        return;
    }
}

void RunMapScene::renderOverlay() const {
    const Rectangle modal = overlayBounds();
    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 165});
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
    renderUpgradePreviewModal();
}

void RunMapScene::renderDeckOverlay(const Rectangle modal) const {
    const Rectangle grid = overlayGridBounds(modal);
    renderCardGrid(grid, allDeckIndices(runState_), false);

    renderOverlayFooterHint(
        modal,
        localization_.format(TextId("run.deck_count"), {{"count", std::to_string(runState_.deckCardIds.size())}})
    );
}

void RunMapScene::renderUpgradeOverlay(const Rectangle modal) const {
    const Rectangle grid = overlayGridBounds(modal);
    const std::vector<std::size_t> deckIndices = upgradableDeckIndices();
    renderCardGrid(grid, deckIndices, true);

    if (deckIndices.empty()) {
        BasicUi::drawCenteredText(
            font_,
            localization_.get(TextId("rest.no_upgradable_cards")),
            runState_.deckCardIds.empty() ? grid : Rectangle{grid.x + 20.f, grid.y + 20.f, grid.width - 40.f, 52.f},
            runState_.deckCardIds.empty() ? 22.f : 20.f,
            runState_.deckCardIds.empty() ? Color{205, 210, 225, 255} : Color{245, 190, 170, 255}
        );
    } else {
        const std::vector<std::string> hintLines = BasicUi::wrapText(
            font_,
            localization_.get(TextId("rest.select_upgrade_card")),
            15.f,
            modal.width - 64.f
        );
        float y = modal.y + 66.f;
        for (const std::string& line : hintLines) {
            if (y > grid.y - 8.f) {
                break;
            }
            BasicUi::drawCenteredText(
                font_,
                line,
                Rectangle{modal.x + 32.f, y, modal.width - 64.f, 18.f},
                15.f,
                Color{170, 178, 198, 255}
            );
            y += 18.f;
        }
    }

    renderOverlayFooterHint(
        modal,
        localization_.format(
            TextId("rest.upgradable_count"),
            {
                {"upgradable", std::to_string(deckIndices.size())},
                {"total", std::to_string(runState_.deckCardIds.size())}
            }
        )
    );
}

void RunMapScene::renderUpgradePreviewModal() const {
    if (overlayMode_ != OverlayMode::Upgrade || !selectedUpgradeDeckIndex_.has_value()) {
        return;
    }

    const std::size_t deckIndex = *selectedUpgradeDeckIndex_;
    if (!canUpgradeDeckIndex(deckIndex)) {
        return;
    }

    const CardId& selectedCardId = runState_.deckCardIds[deckIndex];
    if (!cards_.contains(selectedCardId)) {
        return;
    }

    const CardDefinition& base = cards_.get(selectedCardId);
    const CardDefinition upgraded = CardUpgrade::upgradedDefinition(base);
    const Rectangle modal = upgradePreviewModalBounds();
    const Rectangle beforeCardBounds = upgradePreviewBeforeCardBounds(modal);
    const Rectangle afterCardBounds = upgradePreviewAfterCardBounds(modal);
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 120});
    DrawRectangleRounded(modal, 0.045f, 14, Color{18, 20, 28, 252});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("rest.upgrade_title")),
        Rectangle{modal.x + 28.f, modal.y + 20.f, modal.width - 56.f, 36.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("rest.upgrade_before")),
        Rectangle{beforeCardBounds.x - 10.f, modal.y + 70.f, beforeCardBounds.width + 20.f, 26.f},
        19.f,
        Color{190, 198, 220, 255}
    );
    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("rest.upgrade_after")),
        Rectangle{afterCardBounds.x - 10.f, modal.y + 70.f, afterCardBounds.width + 20.f, 26.f},
        19.f,
        Color{245, 220, 140, 255}
    );

    const CardViewModel beforeModel = CardViewModelFactory::buildStatic(
        base,
        localization_,
        CardInstanceId{static_cast<std::uint64_t>(deckIndex + 1)},
        false,
        false
    );
    const CardViewModel afterModel = CardViewModelFactory::buildStatic(
        base,
        localization_,
        CardInstanceId{static_cast<std::uint64_t>(deckIndex + 1)},
        true,
        true
    );

    CardVisualInstance::renderStatic(
        beforeModel,
        font_.available() ? &font_.font() : nullptr,
        CardVisualInstance::transformForStandardSlot(beforeCardBounds, 0)
    );
    CardVisualInstance::renderStatic(
        afterModel,
        font_.available() ? &font_.font() : nullptr,
        CardVisualInstance::transformForStandardSlot(afterCardBounds, 1)
    );

    const std::string selectedCopyText = localization_.format(
        TextId("rest.upgrade_selected_copy"),
        {{"index", std::to_string(deckIndex + 1)}}
    );
    const std::vector<std::string> selectedCopyLines = BasicUi::wrapText(font_, selectedCopyText, 15.f, modal.width - 104.f);
    float y = modal.y + 338.f;
    for (const std::string& line : selectedCopyLines) {
        if (y > modal.y + 378.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{modal.x + 52.f, y}, 15.f, Color{170, 178, 198, 255});
        y += 18.f;
    }

    const std::string summary = CardUpgrade::summary(base, upgraded, localization_);
    const std::vector<std::string> summaryLines = BasicUi::wrapText(font_, summary, 16.f, modal.width - 104.f);
    y = modal.y + 390.f;
    BasicUi::drawText(font_, localization_.get(TextId("rest.upgrade_summary")), Vector2{modal.x + 52.f, y}, 18.f, Color{245, 220, 140, 255});
    y += 24.f;
    for (const std::string& line : summaryLines) {
        if (y > modal.y + modal.height - 88.f) {
            break;
        }
        BasicUi::drawText(font_, line, Vector2{modal.x + 52.f, y}, 16.f, Color{215, 222, 238, 255});
        y += 20.f;
    }

    BasicUi::drawButton(font_, upgradePreviewCancelButtonBounds(modal), localization_.get(TextId("ui.cancel")), mouse);
    BasicUi::drawButton(
        font_,
        upgradePreviewConfirmButtonBounds(modal),
        localization_.get(TextId("rest.confirm_upgrade")),
        mouse,
        canUpgradeDeckIndex(deckIndex)
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
        std::string rarity;
        if (relics_.contains(RelicId(relicId))) {
            const RelicDefinition& relic = relics_.get(RelicId(relicId));
            name = localization_.get(relic.nameTextId);
            rarity = relicRarityText(relic.rarity);
        }

        BasicUi::drawText(font_, name, Vector2{row.x + 18.f, row.y + 12.f}, 21.f, Color{245, 235, 190, 255});
        if (!rarity.empty()) {
            BasicUi::drawText(font_, rarity, Vector2{row.x + row.width - 170.f, row.y + 15.f}, 15.f, Color{205, 212, 230, 255});
        }
    }

    EndScissorMode();

    renderOverlayFooterHint(
        modal,
        localization_.format(TextId("run.relic_count"), {{"count", std::to_string(runState_.relicIds.size())}})
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

    renderOverlayFooterHint(
        modal,
        localization_.format(TextId("run.consumable_count"), {{"count", std::to_string(runState_.consumableIds.size())}})
    );
}

void RunMapScene::renderOverlayFooterHint(const Rectangle modal, const std::string& countText) const {
    BasicUi::drawText(
        font_,
        countText,
        Vector2{modal.x + 32.f, modal.y + modal.height - 48.f},
        18.f,
        Color{190, 198, 220, 255}
    );

    BasicUi::drawText(
        font_,
        localization_.get(TextId("run.inspect_hint")),
        Vector2{modal.x + 260.f, modal.y + modal.height - 46.f},
        15.f,
        Color{150, 160, 185, 255}
    );
}

void RunMapScene::renderCardGrid(const Rectangle grid, const std::vector<std::size_t>& deckIndices, const bool selectionMode) const {
    DrawRectangleRounded(grid, 0.02f, 8, Color{20, 22, 30, 255});
    BeginScissorMode(static_cast<int>(grid.x), static_cast<int>(grid.y), static_cast<int>(grid.width), static_cast<int>(grid.height));

    const Vector2 mouse = GetMousePosition();
    for (std::size_t i = 0; i < deckIndices.size(); ++i) {
        const std::size_t deckIndex = deckIndices[i];
        if (deckIndex >= runState_.deckCardIds.size()) {
            continue;
        }

        const CardId& cardId = runState_.deckCardIds[deckIndex];
        const Rectangle cell = cardGridCellBounds(grid, i, overlayScrollOffset_);
        if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
            continue;
        }

        if (!cards_.contains(cardId)) {
            BasicUi::drawCenteredText(font_, cardId.value, cell, 16.f, Color{245, 245, 250, 255});
            continue;
        }

        const bool upgraded = isDeckCardUpgraded(deckIndex);
        const bool selected = selectionMode && selectedUpgradeDeckIndex_.has_value() && *selectedUpgradeDeckIndex_ == deckIndex;
        const bool hovered = BasicUi::contains(cell, mouse);
        CardViewModel model = CardViewModelFactory::buildStatic(
            cards_.get(cardId),
            localization_,
            CardInstanceId{static_cast<std::uint64_t>(deckIndex + 1)},
            upgraded,
            selected || hovered
        );

        if (selectionMode && !canUpgradeDeckIndex(deckIndex)) {
            model.playable = false;
            model.unplayableReason = localization_.get(TextId("rest.card_not_upgradable"));
        }

        const CardTransform transform = CardVisualInstance::transformForStandardSlot(cell, static_cast<int>(i));
        CardVisualInstance::renderStatic(model, font_.available() ? &font_.font() : nullptr, transform);
    }

    EndScissorMode();
}


std::optional<std::size_t> RunMapScene::hoveredOverlayDeckIndex(const Vector2 mousePosition) const {
    if (overlayMode_ != OverlayMode::Deck && overlayMode_ != OverlayMode::Upgrade) {
        return std::nullopt;
    }

    const Rectangle grid = overlayGridBounds(overlayBounds());
    const std::vector<std::size_t> deckIndices = visibleOverlayDeckIndices();

    for (std::size_t i = 0; i < deckIndices.size(); ++i) {
        const Rectangle cell = cardGridCellBounds(grid, i, overlayScrollOffset_);
        if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
            continue;
        }

        if (BasicUi::contains(cell, mousePosition)) {
            return deckIndices[i];
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

Rectangle RunMapScene::cardInspectPreviousButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 24.f, modal.y + modal.height - 54.f, 52.f, 36.f};
}

Rectangle RunMapScene::cardInspectNextButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 86.f, modal.y + modal.height - 54.f, 52.f, 36.f};
}

std::string RunMapScene::cardInspectValueText(const EffectValue& value) const {
    if (value.isDice()) {
        return toString(value.diceExpression());
    }

    return std::to_string(value.fixedAmount());
}

std::string RunMapScene::cardInspectKeywordName(const CardKeyword keyword) const {
    return localizedOrFallback(TextId("keyword." + toString(keyword) + ".name"), toString(keyword));
}

std::string RunMapScene::cardInspectKeywordDescription(const CardKeyword keyword) const {
    return localizedOrFallback(TextId("keyword." + toString(keyword) + ".description"), localizedOrFallback(TextId("keyword.unknown.description"), toString(keyword)));
}

std::string RunMapScene::cardInspectEffectText(const EffectDefinition& effect) const {
    std::ostringstream out;
    out << localizedOrFallback(TextId("inspect.effect." + toString(effect.type)), toString(effect.type));
    out << ": " << cardInspectValueText(effect.value);

    if (effect.repeatCount > 1) {
        out << " ×" << effect.repeatCount;
    }

    if (effect.statusId.has_value()) {
        out << " " << localizedOrFallback(TextId("status." + *effect.statusId + ".name"), *effect.statusId);
    }

    out << " -> " << localizedOrFallback(TextId("inspect.target." + toString(effect.target)), toString(effect.target));
    return out.str();
}

void RunMapScene::renderCardInspectModal() const {
    if (!inspectedCardDeckIndex_.has_value() || *inspectedCardDeckIndex_ >= runState_.deckCardIds.size()) {
        return;
    }

    const CardId& inspectedCardId = runState_.deckCardIds[*inspectedCardDeckIndex_];
    if (!cards_.contains(inspectedCardId)) {
        return;
    }

    const bool upgraded = isDeckCardUpgraded(*inspectedCardDeckIndex_);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(cards_.get(inspectedCardId), upgraded);
    const CardDescriptionFormatter formatter(localization_);
    const Rectangle modal = cardInspectModalBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangleRounded(modal, 0.045f, 14, Color{18, 20, 28, 248});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    float y = modal.y + 22.f;
    BasicUi::drawCenteredText(
        font_,
        cardName(inspectedCardId, upgraded),
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

            const std::string keywordHeader = "• " + cardInspectKeywordName(keyword);
            BasicUi::drawText(font_, keywordHeader, Vector2{modal.x + 34.f, y}, 15.f, Color{210, 218, 235, 255});
            y += 19.f;

            const std::vector<std::string> keywordLines = BasicUi::wrapText(
                font_,
                cardInspectKeywordDescription(keyword),
                13.f,
                modal.width - 76.f
            );
            for (const std::string& line : keywordLines) {
                if (y > modal.y + modal.height - 118.f) break;
                BasicUi::drawText(font_, line, Vector2{modal.x + 48.f, y}, 13.f, Color{170, 180, 205, 255});
                y += 16.f;
            }
            y += 4.f;
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

    renderInspectModalControls(modal, inspectedOverlayItemIndex(), inspectedOverlayItemCount());
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

    renderInspectModalControls(modal, inspectedOverlayItemIndex(), inspectedOverlayItemCount());
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

    renderInspectModalControls(modal, inspectedOverlayItemIndex(), inspectedOverlayItemCount());
    BasicUi::drawButton(font_, cardInspectCloseButtonBounds(modal), localization_.get(TextId("ui.close")), mouse);
}


void RunMapScene::renderInspectModalControls(
    const Rectangle modal,
    const std::size_t itemIndex,
    const std::size_t itemCount
) const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle previous = cardInspectPreviousButtonBounds(modal);
    const Rectangle next = cardInspectNextButtonBounds(modal);

    BasicUi::drawButton(font_, previous, "<", mouse, itemCount > 1);
    BasicUi::drawButton(font_, next, ">", mouse, itemCount > 1);

    const std::string counter = itemCount > 0
        ? std::to_string(itemIndex + 1) + "/" + std::to_string(itemCount)
        : "0/0";
    BasicUi::drawCenteredText(
        font_,
        counter,
        Rectangle{previous.x + previous.width + 8.f, previous.y, next.x - previous.x - previous.width - 16.f, previous.height},
        15.f,
        Color{185, 190, 205, 255}
    );
}

std::size_t RunMapScene::inspectedOverlayItemIndex() const {
    if (inspectedCardDeckIndex_.has_value()) {
        const std::vector<std::size_t> deckIndices = visibleOverlayDeckIndices();
        const auto found = std::find(deckIndices.begin(), deckIndices.end(), *inspectedCardDeckIndex_);
        if (found != deckIndices.end()) {
            return static_cast<std::size_t>(std::distance(deckIndices.begin(), found));
        }
        return 0;
    }

    if (inspectedRelicId_.has_value()) {
        const auto found = std::find(runState_.relicIds.begin(), runState_.relicIds.end(), *inspectedRelicId_);
        if (found != runState_.relicIds.end()) {
            return static_cast<std::size_t>(std::distance(runState_.relicIds.begin(), found));
        }
        return 0;
    }

    if (inspectedConsumableId_.has_value()) {
        const auto found = std::find(runState_.consumableIds.begin(), runState_.consumableIds.end(), *inspectedConsumableId_);
        if (found != runState_.consumableIds.end()) {
            return static_cast<std::size_t>(std::distance(runState_.consumableIds.begin(), found));
        }
    }

    return 0;
}

std::size_t RunMapScene::inspectedOverlayItemCount() const {
    if (inspectedCardDeckIndex_.has_value()) {
        return visibleOverlayDeckIndices().size();
    }

    if (inspectedRelicId_.has_value()) {
        return runState_.relicIds.size();
    }

    if (inspectedConsumableId_.has_value()) {
        return runState_.consumableIds.size();
    }

    return 0;
}

void RunMapScene::inspectPreviousOverlayItem() {
    const std::size_t count = inspectedOverlayItemCount();
    if (count <= 1) {
        return;
    }

    const std::size_t current = inspectedOverlayItemIndex();
    const std::size_t previous = current == 0 ? count - 1 : current - 1;

    if (inspectedCardDeckIndex_.has_value()) {
        const std::vector<std::size_t> deckIndices = visibleOverlayDeckIndices();
        if (previous < deckIndices.size()) {
            inspectedCardDeckIndex_ = deckIndices[previous];
        }
        return;
    }

    if (inspectedRelicId_.has_value()) {
        if (previous < runState_.relicIds.size()) {
            inspectedRelicId_ = runState_.relicIds[previous];
        }
        return;
    }

    if (inspectedConsumableId_.has_value() && previous < runState_.consumableIds.size()) {
        inspectedConsumableId_ = runState_.consumableIds[previous];
    }
}

void RunMapScene::inspectNextOverlayItem() {
    const std::size_t count = inspectedOverlayItemCount();
    if (count <= 1) {
        return;
    }

    const std::size_t next = (inspectedOverlayItemIndex() + 1) % count;

    if (inspectedCardDeckIndex_.has_value()) {
        const std::vector<std::size_t> deckIndices = visibleOverlayDeckIndices();
        if (next < deckIndices.size()) {
            inspectedCardDeckIndex_ = deckIndices[next];
        }
        return;
    }

    if (inspectedRelicId_.has_value()) {
        if (next < runState_.relicIds.size()) {
            inspectedRelicId_ = runState_.relicIds[next];
        }
        return;
    }

    if (inspectedConsumableId_.has_value() && next < runState_.consumableIds.size()) {
        inspectedConsumableId_ = runState_.consumableIds[next];
    }
}


std::string RunMapScene::localizedOrFallback(const TextId& textId, const std::string& fallback) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return fallback;
}

Rectangle RunMapScene::cardGridCellBounds(const Rectangle grid, const std::size_t index, const float scrollOffset) const {
    const int columns = cardGridColumns(grid.width);
    const Vector2 slotSize = standardCardSlotSize();
    const float totalWidth = static_cast<float>(columns) * slotSize.x + static_cast<float>(columns - 1) * CARD_GRID_GAP;
    const float startX = grid.x + std::max(0.f, (grid.width - totalWidth) * 0.5f);
    const int column = static_cast<int>(index % static_cast<std::size_t>(columns));
    const int row = static_cast<int>(index / static_cast<std::size_t>(columns));
    return Rectangle{
        startX + static_cast<float>(column) * (slotSize.x + CARD_GRID_GAP),
        grid.y + 14.f + static_cast<float>(row) * (slotSize.y + CARD_GRID_GAP) - scrollOffset,
        slotSize.x,
        slotSize.y
    };
}

float RunMapScene::cardGridMaxScroll(const Rectangle grid, const std::size_t count) const {
    if (count == 0) {
        return 0.f;
    }

    const int columns = cardGridColumns(grid.width);
    const Vector2 slotSize = standardCardSlotSize();
    const std::size_t rows = (count + static_cast<std::size_t>(columns) - 1u) / static_cast<std::size_t>(columns);
    const float totalHeight = 28.f + static_cast<float>(rows) * slotSize.y + static_cast<float>(rows > 0 ? rows - 1 : 0) * CARD_GRID_GAP;
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

bool RunMapScene::isDeckCardUpgraded(const std::size_t deckIndex) const {
    if (deckIndex > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    const int value = static_cast<int>(deckIndex);
    return std::find(runState_.upgradedDeckIndices.begin(), runState_.upgradedDeckIndices.end(), value) != runState_.upgradedDeckIndices.end();
}

bool RunMapScene::canUpgradeDeckIndex(const std::size_t deckIndex) const {
    if (deckIndex >= runState_.deckCardIds.size() || isDeckCardUpgraded(deckIndex)) {
        return false;
    }

    const CardId& cardId = runState_.deckCardIds[deckIndex];
    return cards_.contains(cardId) && CardUpgrade::isUpgradable(cards_.get(cardId));
}

std::vector<std::size_t> RunMapScene::upgradableDeckIndices() const {
    std::vector<std::size_t> result;

    for (std::size_t index = 0; index < runState_.deckCardIds.size(); ++index) {
        if (canUpgradeDeckIndex(index)) {
            result.push_back(index);
        }
    }

    return result;
}

std::vector<std::size_t> RunMapScene::visibleOverlayDeckIndices() const {
    if (overlayMode_ == OverlayMode::Upgrade) {
        return upgradableDeckIndices();
    }

    return allDeckIndices(runState_);
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
