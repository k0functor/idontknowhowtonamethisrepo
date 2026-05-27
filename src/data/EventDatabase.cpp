#include "EventDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/RunEventDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void EventDatabase::clear() {
    events_.clear();
}

void EventDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& eventJson : root) {
        add(RunEventDefinitionParser::parse(eventJson, filePath));
    }
}

void EventDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        throw std::runtime_error("Event directory does not exist: '" + directoryPath.string() + "'");
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Event path is not a directory: '" + directoryPath.string() + "'");
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

void EventDatabase::add(RunEventDefinition definition) {
    if (definition.id.empty()) {
        throw std::runtime_error("Event id must not be empty");
    }

    if (events_.contains(definition.id)) {
        throw std::runtime_error("Duplicate event id: '" + definition.id + "'");
    }

    events_.emplace(definition.id, std::move(definition));
}

bool EventDatabase::contains(const std::string& id) const {
    return events_.contains(id);
}

const RunEventDefinition& EventDatabase::get(const std::string& id) const {
    const auto iterator = events_.find(id);
    if (iterator == events_.end()) {
        throw std::runtime_error("Unknown event id: '" + id + "'");
    }

    return iterator->second;
}

std::vector<const RunEventDefinition*> EventDatabase::all() const {
    std::vector<const RunEventDefinition*> result;
    result.reserve(events_.size());

    for (const auto& [id, definition] : events_) {
        (void)id;
        result.push_back(&definition);
    }

    return result;
}

std::size_t EventDatabase::size() const {
    return events_.size();
}
