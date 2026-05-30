#include "CombatLog.hpp"

void CombatLog::add(CombatLogEntry entry) {
    entries_.push_back(std::move(entry));
}

void CombatLog::add(const CombatLogEntryType type, CombatLogEntry::Variables variables) {
    entries_.push_back(CombatLogEntry::make(type, std::move(variables)));
}

void CombatLog::addText(std::string text) {
    entries_.push_back(CombatLogEntry::makeText(std::move(text)));
}

void CombatLog::clear() {
    entries_.clear();
}

const std::vector<CombatLogEntry>& CombatLog::entries() const {
    return entries_;
}
