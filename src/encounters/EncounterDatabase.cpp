#include "EncounterDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/JsonReader.hpp"

#include <stdexcept>
#include <string>

namespace {
RunMapNodeType nodeTypeFromEncounterKey(const std::string& key) {
    if (key == "combat") return RunMapNodeType::Combat;
    if (key == "elite") return RunMapNodeType::Elite;
    if (key == "boss") return RunMapNodeType::Boss;

    throw std::runtime_error("Unknown encounter pool key: '" + key + "'");
}

EncounterDefinition parseEncounter(
    const Json& json,
    const std::filesystem::path& filePath,
    const RunMapNodeType nodeType
) {
    if (!json.is_object()) {
        throw std::runtime_error(filePath.string() + ": encounter entry must be an object");
    }

    const JsonReader reader(json, filePath);

    EncounterDefinition definition;
    definition.id = reader.requiredString("id");
    definition.nodeType = nodeType;
    definition.enemyIds = reader.requiredStringArray("enemies");
    definition.weight = reader.optionalInt("weight", 1);
    definition.minLayer = reader.optionalInt("min_layer", -1);
    definition.maxLayer = reader.optionalInt("max_layer", -1);

    if (definition.id.empty()) {
        throw std::runtime_error(filePath.string() + ": encounter id must not be empty");
    }

    if (definition.enemyIds.empty()) {
        throw std::runtime_error(filePath.string() + ": encounter '" + definition.id + "' must contain at least one enemy");
    }

    if (definition.enemyIds.size() > EncounterDefinition::MaximumEnemyCount) {
        throw std::runtime_error(
            filePath.string() + ": encounter '" + definition.id + "' contains " +
            std::to_string(definition.enemyIds.size()) + " enemies, but combat supports at most " +
            std::to_string(EncounterDefinition::MaximumEnemyCount)
        );
    }

    if (definition.weight <= 0) {
        throw std::runtime_error(filePath.string() + ": encounter '" + definition.id + "' must have positive weight");
    }

    if (definition.minLayer < -1 || definition.maxLayer < -1) {
        throw std::runtime_error(filePath.string() + ": encounter '" + definition.id + "' has invalid layer limits");
    }

    if (definition.minLayer >= 0 && definition.maxLayer >= 0 && definition.minLayer > definition.maxLayer) {
        throw std::runtime_error(filePath.string() + ": encounter '" + definition.id + "' has min_layer greater than max_layer");
    }

    return definition;
}
}

void EncounterDatabase::clear() {
    combat_.clear();
    elite_.clear();
    boss_.clear();
}

void EncounterDatabase::loadFromFile(const std::filesystem::path& filePath) {
    clear();

    const Json root = JsonLoader::loadObjectFromFile(filePath);
    const JsonReader reader(root, filePath);
    const Json& pools = reader.requiredObject("pools");

    for (const auto& [key, value] : pools.items()) {
        const RunMapNodeType nodeType = nodeTypeFromEncounterKey(key);
        if (!value.is_array()) {
            throw std::runtime_error(filePath.string() + ": encounter pool '" + key + "' must be an array");
        }

        std::vector<EncounterDefinition>& pool = mutablePoolFor(nodeType);
        for (const Json& entry : value) {
            pool.push_back(parseEncounter(entry, filePath, nodeType));
        }
    }

    if (combat_.empty()) {
        throw std::runtime_error(filePath.string() + ": combat encounter pool must not be empty");
    }

    if (elite_.empty()) {
        throw std::runtime_error(filePath.string() + ": elite encounter pool must not be empty");
    }

    if (boss_.empty()) {
        throw std::runtime_error(filePath.string() + ": boss encounter pool must not be empty");
    }
}

const EncounterDefinition& EncounterDatabase::choose(
    const RunMapNodeType nodeType,
    Random& random,
    const int layerIndex
) const {
    const std::vector<EncounterDefinition>& pool = poolFor(nodeType);

    std::vector<const EncounterDefinition*> eligible;
    eligible.reserve(pool.size());
    for (const EncounterDefinition& encounter : pool) {
        if (encounter.isAllowedOnLayer(layerIndex)) {
            eligible.push_back(&encounter);
        }
    }

    if (eligible.empty()) {
        for (const EncounterDefinition& encounter : pool) {
            eligible.push_back(&encounter);
        }
    }

    int totalWeight = 0;
    for (const EncounterDefinition* encounter : eligible) {
        totalWeight += encounter->weight;
    }

    int roll = random.rangeInclusive(1, totalWeight);
    for (const EncounterDefinition* encounter : eligible) {
        roll -= encounter->weight;
        if (roll <= 0) {
            return *encounter;
        }
    }

    return *eligible.back();
}

std::vector<const EncounterDefinition*> EncounterDatabase::all() const {
    std::vector<const EncounterDefinition*> result;
    result.reserve(size());

    for (const EncounterDefinition& encounter : combat_) {
        result.push_back(&encounter);
    }
    for (const EncounterDefinition& encounter : elite_) {
        result.push_back(&encounter);
    }
    for (const EncounterDefinition& encounter : boss_) {
        result.push_back(&encounter);
    }

    return result;
}

std::size_t EncounterDatabase::size() const {
    return combat_.size() + elite_.size() + boss_.size();
}

const std::vector<EncounterDefinition>& EncounterDatabase::poolFor(const RunMapNodeType nodeType) const {
    switch (nodeType) {
        case RunMapNodeType::Elite:
            return elite_;
        case RunMapNodeType::Boss:
            return boss_;
        case RunMapNodeType::Combat:
        case RunMapNodeType::Event:
        case RunMapNodeType::Shop:
        case RunMapNodeType::Chest:
        case RunMapNodeType::Rest:
            return combat_;
    }

    throw std::runtime_error("Unknown RunMapNodeType in EncounterDatabase::poolFor");
}

std::vector<EncounterDefinition>& EncounterDatabase::mutablePoolFor(const RunMapNodeType nodeType) {
    switch (nodeType) {
        case RunMapNodeType::Elite:
            return elite_;
        case RunMapNodeType::Boss:
            return boss_;
        case RunMapNodeType::Combat:
            return combat_;
        case RunMapNodeType::Event:
        case RunMapNodeType::Shop:
        case RunMapNodeType::Chest:
        case RunMapNodeType::Rest:
            break;
    }

    throw std::runtime_error("Unsupported encounter pool node type");
}
