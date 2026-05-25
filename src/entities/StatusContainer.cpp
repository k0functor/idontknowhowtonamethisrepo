#include "StatusContainer.hpp"

bool StatusContainer::has(const std::string& statusId) const {
    return stacks(statusId) > 0;
}

int StatusContainer::stacks(const std::string& statusId) const {
    const auto iterator = statuses_.find(statusId);
    if (iterator == statuses_.end()) {
        return 0;
    }

    return iterator->second;
}

void StatusContainer::set(const std::string& statusId, const int amount) {
    if (amount <= 0) {
        statuses_.erase(statusId);
        return;
    }

    statuses_[statusId] = amount;
}

void StatusContainer::add(const std::string& statusId, const int amount) {
    if (amount == 0) {
        return;
    }

    set(statusId, stacks(statusId) + amount);
}

void StatusContainer::remove(const std::string& statusId) {
    statuses_.erase(statusId);
}

void StatusContainer::clear() {
    statuses_.clear();
}

std::vector<std::pair<std::string, int>> StatusContainer::all() const {
    std::vector<std::pair<std::string, int>> result;
    result.reserve(statuses_.size());

    for (const auto& entry : statuses_) {
        result.push_back(entry);
    }

    return result;
}
