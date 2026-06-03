#include "FloorDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/JsonReader.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {
FloorDefinition parseFloorDefinition(
    const Json& json,
    const std::filesystem::path& filePath,
    const std::size_t index
) {
    if (!json.is_object()) {
        throw std::runtime_error(filePath.string() + ": floor entry " + std::to_string(index) + " must be an object");
    }

    const JsonReader reader(json, filePath);

    FloorDefinition definition;
    definition.id = reader.requiredString("id");
    definition.index = reader.requiredInt("index");
    definition.act = reader.optionalInt("act", definition.index);
    definition.isImplemented = reader.optionalBool("is_implemented", true);
    definition.nameTextId = reader.requiredString("name_text_id");
    definition.themeId = reader.optionalString("theme_id", {});
    definition.mapConfigId = reader.optionalString("map_config_id", {});
    definition.mapConfigPath = reader.optionalString("map_config_path", {});
    definition.encounterTableId = reader.optionalString("encounter_table_id", {});
    definition.encounterTablePath = reader.optionalString("encounter_table_path", {});
    definition.eventPoolId = reader.optionalString("event_pool_id", {});
    definition.nextFloorId = reader.optionalString("next_floor_id", {});

    if (definition.id.empty()) {
        throw std::runtime_error(filePath.string() + ": floor entry " + std::to_string(index) + " has empty id");
    }

    if (definition.index <= 0) {
        throw std::runtime_error(filePath.string() + ": floor '" + definition.id + "' index must be positive");
    }

    if (definition.act <= 0) {
        throw std::runtime_error(filePath.string() + ": floor '" + definition.id + "' act must be positive");
    }

    if (definition.nameTextId.empty()) {
        throw std::runtime_error(filePath.string() + ": floor '" + definition.id + "' name_text_id must not be empty");
    }

    if (definition.isImplemented) {
        if (definition.mapConfigId.empty()) {
            throw std::runtime_error(filePath.string() + ": implemented floor '" + definition.id + "' must define map_config_id");
        }
        if (definition.mapConfigPath.empty()) {
            throw std::runtime_error(filePath.string() + ": implemented floor '" + definition.id + "' must define map_config_path");
        }
        if (definition.encounterTableId.empty()) {
            throw std::runtime_error(filePath.string() + ": implemented floor '" + definition.id + "' must define encounter_table_id");
        }
        if (definition.encounterTablePath.empty()) {
            throw std::runtime_error(filePath.string() + ": implemented floor '" + definition.id + "' must define encounter_table_path");
        }
        if (definition.eventPoolId.empty()) {
            throw std::runtime_error(filePath.string() + ": implemented floor '" + definition.id + "' must define event_pool_id");
        }
    }

    return definition;
}
}

void FloorDatabase::clear() {
    floors_.clear();
}

void FloorDatabase::loadFromFile(const std::filesystem::path& filePath) {
    clear();

    const Json root = JsonLoader::loadObjectFromFile(filePath);
    const JsonReader reader(root, filePath);
    const Json& floors = reader.requiredArray("floors");

    if (floors.size() == 0) {
        throw std::runtime_error(filePath.string() + ": floors must not be empty");
    }

    floors_.reserve(floors.size());
    std::unordered_set<std::string> ids;
    std::unordered_set<int> indices;

    for (std::size_t index = 0; index < floors.size(); ++index) {
        FloorDefinition floor = parseFloorDefinition(floors.at(index), filePath, index);
        if (!ids.insert(floor.id).second) {
            throw std::runtime_error(filePath.string() + ": duplicate floor id '" + floor.id + "'");
        }
        if (!indices.insert(floor.index).second) {
            throw std::runtime_error(filePath.string() + ": duplicate floor index " + std::to_string(floor.index));
        }
        floors_.push_back(std::move(floor));
    }

    for (const FloorDefinition& floor : floors_) {
        if (!floor.nextFloorId.empty() && !contains(floor.nextFloorId)) {
            throw std::runtime_error(filePath.string() + ": floor '" + floor.id + "' references unknown next_floor_id '" + floor.nextFloorId + "'");
        }
    }

    const bool hasImplementedFloor = std::any_of(floors_.begin(), floors_.end(), [](const FloorDefinition& floor) {
        return floor.isImplemented;
    });
    if (!hasImplementedFloor) {
        throw std::runtime_error(filePath.string() + ": at least one floor must be implemented");
    }
}

bool FloorDatabase::contains(const std::string& id) const {
    return std::any_of(floors_.begin(), floors_.end(), [&id](const FloorDefinition& floor) {
        return floor.id == id;
    });
}

const FloorDefinition& FloorDatabase::get(const std::string& id) const {
    for (const FloorDefinition& floor : floors_) {
        if (floor.id == id) {
            return floor;
        }
    }

    throw std::runtime_error("Unknown floor id: " + id);
}

const FloorDefinition& FloorDatabase::startingFloor() const {
    const FloorDefinition* best = nullptr;
    for (const FloorDefinition& floor : floors_) {
        if (!floor.isImplemented) {
            continue;
        }

        if (best == nullptr || floor.index < best->index) {
            best = &floor;
        }
    }

    if (best == nullptr) {
        throw std::runtime_error("FloorDatabase has no implemented starting floor");
    }

    return *best;
}

std::vector<const FloorDefinition*> FloorDatabase::all() const {
    std::vector<const FloorDefinition*> result;
    result.reserve(floors_.size());
    for (const FloorDefinition& floor : floors_) {
        result.push_back(&floor);
    }
    return result;
}

std::size_t FloorDatabase::size() const {
    return floors_.size();
}
