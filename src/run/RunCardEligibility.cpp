#include "RunCardEligibility.hpp"

#include <algorithm>
#include <vector>

namespace {
bool containsString(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}
}

bool runCanReceiveCard(const RunState& run, const CardDefinition& card) {
    if (card.ownerActorId.empty()) {
        return true;
    }

    return containsString(run.actorDefinitionIds, card.ownerActorId);
}

bool runCanReceiveArchetypeRewardCard(const RunState& run, const CardDefinition& card) {
    if (card.ownerActorId.empty()) {
        return false;
    }

    const std::vector<std::string>& pools = run.rewardCardPoolIds.empty()
        ? run.actorDefinitionIds
        : run.rewardCardPoolIds;

    return containsString(pools, card.ownerActorId);
}
