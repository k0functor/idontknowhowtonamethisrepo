#include "RelicInventory.hpp"

#include <algorithm>

void RelicInventory::clear() {
    relics_.clear();
}

void RelicInventory::resetCombatState() {
    for (RelicInstance& relic : relics_) {
        relic.triggersThisCombat = 0;
    }
}

void RelicInventory::add(RelicId id) {
    relics_.push_back(RelicInstance{std::move(id)});
}

void RelicInventory::setFromIds(const std::vector<std::string>& ids) {
    clear();

    for (const std::string& id : ids) {
        add(RelicId(id));
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
