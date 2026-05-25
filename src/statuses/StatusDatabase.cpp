#include "StatusDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/StatusDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void StatusDatabase::clear() {
    statuses_.clear();
}

void StatusDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& statusJson : root) {
        add(StatusDefinitionParser::parse(statusJson, filePath));
    }
}

void StatusDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        throw std::runtime_error(
            "Status directory does not exist: '" + directoryPath.string() + "'"
        );
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error(
            "Status path is not a directory: '" + directoryPath.string() + "'"
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

void StatusDatabase::add(StatusDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Status id must not be empty");
    }

    if (statuses_.contains(id)) {
        throw std::runtime_error("Duplicate status id: '" + id + "'");
    }

    statuses_.emplace(id, std::move(definition));
}

bool StatusDatabase::contains(const StatusId& id) const {
    return statuses_.contains(id.value);
}

const StatusDefinition& StatusDatabase::get(const StatusId& id) const {
    const auto iterator = statuses_.find(id.value);

    if (iterator == statuses_.end()) {
        throw std::runtime_error("Unknown status id: '" + id.value + "'");
    }

    return iterator->second;
}

std::vector<const StatusDefinition*> StatusDatabase::all() const {
    std::vector<const StatusDefinition*> result;
    result.reserve(statuses_.size());

    for (const auto& [id, definition] : statuses_) {
        result.push_back(&definition);
    }

    return result;
}

std::size_t StatusDatabase::size() const {
    return statuses_.size();
}
