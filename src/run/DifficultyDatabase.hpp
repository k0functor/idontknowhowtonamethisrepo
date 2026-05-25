#pragma once

#include "run/DifficultyDefinition.hpp"
#include "run/DifficultyId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class DifficultyDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(DifficultyDefinition definition);

    bool contains(const DifficultyId& id) const;
    const DifficultyDefinition& get(const DifficultyId& id) const;

    std::vector<const DifficultyDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, DifficultyDefinition> difficulties_;
};
