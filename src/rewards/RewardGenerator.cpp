#include "RewardGenerator.hpp"

#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "rewards/RewardOption.hpp"
#include "relics/RelicDefinition.hpp"
#include "relics/RelicRarity.hpp"
#include "run/RunCardEligibility.hpp"

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
    const RelicDatabase& relics,
    const RewardTuning& tuning,
    Random& random
) const {
    RewardState reward;
    reward.sourceNodeType = context.nodeType;

    const NodeRewardTuning& nodeTuning = tuning.node(context.nodeType);
    const int baseGold = nodeTuning.gold;
    int gold = static_cast<int>(static_cast<float>(baseGold) * context.run.goldRewardMultiplier);
    gold = static_cast<int>(static_cast<double>(gold) * relicGoldMultiplier(context, relics));

    if (context.run.archetypeMechanicId == "merchant_progression") {
        gold = static_cast<int>(static_cast<double>(gold) * tuning.merchantGoldMultiplier());
    }

    if (gold > 0) {
        reward.options.push_back(RewardOption::goldReward(gold));
    }

    if (shouldOfferCards(context, tuning)) {
        std::vector<const CardDefinition*> candidates;
        for (const CardDefinition* card : cards.all()) {
            if (card != nullptr && canAppearAsCombatReward(*card) && runCanReceiveCard(context.run, *card)) {
                candidates.push_back(card);
            }
        }

        if (!candidates.empty()) {
            const int optionCount = std::min<int>(cardRewardCount(context, tuning), static_cast<int>(candidates.size()));
            std::vector<CardRewardOption> cardOptions;
            cardOptions.reserve(static_cast<std::size_t>(optionCount));

            for (int i = 0; i < optionCount; ++i) {
                const int pickedIndex = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
                const CardDefinition* picked = candidates[static_cast<std::size_t>(pickedIndex)];

                cardOptions.push_back(CardRewardOption{picked->id});
                candidates.erase(candidates.begin() + pickedIndex);
            }

            if (!cardOptions.empty()) {
                reward.options.push_back(RewardOption::cardChoice(std::move(cardOptions)));
            }
        }
    }

    if (nodeTuning.guaranteedRelic) {
        const std::optional<RelicId> relic = chooseRelicReward(context, relics, random);
        if (relic.has_value()) {
            reward.options.push_back(RewardOption::relic(relic->value));
        }
    }

    return reward;
}

bool RewardGenerator::shouldOfferCards(const RewardContext& context, const RewardTuning& tuning) const {
    if (context.run.archetypeMechanicId == "merchant_progression") {
        return false;
    }

    return tuning.node(context.nodeType).offerCards;
}

int RewardGenerator::cardRewardCount(const RewardContext& context, const RewardTuning& tuning) const {
    return tuning.node(context.nodeType).cardChoices;
}


double RewardGenerator::relicGoldMultiplier(
    const RewardContext& context,
    const RelicDatabase& relics
) const {
    double result = 1.0;

    for (const std::string& relicId : context.run.relicIds) {
        const RelicId id(relicId);

        if (!relics.contains(id)) {
            continue;
        }

        const RelicDefinition& relic = relics.get(id);

        for (const RelicModifierDefinition& modifier : relic.modifiers) {
            if (modifier.type == RelicModifierType::GoldRewardMultiply) {
                result *= modifier.multiplier;
            }
        }
    }

    return result;
}

std::optional<RelicId> RewardGenerator::chooseRelicReward(
    const RewardContext& context,
    const RelicDatabase& relics,
    Random& random
) const {
    std::vector<const RelicDefinition*> candidates;

    for (const RelicDefinition* relic : relics.all()) {
        if (relic == nullptr) {
            continue;
        }

        if (relic->rarity == RelicRarity::Starter || relic->rarity == RelicRarity::Special) {
            continue;
        }

        const bool alreadyOwned = std::find(
            context.run.relicIds.begin(),
            context.run.relicIds.end(),
            relic->id.value
        ) != context.run.relicIds.end();

        if (!alreadyOwned) {
            candidates.push_back(relic);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(index)]->id;
}
