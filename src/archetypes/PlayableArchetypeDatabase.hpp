#pragma once

#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "archetypes/PlayableArchetypeId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class PlayableArchetypeDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(PlayableArchetypeDefinition definition);

    bool contains(const PlayableArchetypeId& id) const;
    const PlayableArchetypeDefinition& get(const PlayableArchetypeId& id) const;

    std::vector<const PlayableArchetypeDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, PlayableArchetypeDefinition> archetypes_;
};
