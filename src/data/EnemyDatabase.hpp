#pragma once

#include "enemies/EnemyDefinition.hpp"
#include "enemies/EnemyId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class EnemyDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(EnemyDefinition definition);

    bool contains(const EnemyId& id) const;
    const EnemyDefinition& get(const EnemyId& id) const;

    std::vector<const EnemyDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, EnemyDefinition> enemies_;
};
