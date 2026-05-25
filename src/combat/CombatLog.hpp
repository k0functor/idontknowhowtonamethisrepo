#pragma once

#include <string>
#include <vector>

class CombatLog {
public:
    void add(std::string entry);
    void clear();

    const std::vector<std::string>& entries() const;

private:
    std::vector<std::string> entries_;
};
