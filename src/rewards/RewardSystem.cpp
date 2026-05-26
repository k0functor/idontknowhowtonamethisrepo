#include "RewardSystem.hpp"

void RewardSystem::applyReward(
    RunState& run,
    const RewardState&,
    const RewardSelection& selection
) const {
    if (selection.goldTaken > 0) {
        run.gold += selection.goldTaken;
        run.stats.goldGained += selection.goldTaken;
    }

    for (const CardId& cardId : selection.selectedCardIds) {
        run.deckCardIds.push_back(cardId);
        ++run.stats.cardsAdded;
    }

    for (const std::string& consumableId : selection.selectedConsumableIds) {
        if (static_cast<int>(run.consumableIds.size()) < run.maxConsumables) {
            run.consumableIds.push_back(consumableId);
        }
    }
}
