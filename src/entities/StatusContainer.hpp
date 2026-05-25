#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class StatusContainer {
public:
    bool has(const std::string& statusId) const;
    int stacks(const std::string& statusId) const;

    void set(const std::string& statusId, int amount);
    void add(const std::string& statusId, int amount);
    void remove(const std::string& statusId);
    void clear();

    bool empty() const;
    std::vector<std::pair<std::string, int>> all() const;

private:
    std::unordered_map<std::string, int> statuses_;
};
