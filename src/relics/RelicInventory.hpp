#pragma once

#include "relics/RelicId.hpp"
#include "relics/RelicInstance.hpp"

#include <string>
#include <vector>

class RelicInventory {
public:
    void clear();
    void resetCombatState();

    void add(RelicId id);
    void setFromIds(const std::vector<std::string>& ids);

    bool contains(const RelicId& id) const;

    std::vector<RelicInstance>& all();
    const std::vector<RelicInstance>& all() const;

private:
    std::vector<RelicInstance> relics_;
};
