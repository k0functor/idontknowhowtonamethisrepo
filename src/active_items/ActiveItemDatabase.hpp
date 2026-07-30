#pragma once

#include "active_items/ActiveItemDefinition.hpp"
#include "active_items/ActiveItemId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class ActiveItemDatabase {
public:
    void clear();
    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);
    void add(ActiveItemDefinition definition);

    bool contains(const ActiveItemId& id) const;
    const ActiveItemDefinition& get(const ActiveItemId& id) const;
    std::vector<const ActiveItemDefinition*> all() const;
    std::size_t size() const;

private:
    std::unordered_map<std::string, ActiveItemDefinition> items_;
};
