#pragma once

#include "relics/RelicDefinition.hpp"
#include "relics/RelicId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class RelicDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(RelicDefinition definition);

    bool contains(const RelicId& id) const;
    const RelicDefinition& get(const RelicId& id) const;

    std::vector<const RelicDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, RelicDefinition> relics_;
};
