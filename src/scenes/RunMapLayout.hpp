#pragma once

#include "run/RunMap.hpp"

#include <raylib.h>

class RunMapLayout {
public:
    static Vector2 nodeScreenPosition(const RunMap& map, const RunMapNode& node, float scrollOffset);
    static Rectangle mapViewportBounds();
    static float mapMaxScrollOffset(const RunMap& map);
    static float scrollOffsetForNode(const RunMap& map, const RunMapNode& node);
    static Rectangle nodeBounds(const RunMap& map, const RunMapNode& node, float scrollOffset);

    static float scrollEpsilon();
    static float edgeScrollZone();
    static float edgeScrollMaxSpeed();

    static Rectangle restModalBounds();
    static Rectangle restHealButtonBounds(Rectangle modal);
    static Rectangle restCalmButtonBounds(Rectangle modal);
    static Rectangle restUpgradeButtonBounds(Rectangle modal);
    static Rectangle restSkipButtonBounds(Rectangle modal);
    static Rectangle restCancelButtonBounds(Rectangle modal);

    static Rectangle deckButtonBounds();
    static Rectangle relicsButtonBounds();
    static Rectangle consumablesButtonBounds();
    static Rectangle abandonButtonBounds();
    static Rectangle abandonModalBounds();
    static Rectangle abandonCancelButtonBounds(Rectangle modal);
    static Rectangle abandonConfirmButtonBounds(Rectangle modal);

    static Rectangle overlayBounds();
    static Rectangle overlayCloseButtonBounds(Rectangle modal);
    static Rectangle overlayGridBounds(Rectangle modal);
    static Rectangle upgradePreviewModalBounds();
    static Rectangle upgradePreviewBeforeCardBounds(Rectangle modal);
    static Rectangle upgradePreviewAfterCardBounds(Rectangle modal);
    static Rectangle upgradePreviewCancelButtonBounds(Rectangle modal);
    static Rectangle upgradePreviewConfirmButtonBounds(Rectangle modal);

private:
    static float mapScale(const RunMap& map);
    static float mapContentWidth(const RunMap& map);
};
