#include "rewards/RewardPoolRules.hpp"

#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "relics/RelicRarity.hpp"

namespace {
bool isGeneratedCardPoolExcluded(const CardDefinition& card) {
    if (card.type == CardType::Status || card.type == CardType::Curse) {
        return true;
    }

    if (card.rarity == CardRarity::Starter || card.rarity == CardRarity::Special) {
        return true;
    }

    return false;
}

bool isGeneratedRelicPoolExcluded(const RelicDefinition& relic) {
    return relic.rarity == RelicRarity::Starter || relic.rarity == RelicRarity::Special;
}
}

namespace RewardPoolRules {

bool canAppearAsCardReward(const CardDefinition& card) {
    return !isGeneratedCardPoolExcluded(card);
}

bool canAppearInShop(const CardDefinition& card) {
    return !isGeneratedCardPoolExcluded(card);
}

bool canAppearAsRelicReward(const RelicDefinition& relic) {
    return !isGeneratedRelicPoolExcluded(relic);
}

bool canAppearInShop(const RelicDefinition& relic) {
    return !isGeneratedRelicPoolExcluded(relic);
}

}
