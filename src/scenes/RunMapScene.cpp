#include "RunMapScene.hpp"

#include "ui/BasicUi.hpp"
#include "localization/TextFormatter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
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
    const RunState& runState,
    std::function<void(int)> onNodeSelected,
    std::function<void(int)> onRestHeal,
    std::function<void(int)> onRestUpgrade,
    std::function<void(int)> onRestSkip,
    std::function<void()> onBackToHub
)
    : font_(font),
      localization_(localization),
      runState_(runState),
      onNodeSelected_(std::move(onNodeSelected)),
      onRestHeal_(std::move(onRestHeal)),
      onRestUpgrade_(std::move(onRestUpgrade)),
      onRestSkip_(std::move(onRestSkip)),
      onBackToHub_(std::move(onBackToHub)) {}

void RunMapScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (restModalNodeId_.has_value()) {
        updateRestModal(mouse);
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        onBackToHub_();
        return;
    }

    const Rectangle back{32.f, 32.f, 180.f, 48.f};
    if (BasicUi::contains(back, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBackToHub_();
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

    BasicUi::drawText(
        font_,
        runHpSummaryText(),
        Vector2{232.f, 45.f},
        22.f,
        Color{235, 224, 185, 255}
    );
    BasicUi::drawText(
        font_,
        runStressSummaryText(),
        Vector2{472.f, 45.f},
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
        const int nodeId = *restModalNodeId_;
        restModalNodeId_ = std::nullopt;
        onRestUpgrade_(nodeId);
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
