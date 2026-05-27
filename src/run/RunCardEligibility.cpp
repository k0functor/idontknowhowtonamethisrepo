#include "RunCardEligibility.hpp"

#include <algorithm>

bool runCanReceiveCard(const RunState& run, const CardDefinition& card) {
    if (card.ownerActorId.empty()) {
        return true;
    }

    return std::find(
        run.actorDefinitionIds.begin(),
        run.actorDefinitionIds.end(),
        card.ownerActorId
    ) != run.actorDefinitionIds.end();
}
