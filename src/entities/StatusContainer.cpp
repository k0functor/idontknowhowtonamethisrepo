#include "StatusContainer.hpp"

#include <algorithm>

bool StatusContainer::has(const std::string& statusId) const {
    return stacks(statusId) > 0;
}

int StatusContainer::stacks(const std::string& statusId) const {
    const auto iterator = statuses_.find(statusId);
    if (iterator == statuses_.end()) {
        return 0;
    }

    return iterator->second.amount;
}

std::optional<EntityId> StatusContainer::source(const std::string& statusId) const {
    const auto iterator = statuses_.find(statusId);
    if (iterator == statuses_.end()) {
        return std::nullopt;
    }

    return iterator->second.source;
}

void StatusContainer::set(
    const std::string& statusId,
    const int amount,
    const std::optional<EntityId> source
) {
    if (amount <= 0) {
        statuses_.erase(statusId);
        return;
    }

    StatusEntry& entry = statuses_[statusId];
    entry.amount = amount;

    if (source.has_value()) {
        entry.source = source;
    }
}

void StatusContainer::add(
    const std::string& statusId,
    const int amount,
    const std::optional<EntityId> source
) {
    if (amount == 0) {
        return;
    }

    set(statusId, stacks(statusId) + amount, source);
}

void StatusContainer::remove(const std::string& statusId) {
    statuses_.erase(statusId);
}

void StatusContainer::clear() {
    statuses_.clear();
}

bool StatusContainer::empty() const {
    return statuses_.empty();
}

std::vector<std::pair<std::string, int>> StatusContainer::all() const {
    std::vector<std::pair<std::string, int>> result;
    result.reserve(statuses_.size());

    for (const auto& [statusId, entry] : statuses_) {
        result.emplace_back(statusId, entry.amount);
    }

    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    return result;
}
