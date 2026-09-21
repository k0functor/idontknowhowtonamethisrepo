#include "RunMapLayout.hpp"

#include "ui/CardVisualInstance.hpp"
#include "ui/VirtualViewport.hpp"

#include <algorithm>
#include <limits>

namespace {
constexpr float NODE_WIDTH = 108.f;
constexpr float NODE_HEIGHT = 64.f;
constexpr float MAP_SCROLL_EPSILON = 1.f;
constexpr float MAP_EDGE_SCROLL_ZONE = 86.f;
constexpr float MAP_EDGE_SCROLL_MAX_SPEED = 980.f;
constexpr float MAP_VIEWPORT_MARGIN_X = 32.f;
constexpr float MAP_VIEWPORT_TOP = 96.f;
constexpr float MAP_VIEWPORT_MIN_HEIGHT = 150.f;
constexpr float MAP_VIEWPORT_BOTTOM_RESERVE = 78.f;
constexpr float MAP_CONTENT_EDGE_PADDING = 118.f;

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
}

Vector2 RunMapLayout::nodeScreenPosition(
    const RunMap& map,
    const RunMapNode& node,
    const float scrollOffset
) {
    const MapRawBounds rawBounds = calculateRawBounds(map);
    const float scale = mapScale(map);
    const Rectangle viewport = mapViewportBounds();
    const Vector2 rawCenter{
        (rawBounds.minX + rawBounds.maxX) * 0.5f,
        (rawBounds.minY + rawBounds.maxY) * 0.5f
    };

    return Vector2{
        viewport.x + MAP_CONTENT_EDGE_PADDING + (node.position.x - rawBounds.minX) * scale - scrollOffset,
        viewport.y + viewport.height * 0.5f + (node.position.y - rawCenter.y) * scale
    };
}

Rectangle RunMapLayout::mapViewportBounds() {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());
    const float width = std::max(1.f, screenWidth - MAP_VIEWPORT_MARGIN_X * 2.f);
    const float height = std::max(
        MAP_VIEWPORT_MIN_HEIGHT,
        screenHeight - MAP_VIEWPORT_TOP - MAP_VIEWPORT_BOTTOM_RESERVE
    );

    return Rectangle{MAP_VIEWPORT_MARGIN_X, MAP_VIEWPORT_TOP, width, height};
}

float RunMapLayout::mapScale(const RunMap& map) {
    const MapRawBounds rawBounds = calculateRawBounds(map);
    const float rawHeight = safeDimension(rawBounds.maxY - rawBounds.minY);
    const float verticalScale = mapViewportBounds().height / rawHeight;

    // Long maps scroll horizontally instead of compressing nodes into unreadable clusters.
    return std::min(1.f, verticalScale);
}

float RunMapLayout::mapContentWidth(const RunMap& map) {
    const MapRawBounds rawBounds = calculateRawBounds(map);
    const float rawWidth = safeDimension(rawBounds.maxX - rawBounds.minX);
    return rawWidth * mapScale(map) + MAP_CONTENT_EDGE_PADDING * 2.f;
}

float RunMapLayout::mapMaxScrollOffset(const RunMap& map) {
    return std::max(0.f, mapContentWidth(map) - mapViewportBounds().width);
}

float RunMapLayout::scrollOffsetForNode(const RunMap& map, const RunMapNode& node) {
    const MapRawBounds rawBounds = calculateRawBounds(map);
    const Rectangle viewport = mapViewportBounds();
    const float rawNodeX = MAP_CONTENT_EDGE_PADDING + (node.position.x - rawBounds.minX) * mapScale(map);
    const float desiredScreenX = viewport.x + viewport.width * 0.5f;
    return std::clamp(
        viewport.x + rawNodeX - desiredScreenX,
        0.f,
        mapMaxScrollOffset(map)
    );
}

Rectangle RunMapLayout::nodeBounds(
    const RunMap& map,
    const RunMapNode& node,
    const float scrollOffset
) {
    const Vector2 position = nodeScreenPosition(map, node, scrollOffset);
    return Rectangle{
        position.x - NODE_WIDTH * 0.5f,
        position.y - NODE_HEIGHT * 0.5f,
        NODE_WIDTH,
        NODE_HEIGHT
    };
}

float RunMapLayout::scrollEpsilon() {
    return MAP_SCROLL_EPSILON;
}

float RunMapLayout::edgeScrollZone() {
    return MAP_EDGE_SCROLL_ZONE;
}

float RunMapLayout::edgeScrollMaxSpeed() {
    return MAP_EDGE_SCROLL_MAX_SPEED;
}

Rectangle RunMapLayout::restModalBounds() {
    const float width = std::min(660.f, static_cast<float>(VirtualViewport::width()) - 48.f);
    const float height = std::min(500.f, static_cast<float>(VirtualViewport::height()) - 48.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapLayout::restHealButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + 42.f, modal.y + modal.height - 214.f, (modal.width - 96.f) * 0.5f, 52.f};
}

Rectangle RunMapLayout::restCalmButtonBounds(const Rectangle modal) {
    const Rectangle heal = restHealButtonBounds(modal);
    return Rectangle{heal.x + heal.width + 12.f, heal.y, heal.width, heal.height};
}

Rectangle RunMapLayout::restUpgradeButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + 42.f, modal.y + modal.height - 150.f, modal.width - 84.f, 52.f};
}

Rectangle RunMapLayout::restSkipButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + 42.f, modal.y + modal.height - 88.f, modal.width - 220.f, 46.f};
}

Rectangle RunMapLayout::restCancelButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width - 150.f, modal.y + modal.height - 58.f, 112.f, 38.f};
}

Rectangle RunMapLayout::deckButtonBounds() {
    constexpr float buttonWidth = 126.f;
    constexpr float buttonHeight = 42.f;
    constexpr float gap = 10.f;
    constexpr float relicsWidth = 126.f;
    constexpr float consumablesWidth = 156.f;
    constexpr float totalWidth = buttonWidth + relicsWidth + consumablesWidth + gap * 2.f;
    const float startX = (static_cast<float>(VirtualViewport::width()) - totalWidth) * 0.5f;
    return Rectangle{startX, 32.f, buttonWidth, buttonHeight};
}

Rectangle RunMapLayout::relicsButtonBounds() {
    constexpr float buttonWidth = 126.f;
    constexpr float buttonHeight = 42.f;
    constexpr float gap = 10.f;
    const Rectangle deck = deckButtonBounds();
    return Rectangle{deck.x + deck.width + gap, 32.f, buttonWidth, buttonHeight};
}

Rectangle RunMapLayout::consumablesButtonBounds() {
    constexpr float buttonWidth = 156.f;
    constexpr float buttonHeight = 42.f;
    constexpr float gap = 10.f;
    const Rectangle relics = relicsButtonBounds();
    return Rectangle{relics.x + relics.width + gap, 32.f, buttonWidth, buttonHeight};
}

Rectangle RunMapLayout::abandonButtonBounds() {
    return Rectangle{274.f, 32.f, 140.f, 48.f};
}

Rectangle RunMapLayout::abandonModalBounds() {
    constexpr float width = 440.f;
    constexpr float height = 190.f;
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapLayout::abandonCancelButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + 28.f, modal.y + modal.height - 66.f, 180.f, 42.f};
}

Rectangle RunMapLayout::abandonConfirmButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width - 208.f, modal.y + modal.height - 66.f, 180.f, 42.f};
}

Rectangle RunMapLayout::overlayBounds() {
    const float width = std::min(1180.f, static_cast<float>(VirtualViewport::width()) - 56.f);
    const float height = std::min(680.f, static_cast<float>(VirtualViewport::height()) - 56.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapLayout::overlayCloseButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width - 146.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle RunMapLayout::overlayGridBounds(const Rectangle modal) {
    return Rectangle{modal.x + 28.f, modal.y + 86.f, modal.width - 56.f, modal.height - 166.f};
}

Rectangle RunMapLayout::upgradePreviewModalBounds() {
    constexpr float width = 760.f;
    constexpr float height = 520.f;
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle RunMapLayout::upgradePreviewBeforeCardBounds(const Rectangle modal) {
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    return Rectangle{modal.x + 72.f, modal.y + 104.f, cardSize.x, cardSize.y};
}

Rectangle RunMapLayout::upgradePreviewAfterCardBounds(const Rectangle modal) {
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    return Rectangle{modal.x + modal.width - 72.f - cardSize.x, modal.y + 104.f, cardSize.x, cardSize.y};
}

Rectangle RunMapLayout::upgradePreviewCancelButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width * 0.5f - 224.f, modal.y + modal.height - 72.f, 192.f, 44.f};
}

Rectangle RunMapLayout::upgradePreviewConfirmButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width * 0.5f + 32.f, modal.y + modal.height - 72.f, 192.f, 44.f};
}
