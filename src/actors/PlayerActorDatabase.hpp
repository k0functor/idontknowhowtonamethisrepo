#pragma once

#include "actors/PlayerActorDefinition.hpp"
#include "actors/PlayerActorId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class PlayerActorDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(PlayerActorDefinition definition);

    bool contains(const PlayerActorId& id) const;
    const PlayerActorDefinition& get(const PlayerActorId& id) const;

    std::vector<const PlayerActorDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, PlayerActorDefinition> actors_;
};
