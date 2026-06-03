#include "RelicInventory.hpp"

#include <algorithm>
#include <utility>

void RelicInventory::clear() {
    relics_.clear();
}

void RelicInventory::resetCombatState() {
    for (RelicInstance& relic : relics_) {
        relic.triggersThisCombat = 0;
    }
}

void RelicInventory::add(RelicId id) {
    relics_.push_back(RelicInstance{std::move(id), {}});
}

void RelicInventory::add(RelicId id, std::string ownerActorDefinitionId) {
    relics_.push_back(RelicInstance{std::move(id), std::move(ownerActorDefinitionId)});
}

void RelicInventory::setFromIds(const std::vector<std::string>& ids) {
    clear();

    for (const std::string& id : ids) {
        add(RelicId(id));
    }
}

void RelicInventory::setFromRun(const RunState& run) {
    clear();

    for (const RunActorState& actor : run.actorStates) {
        for (const std::string& relicId : actor.relicIds) {
            add(RelicId(relicId), actor.definitionId);
        }
    }

    for (const std::string& relicId : run.relicIds) {
        const bool alreadyPresent = std::any_of(relics_.begin(), relics_.end(), [&relicId](const RelicInstance& relic) {
            return relic.id.value == relicId;
        });

        if (!alreadyPresent) {
            add(RelicId(relicId));
        }
    }
}

bool RelicInventory::contains(const RelicId& id) const {
    return std::any_of(relics_.begin(), relics_.end(), [&id](const RelicInstance& relic) {
        return relic.id == id;
    });
}

std::vector<RelicInstance>& RelicInventory::all() {
    return relics_;
}

const std::vector<RelicInstance>& RelicInventory::all() const {
    return relics_;
}
