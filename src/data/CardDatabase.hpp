#pragma once

#include "cards/CardDefinition.hpp"
#include "cards/CardId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class CardDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(CardDefinition definition);

    bool contains(const CardId& id) const;
    const CardDefinition& get(const CardId& id) const;

    std::vector<const CardDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, CardDefinition> cards_;
};
