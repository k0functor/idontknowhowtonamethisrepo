#pragma once

#include "cards/CardDefinition.hpp"
#include "relics/RelicDefinition.hpp"

namespace RewardPoolRules {

bool canAppearAsCardReward(const CardDefinition& card);
bool canAppearInShop(const CardDefinition& card);

bool canAppearAsRelicReward(const RelicDefinition& relic);
bool canAppearInShop(const RelicDefinition& relic);

}
