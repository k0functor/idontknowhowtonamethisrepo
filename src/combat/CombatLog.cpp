#include "CombatLog.hpp"

void CombatLog::add(std::string entry) {
    entries_.push_back(std::move(entry));
}

void CombatLog::clear() {
    entries_.clear();
}

const std::vector<std::string>& CombatLog::entries() const {
    return entries_;
}
