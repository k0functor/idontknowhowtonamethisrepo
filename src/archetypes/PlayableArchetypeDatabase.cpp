#include "PlayableArchetypeDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/PlayableArchetypeParser.hpp"

#include <algorithm>
#include <stdexcept>

void PlayableArchetypeDatabase::clear() {
    archetypes_.clear();
}

void PlayableArchetypeDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& item : root) {
        add(PlayableArchetypeParser::parse(item, filePath));
    }
}

void PlayableArchetypeDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
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

void PlayableArchetypeDatabase::add(PlayableArchetypeDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Playable archetype id must not be empty");
    }

    if (archetypes_.contains(id)) {
        throw std::runtime_error("Duplicate playable archetype id: " + id);
    }

    archetypes_.emplace(id, std::move(definition));
}

bool PlayableArchetypeDatabase::contains(const PlayableArchetypeId& id) const {
    return archetypes_.contains(id.value);
}

const PlayableArchetypeDefinition& PlayableArchetypeDatabase::get(const PlayableArchetypeId& id) const {
    const auto iterator = archetypes_.find(id.value);

    if (iterator == archetypes_.end()) {
        throw std::runtime_error("Unknown playable archetype id: " + id.value);
    }

    return iterator->second;
}

std::vector<const PlayableArchetypeDefinition*> PlayableArchetypeDatabase::all() const {
    std::vector<const PlayableArchetypeDefinition*> result;
    result.reserve(archetypes_.size());

    for (const auto& [_, definition] : archetypes_) {
        result.push_back(&definition);
    }

    std::sort(result.begin(), result.end(), [](
        const PlayableArchetypeDefinition* left,
        const PlayableArchetypeDefinition* right
    ) {
        if (left->selectionOrder != right->selectionOrder) {
            return left->selectionOrder < right->selectionOrder;
        }

        return left->id.value < right->id.value;
    });

    return result;
}

std::size_t PlayableArchetypeDatabase::size() const {
    return archetypes_.size();
}
