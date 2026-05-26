#include "ConsumableDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/ConsumableDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void ConsumableDatabase::clear() {
    consumables_.clear();
}

void ConsumableDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& item : root) {
        add(ConsumableDefinitionParser::parse(item, filePath));
    }
}

void ConsumableDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        return;
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Consumable path is not a directory: '" + directoryPath.string() + "'");
    }

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    for (const std::filesystem::path& file : files) {
        loadFromFile(file);
    }
}

void ConsumableDatabase::add(ConsumableDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Consumable id must not be empty");
    }

    if (consumables_.contains(id)) {
        throw std::runtime_error("Duplicate consumable id: " + id);
    }

    consumables_.emplace(id, std::move(definition));
}

bool ConsumableDatabase::contains(const ConsumableId& id) const {
    return consumables_.contains(id.value);
}

const ConsumableDefinition& ConsumableDatabase::get(const ConsumableId& id) const {
    const auto iterator = consumables_.find(id.value);

    if (iterator == consumables_.end()) {
        throw std::runtime_error("Unknown consumable id: " + id.value);
    }

    return iterator->second;
}

std::vector<const ConsumableDefinition*> ConsumableDatabase::all() const {
    std::vector<const ConsumableDefinition*> result;
    result.reserve(consumables_.size());

    for (const auto& [_, definition] : consumables_) {
        result.push_back(&definition);
    }

    return result;
}

std::size_t ConsumableDatabase::size() const {
    return consumables_.size();
}
