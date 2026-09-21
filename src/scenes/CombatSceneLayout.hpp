#pragma once

#include <raylib.h>

#include <cstddef>
#include <optional>

class CombatSceneLayout {
public:
    static Vector2 playedCardCenterPosition();
    static Vector2 playedCardQueuePosition(std::size_t queueIndex);
    static Vector2 discardPileCenterPosition();

    static Rectangle drawPileButtonBounds();
    static Rectangle discardPileButtonBounds();
    static Rectangle exhaustPileButtonBounds();
    static Rectangle energyBubbleBounds(std::optional<Rectangle> primaryPlayerBounds);

    static Rectangle pileOverlayBounds();
    static Rectangle pileOverlayGridBounds(Rectangle modal);
    static Rectangle pileOverlayCloseButtonBounds(Rectangle modal);
    static Rectangle pileOverlayCardBounds(Rectangle grid, std::size_t index, float scrollOffset);
    static float pileOverlayMaxScroll(Rectangle grid, std::size_t count);

    static Rectangle combatItemInspectModalBounds();
    static Rectangle combatItemInspectCloseButtonBounds(Rectangle modal);
    static Rectangle combatItemInspectPreviousButtonBounds(Rectangle modal);
    static Rectangle combatItemInspectNextButtonBounds(Rectangle modal);

    static Rectangle consumableConfirmationBounds();
    static Rectangle consumableConfirmButtonBounds(Rectangle modal);
    static Rectangle consumableCancelButtonBounds(Rectangle modal);

    static Rectangle rewardModalBounds();
    static Rectangle rewardOptionRowBounds(std::size_t index);
    static Rectangle rewardContinueButtonBounds();
    static Rectangle rewardCardChoiceModalBounds();
    static Rectangle rewardCardChoiceOptionBounds(std::size_t optionCount, std::size_t index);
    static Rectangle rewardCardChoiceCancelBounds();
    static Rectangle rewardCardChoiceConfirmBounds();

    static Rectangle fallbackFeedbackTargetBounds();
    static Vector2 feedbackAnchor(Rectangle targetBounds);
};
