#include "ActiveItemDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/ActiveItemDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void ActiveItemDatabase::clear() {
    items_.clear();
}

void ActiveItemDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);
    for (const Json& item : root) {
        add(ActiveItemDefinitionParser::parse(item, filePath));
    }
}

void ActiveItemDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) return;
    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Active item path is not a directory: '" + directoryPath.string() + "'");
    }

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const auto& file : files) loadFromFile(file);
}

void ActiveItemDatabase::add(ActiveItemDefinition definition) {
    const std::string id = definition.id.value;
    if (id.empty()) throw std::runtime_error("Active item id must not be empty");
    if (items_.contains(id)) throw std::runtime_error("Duplicate active item id: " + id);
    items_.emplace(id, std::move(definition));
}

bool ActiveItemDatabase::contains(const ActiveItemId& id) const {
    return items_.contains(id.value);
}

const ActiveItemDefinition& ActiveItemDatabase::get(const ActiveItemId& id) const {
    const auto iterator = items_.find(id.value);
    if (iterator == items_.end()) throw std::runtime_error("Unknown active item id: " + id.value);
    return iterator->second;
}

std::vector<const ActiveItemDefinition*> ActiveItemDatabase::all() const {
    std::vector<const ActiveItemDefinition*> result;
    result.reserve(items_.size());
    for (const auto& [_, definition] : items_) result.push_back(&definition);
    return result;
}

std::size_t ActiveItemDatabase::size() const {
    return items_.size();
}
