#include "RewardGenerator.hpp"

#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
bool canAppearAsCombatReward(const CardDefinition& card) {
    if (card.type == CardType::Status || card.type == CardType::Curse) {
        return false;
    }

    if (card.rarity == CardRarity::Starter || card.rarity == CardRarity::Special) {
        return false;
    }

    return true;
}
}

RewardState RewardGenerator::generateCombatReward(
    const RewardContext& context,
    const CardDatabase& cards,
    Random& random
) const {
    RewardState reward;
    reward.sourceNodeType = context.nodeType;

    const int baseGold = baseGoldForNode(context.nodeType);
    reward.gold = static_cast<int>(static_cast<float>(baseGold) * context.run.goldRewardMultiplier);

    if (context.run.archetypeMechanicId == "merchant_progression") {
        reward.gold = static_cast<int>(static_cast<float>(reward.gold) * 1.25f);
    }

    if (!shouldOfferCards(context)) {
        return reward;
    }

    std::vector<const CardDefinition*> candidates;
    for (const CardDefinition* card : cards.all()) {
        if (card != nullptr && canAppearAsCombatReward(*card)) {
            candidates.push_back(card);
        }
    }

    if (candidates.empty()) {
        return reward;
    }

    const int optionCount = std::min<int>(cardRewardCount(context), static_cast<int>(candidates.size()));

    for (int i = 0; i < optionCount; ++i) {
        const int pickedIndex = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
        const CardDefinition* picked = candidates[static_cast<std::size_t>(pickedIndex)];

        reward.cardOptions.push_back(CardRewardOption{picked->id});
        candidates.erase(candidates.begin() + pickedIndex);
    }

    return reward;
}

int RewardGenerator::baseGoldForNode(const RunMapNodeType nodeType) const {
    switch (nodeType) {
        case RunMapNodeType::Combat:
            return 18;
        case RunMapNodeType::Elite:
            return 35;
        case RunMapNodeType::Boss:
            return 75;
        case RunMapNodeType::Event:
        case RunMapNodeType::Shop:
        case RunMapNodeType::Rest:
            return 0;
    }

    throw std::runtime_error("Unknown RunMapNodeType in RewardGenerator");
}

bool RewardGenerator::shouldOfferCards(const RewardContext& context) const {
    if (context.run.archetypeMechanicId == "merchant_progression") {
        return false;
    }

    return context.nodeType == RunMapNodeType::Combat ||
        context.nodeType == RunMapNodeType::Elite ||
        context.nodeType == RunMapNodeType::Boss;
}

int RewardGenerator::cardRewardCount(const RewardContext&) const {
    return 3;
}
