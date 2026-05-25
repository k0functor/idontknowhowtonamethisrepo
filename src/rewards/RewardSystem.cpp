#include "RewardSystem.hpp"

void RewardSystem::applyReward(
    RunState& run,
    const RewardState& reward,
    const RewardSelection& selection
) const {
    if (selection.takeGold) {
        run.gold += reward.gold;
        run.stats.goldGained += reward.gold;
    }

    if (selection.selectedCardId.has_value()) {
        run.deckCardIds.push_back(*selection.selectedCardId);
        ++run.stats.cardsAdded;
    }
}
