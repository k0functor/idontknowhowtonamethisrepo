#include "DifficultyDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/DifficultyDefinitionParser.hpp"

#include <stdexcept>

void DifficultyDatabase::clear() {
    difficulties_.clear();
}

void DifficultyDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& item : root) {
        add(DifficultyDefinitionParser::parse(item, filePath));
    }
}

void DifficultyDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
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

void DifficultyDatabase::add(DifficultyDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Difficulty id must not be empty");
    }

    if (difficulties_.contains(id)) {
        throw std::runtime_error("Duplicate difficulty id: " + id);
    }

    difficulties_.emplace(id, std::move(definition));
}

bool DifficultyDatabase::contains(const DifficultyId& id) const {
    return difficulties_.contains(id.value);
}

const DifficultyDefinition& DifficultyDatabase::get(const DifficultyId& id) const {
    const auto iterator = difficulties_.find(id.value);

    if (iterator == difficulties_.end()) {
        throw std::runtime_error("Unknown difficulty id: " + id.value);
    }

    return iterator->second;
}

std::vector<const DifficultyDefinition*> DifficultyDatabase::all() const {
    std::vector<const DifficultyDefinition*> result;
    result.reserve(difficulties_.size());

    for (const auto& [_, definition] : difficulties_) {
        result.push_back(&definition);
    }

    return result;
}

std::size_t DifficultyDatabase::size() const {
    return difficulties_.size();
}
