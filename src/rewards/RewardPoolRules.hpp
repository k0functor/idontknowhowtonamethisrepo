#pragma once

#include "cards/CardDefinition.hpp"
#include "relics/RelicDefinition.hpp"

#include <string>

namespace RewardPoolRules {

bool canAppearAsCardReward(const CardDefinition& card);
bool canAppearInShop(const CardDefinition& card);

bool matchesRunMechanic(const RelicDefinition& relic, const std::string& mechanicId);
bool canAppearAsRelicReward(const RelicDefinition& relic);
bool canAppearAsRelicReward(const RelicDefinition& relic, const std::string& mechanicId);
bool canAppearInShop(const RelicDefinition& relic);
bool canAppearInShop(const RelicDefinition& relic, const std::string& mechanicId);

}
