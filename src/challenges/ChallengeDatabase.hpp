#pragma once

#include "challenges/ChallengeDefinition.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class ChallengeDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(ChallengeDefinition definition);

    bool contains(const std::string& id) const;
    const ChallengeDefinition& get(const std::string& id) const;

    std::vector<const ChallengeDefinition*> all() const;
    std::size_t size() const;

private:
    std::unordered_map<std::string, ChallengeDefinition> challenges_;
};
