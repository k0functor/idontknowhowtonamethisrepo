#include "DroneDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/DroneDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void DroneDatabase::clear() {
    drones_.clear();
}

void DroneDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& droneJson : root) {
        add(DroneDefinitionParser::parse(droneJson, filePath));
    }
}

void DroneDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        return;
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error(
            "Drone path is not a directory: '" + directoryPath.string() + "'"
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

void DroneDatabase::add(DroneDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Drone id must not be empty");
    }

    if (drones_.contains(id)) {
        throw std::runtime_error("Duplicate drone id: '" + id + "'");
    }

    drones_.emplace(id, std::move(definition));
}

bool DroneDatabase::contains(const DroneId& id) const {
    return drones_.contains(id.value);
}

const DroneDefinition& DroneDatabase::get(const DroneId& id) const {
    const auto iterator = drones_.find(id.value);

    if (iterator == drones_.end()) {
        throw std::runtime_error("Unknown drone id: '" + id.value + "'");
    }

    return iterator->second;
}

std::vector<const DroneDefinition*> DroneDatabase::all() const {
    std::vector<const DroneDefinition*> result;
    result.reserve(drones_.size());

    for (const auto& [id, definition] : drones_) {
        (void)id;
        result.push_back(&definition);
    }

    return result;
}

std::size_t DroneDatabase::size() const {
    return drones_.size();
}
