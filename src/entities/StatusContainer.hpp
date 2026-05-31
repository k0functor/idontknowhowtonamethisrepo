#pragma once

#include "entities/EntityId.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class StatusContainer {
public:
    bool has(const std::string& statusId) const;
    int stacks(const std::string& statusId) const;
    std::optional<EntityId> source(const std::string& statusId) const;

    void set(const std::string& statusId, int amount, std::optional<EntityId> source = std::nullopt);
    void add(const std::string& statusId, int amount, std::optional<EntityId> source = std::nullopt);
    void remove(const std::string& statusId);
    void clear();

    bool empty() const;
    std::vector<std::pair<std::string, int>> all() const;

private:
    struct StatusEntry {
        int amount = 0;
        std::optional<EntityId> source;
    };

    std::unordered_map<std::string, StatusEntry> statuses_;
};
