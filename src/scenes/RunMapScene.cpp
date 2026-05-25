#include "RunMapScene.hpp"

#include "ui/BasicUi.hpp"

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
    const RunState& runState,
    std::function<void(int)> onNodeSelected,
    std::function<void()> onBackToHub
)
    : font_(font),
      runState_(runState),
      onNodeSelected_(std::move(onNodeSelected)),
      onBackToHub_(std::move(onBackToHub)) {}

void RunMapScene::update(float) {
    const Vector2 mouse = GetMousePosition();

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

        if (BasicUi::contains(nodeBounds(node), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onNodeSelected_(node.id);
            return;
        }
    }
}

void RunMapScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, Rectangle{32.f, 32.f, 180.f, 48.f}, "В хаб", mouse);

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

    for (const RunMapNode& node : runState_.map.nodes) {
        const Rectangle bounds = nodeBounds(node);
        const bool isHovered = node.state == RunMapNodeState::Available && BasicUi::contains(bounds, mouse);

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
}

Vector2 RunMapScene::nodeScreenPosition(const RunMapNode& node) const {
    const MapRawBounds rawBounds = calculateRawBounds(runState_.map);

    const float rawWidth = safeDimension(rawBounds.maxX - rawBounds.minX);
    const float rawHeight = safeDimension(rawBounds.maxY - rawBounds.minY);

    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());

    const float horizontalPadding = std::max(180.f, screenWidth * 0.12f);
    const float verticalPadding = std::max(120.f, screenHeight * 0.16f);

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
            return "Бой";
        case RunMapNodeType::Elite:
            return "Элитка";
        case RunMapNodeType::Event:
            return "?";
        case RunMapNodeType::Shop:
            return "Магазин";
        case RunMapNodeType::Rest:
            return "Отдых";
        case RunMapNodeType::Boss:
            return "Босс";
    }

    return "?";
}
