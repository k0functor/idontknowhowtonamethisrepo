#include "PlayerActorDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/PlayerActorParser.hpp"

#include <stdexcept>

void PlayerActorDatabase::clear() {
    actors_.clear();
}

void PlayerActorDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& item : root) {
        add(PlayerActorParser::parse(item, filePath));
    }
}

void PlayerActorDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        return;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }

        loadFromFile(entry.path());
    }
}

void PlayerActorDatabase::add(PlayerActorDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Player actor id must not be empty");
    }

    if (actors_.contains(id)) {
        throw std::runtime_error("Duplicate player actor id: " + id);
    }

    actors_.emplace(id, std::move(definition));
}

bool PlayerActorDatabase::contains(const PlayerActorId& id) const {
    return actors_.contains(id.value);
}

const PlayerActorDefinition& PlayerActorDatabase::get(const PlayerActorId& id) const {
    const auto iterator = actors_.find(id.value);

    if (iterator == actors_.end()) {
        throw std::runtime_error("Unknown player actor id: " + id.value);
    }

    return iterator->second;
}

std::vector<const PlayerActorDefinition*> PlayerActorDatabase::all() const {
    std::vector<const PlayerActorDefinition*> result;
    result.reserve(actors_.size());

    for (const auto& [_, definition] : actors_) {
        result.push_back(&definition);
    }

    return result;
}

std::size_t PlayerActorDatabase::size() const {
    return actors_.size();
}
