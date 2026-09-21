#include "CombatLog.hpp"

void CombatLog::add(CombatLogEntry entry) {
    entry.sequence = nextSequence_++;
    entries_.push_back(std::move(entry));
    if (entries_.size() > maxEntries_) {
        const std::size_t overflow = entries_.size() - maxEntries_;
        entries_.erase(entries_.begin(), entries_.begin() + static_cast<std::ptrdiff_t>(overflow));
    }
}

void CombatLog::add(const CombatLogEntryType type, CombatLogEntry::Variables variables) {
    add(CombatLogEntry::make(type, std::move(variables)));
}

void CombatLog::addText(std::string text) {
    add(CombatLogEntry::makeText(std::move(text)));
}

void CombatLog::clear() {
    entries_.clear();
    nextSequence_ = 1u;
}

const std::vector<CombatLogEntry>& CombatLog::entries() const {
    return entries_;
}
