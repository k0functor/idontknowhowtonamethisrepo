#pragma once

#include "consumables/ConsumableDefinition.hpp"
#include "consumables/ConsumableId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class ConsumableDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(ConsumableDefinition definition);

    bool contains(const ConsumableId& id) const;
    const ConsumableDefinition& get(const ConsumableId& id) const;

    std::vector<const ConsumableDefinition*> all() const;
    std::size_t size() const;

private:
    std::unordered_map<std::string, ConsumableDefinition> consumables_;
};
