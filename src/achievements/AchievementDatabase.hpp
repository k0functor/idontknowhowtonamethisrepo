#pragma once

#include "achievements/AchievementDefinition.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class AchievementDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(AchievementDefinition definition);

    bool contains(const std::string& id) const;
    const AchievementDefinition& get(const std::string& id) const;

    std::vector<const AchievementDefinition*> all() const;
    std::size_t size() const;

private:
    std::unordered_map<std::string, AchievementDefinition> achievements_;
};
