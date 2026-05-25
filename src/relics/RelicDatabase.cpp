#include "RelicDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/RelicDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void RelicDatabase::clear() {
    relics_.clear();
}

void RelicDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& relicJson : root) {
        add(RelicDefinitionParser::parse(relicJson, filePath));
    }
}

void RelicDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        throw std::runtime_error(
            "Relic directory does not exist: '" + directoryPath.string() + "'"
        );
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error(
            "Relic path is not a directory: '" + directoryPath.string() + "'"
        );
    }

    std::vector<std::filesystem::path> files;

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    for (const std::filesystem::path& file : files) {
        loadFromFile(file);
    }
}

void RelicDatabase::add(RelicDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Relic id must not be empty");
    }

    if (relics_.contains(id)) {
        throw std::runtime_error("Duplicate relic id: '" + id + "'");
    }

    relics_.emplace(id, std::move(definition));
}

bool RelicDatabase::contains(const RelicId& id) const {
    return relics_.contains(id.value);
}

const RelicDefinition& RelicDatabase::get(const RelicId& id) const {
    const auto iterator = relics_.find(id.value);

    if (iterator == relics_.end()) {
        throw std::runtime_error("Unknown relic id: '" + id.value + "'");
    }

    return iterator->second;
}

std::vector<const RelicDefinition*> RelicDatabase::all() const {
    std::vector<const RelicDefinition*> result;
    result.reserve(relics_.size());

    for (const auto& [id, definition] : relics_) {
        result.push_back(&definition);
    }

    return result;
}

std::size_t RelicDatabase::size() const {
    return relics_.size();
}
