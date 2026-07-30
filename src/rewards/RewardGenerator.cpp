#include "RewardGenerator.hpp"

#include "active_items/ActiveItemAcquisitionSystem.hpp"

#include "cards/CardRarity.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "relics/RelicDefinition.hpp"
#include "rewards/RewardOption.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "run/RunCardEligibility.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int rarityRank(const CardRarity rarity) {
    switch (rarity) {
        case CardRarity::Starter:
            return 0;
        case CardRarity::Common:
            return 1;
        case CardRarity::Uncommon:
            return 2;
        case CardRarity::Rare:
            return 3;
        case CardRarity::Special:
            return 4;
    }

    return 0;
}

bool meetsMinimumCardRarity(const CardDefinition& card, const std::optional<CardRarity>& minimumRarity) {
    if (!minimumRarity.has_value()) {
        return true;
    }

    return rarityRank(card.rarity) >= rarityRank(*minimumRarity);
}
}

RewardState RewardGenerator::generateCombatReward(
    const RewardContext& context,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const ActiveItemDatabase& activeItems,
    const RewardTuning& tuning,
    Random& random
) const {
    RewardState reward;
    reward.sourceNodeType = context.nodeType;

    const NodeRewardTuning& nodeTuning = tuning.node(context.nodeType);
    const int baseGold = nodeTuning.gold;
    int gold = static_cast<int>(static_cast<float>(baseGold) * context.run.goldRewardMultiplier);
    gold = static_cast<int>(static_cast<double>(gold) * tuning.groupGoldMultiplier(context.enemyCount));
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
            if (card != nullptr &&
                RewardPoolRules::canAppearAsCardReward(*card) &&
                runCanReceiveArchetypeRewardCard(context.run, *card) &&
                meetsMinimumCardRarity(*card, nodeTuning.minimumCardRarity)) {
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

    const std::optional<std::string> consumable = chooseConsumableReward(context, consumables, tuning, random);
    if (consumable.has_value()) {
        reward.options.push_back(RewardOption::consumable(*consumable));
    }

    if (nodeTuning.guaranteedRelic) {
        const std::optional<RelicId> relic = chooseRelicReward(context, relics, random);
        if (relic.has_value()) {
            reward.options.push_back(RewardOption::relic(relic->value));
        }
    }

    if (nodeTuning.activeItemChancePercent > 0 &&
        random.chance(static_cast<double>(nodeTuning.activeItemChancePercent) / 100.0)) {
        const std::optional<ActiveItemId> item = ActiveItemAcquisitionSystem::chooseReward(
            activeItems,
            context.run.activeItem.itemId,
            random
        );
        if (item.has_value()) {
            reward.options.push_back(RewardOption::activeItem(item->value));
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

std::optional<std::string> RewardGenerator::chooseConsumableReward(
    const RewardContext& context,
    const ConsumableDatabase& consumables,
    const RewardTuning& tuning,
    Random& random
) const {
    const NodeRewardTuning& nodeTuning = tuning.node(context.nodeType);
    if (nodeTuning.consumableChancePercent <= 0) {
        return std::nullopt;
    }

    if (static_cast<int>(context.run.consumableIds.size()) >= context.run.maxConsumables) {
        return std::nullopt;
    }

    if (!random.chance(static_cast<double>(nodeTuning.consumableChancePercent) / 100.0)) {
        return std::nullopt;
    }

    std::vector<const ConsumableDefinition*> candidates;
    for (const ConsumableDefinition* consumable : consumables.all()) {
        if (consumable != nullptr) {
            candidates.push_back(consumable);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(index)]->id.value;
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

        if (!RewardPoolRules::canAppearAsRelicReward(*relic)) {
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
