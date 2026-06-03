#pragma once

#include "run/FloorDefinition.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

class FloorDatabase {
public:
    void clear();
    void loadFromFile(const std::filesystem::path& filePath);

    bool contains(const std::string& id) const;
    const FloorDefinition& get(const std::string& id) const;
    const FloorDefinition& startingFloor() const;
    std::vector<const FloorDefinition*> all() const;
    std::size_t size() const;

private:
    std::vector<FloorDefinition> floors_;
};
