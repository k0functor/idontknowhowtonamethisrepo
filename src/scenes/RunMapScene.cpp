#include "RunMapScene.hpp"

#include "ui/BasicUi.hpp"

#include <raylib.h>

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
    BasicUi::drawCenteredText(font_, "Карта забега", Rectangle{0.f, 56.f, static_cast<float>(GetScreenWidth()), 60.f}, 38.f, Color{240, 240, 250, 255});

    for (const RunMapNode& node : runState_.map.nodes) {
        for (const int nextNodeId : node.nextNodeIds) {
            for (const RunMapNode& target : runState_.map.nodes) {
                if (target.id == nextNodeId) {
                    DrawLineEx(node.position, target.position, 3.f, Color{70, 75, 92, 255});
                    break;
                }
            }
        }
    }

    for (const RunMapNode& node : runState_.map.nodes) {
        const Rectangle bounds = nodeBounds(node);
        DrawRectangleRounded(bounds, 0.3f, 16, nodeColor(node));
        DrawRectangleRoundedLinesEx(bounds, 0.3f, 16, 2.f, Color{145, 150, 180, 255});
        BasicUi::drawCenteredText(font_, nodeLabel(node), bounds, 18.f, Color{240, 240, 250, 255});
    }

    BasicUi::drawText(font_, "Первый слой карты: доступен первый бой. Остальное откроется после наград.", Vector2{250.f, 670.f}, 18.f, Color{178, 184, 205, 255});
}

Rectangle RunMapScene::nodeBounds(const RunMapNode& node) const {
    return Rectangle{node.position.x - 54.f, node.position.y - 32.f, 108.f, 64.f};
}

Color RunMapScene::nodeColor(const RunMapNode& node) const {
    if (node.state == RunMapNodeState::Completed) {
        return Color{55, 95, 72, 255};
    }

    if (node.state == RunMapNodeState::Available) {
        return Color{85, 76, 112, 255};
    }

    return Color{38, 41, 52, 255};
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
