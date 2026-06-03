#include "RewardSystem.hpp"

#include "run/RunRelicOwnership.hpp"

#include <algorithm>

namespace {
int offeredCardCount(const RewardState& reward) {
    int count = 0;
    for (const RewardOption& option : reward.options) {
        if (option.type == RewardOptionType::CardChoice) {
            count += static_cast<int>(option.cardOptions.size());
        }
    }
    return count;
}

void addRelicToRun(RunState& run, const std::string& relicId, const std::string& actorDefinitionId) {
    if (RunRelicOwnership::assignRelicToActor(run, relicId, actorDefinitionId)) {
        ++run.stats.relicsGained;
    }
}
}

void RewardSystem::applyReward(
    RunState& run,
    const RewardState& reward,
    const RewardSelection& selection
) const {
    if (!reward.empty() && selection.empty()) {
        ++run.stats.rewardsSkipped;
    }

    const int offeredCards = offeredCardCount(reward);
    if (offeredCards > 0) {
        run.stats.cardsSkipped += std::max(0, offeredCards - static_cast<int>(selection.selectedCardIds.size()));
    }

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
            ++run.stats.consumablesGained;
        }
    }

    for (const std::string& relicId : selection.selectedRelicIds) {
        addRelicToRun(run, relicId, RunRelicOwnership::defaultActorDefinitionId(run));
    }

    for (const RelicRewardSelection& relic : selection.selectedRelics) {
        addRelicToRun(run, relic.relicId, relic.actorDefinitionId);
    }
}
