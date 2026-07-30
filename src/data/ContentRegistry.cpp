#include "ContentRegistry.hpp"

#include <stdexcept>

void ContentRegistry::clear() {
    cards_.clear();
    enemies_.clear();
    statuses_.clear();
    relics_.clear();
    archetypes_.clear();
    actors_.clear();
    difficulties_.clear();
    consumables_.clear();
    activeItems_.clear();
    drones_.clear();
    challenges_.clear();
    achievements_.clear();
    events_.clear();
    encounters_.clear();
    rewardTuning_ = RewardTuning{};
    shopTuning_ = ShopTuning{};
    floors_.clear();
    actOneMapGeneration_ = RunMapGenerationConfig{};
    floorMapGeneration_.clear();
    floorEncounters_.clear();
}

void ContentRegistry::loadFromDataDirectory(const std::filesystem::path& dataDirectory) {
    clear();

    statuses_.loadFromDirectory(dataDirectory / "statuses");
    relics_.loadFromDirectory(dataDirectory / "relics");
    cards_.loadFromDirectory(dataDirectory / "cards");
    enemies_.loadFromDirectory(dataDirectory / "enemies");
    actors_.loadFromDirectory(dataDirectory / "actors");
    archetypes_.loadFromDirectory(dataDirectory / "archetypes");
    difficulties_.loadFromFile(dataDirectory / "run" / "difficulties.json");
    consumables_.loadFromDirectory(dataDirectory / "consumables");
    activeItems_.loadFromDirectory(dataDirectory / "active_items");
    drones_.loadFromDirectory(dataDirectory / "drones");
    challenges_.loadFromDirectory(dataDirectory / "challenges");
    achievements_.loadFromDirectory(dataDirectory / "achievements");
    events_.loadFromDirectory(dataDirectory / "events");
    floors_.loadFromFile(dataDirectory / "run" / "floors.json");

    for (const FloorDefinition* floor : floors_.all()) {
        if (floor == nullptr) {
            continue;
        }

        if (!floor->mapConfigPath.empty()) {
            RunMapGenerationConfig mapGeneration;
            mapGeneration.loadFromFile(dataDirectory / "run" / floor->mapConfigPath);
            floorMapGeneration_.emplace(floor->id, std::move(mapGeneration));
        }

        if (!floor->encounterTablePath.empty()) {
            EncounterDatabase encounterTable;
            encounterTable.loadFromFile(dataDirectory / "run" / floor->encounterTablePath);
            floorEncounters_.emplace(floor->id, std::move(encounterTable));
        }
    }

    const FloorDefinition& startingFloor = floors_.startingFloor();
    actOneMapGeneration_ = mapGenerationForFloor(startingFloor.id);
    encounters_ = encountersForFloor(startingFloor.id);

    rewardTuning_.loadFromFile(dataDirectory / "rewards" / "reward_tables.json");
    shopTuning_.loadFromFile(dataDirectory / "shop" / "shop_tables.json");
}

const CardDatabase& ContentRegistry::cards() const {
    return cards_;
}

CardDatabase& ContentRegistry::cards() {
    return cards_;
}

const EnemyDatabase& ContentRegistry::enemies() const {
    return enemies_;
}

EnemyDatabase& ContentRegistry::enemies() {
    return enemies_;
}

const StatusDatabase& ContentRegistry::statuses() const {
    return statuses_;
}

StatusDatabase& ContentRegistry::statuses() {
    return statuses_;
}

const RelicDatabase& ContentRegistry::relics() const {
    return relics_;
}

RelicDatabase& ContentRegistry::relics() {
    return relics_;
}

const PlayableArchetypeDatabase& ContentRegistry::archetypes() const {
    return archetypes_;
}

PlayableArchetypeDatabase& ContentRegistry::archetypes() {
    return archetypes_;
}

const PlayerActorDatabase& ContentRegistry::actors() const {
    return actors_;
}

PlayerActorDatabase& ContentRegistry::actors() {
    return actors_;
}

const DifficultyDatabase& ContentRegistry::difficulties() const {
    return difficulties_;
}

DifficultyDatabase& ContentRegistry::difficulties() {
    return difficulties_;
}

const ConsumableDatabase& ContentRegistry::consumables() const {
    return consumables_;
}

ConsumableDatabase& ContentRegistry::consumables() {
    return consumables_;
}

const ActiveItemDatabase& ContentRegistry::activeItems() const {
    return activeItems_;
}

ActiveItemDatabase& ContentRegistry::activeItems() {
    return activeItems_;
}


const DroneDatabase& ContentRegistry::drones() const {
    return drones_;
}

DroneDatabase& ContentRegistry::drones() {
    return drones_;
}

const ChallengeDatabase& ContentRegistry::challenges() const {
    return challenges_;
}

ChallengeDatabase& ContentRegistry::challenges() {
    return challenges_;
}

const AchievementDatabase& ContentRegistry::achievements() const {
    return achievements_;
}

AchievementDatabase& ContentRegistry::achievements() {
    return achievements_;
}

const EventDatabase& ContentRegistry::events() const {
    return events_;
}

EventDatabase& ContentRegistry::events() {
    return events_;
}

const EncounterDatabase& ContentRegistry::encounters() const {
    return encounters_;
}

EncounterDatabase& ContentRegistry::encounters() {
    return encounters_;
}

const RewardTuning& ContentRegistry::rewardTuning() const {
    return rewardTuning_;
}

RewardTuning& ContentRegistry::rewardTuning() {
    return rewardTuning_;
}

const ShopTuning& ContentRegistry::shopTuning() const {
    return shopTuning_;
}

ShopTuning& ContentRegistry::shopTuning() {
    return shopTuning_;
}


const FloorDatabase& ContentRegistry::floors() const {
    return floors_;
}

FloorDatabase& ContentRegistry::floors() {
    return floors_;
}

const RunMapGenerationConfig& ContentRegistry::actOneMapGeneration() const {
    return actOneMapGeneration_;
}

RunMapGenerationConfig& ContentRegistry::actOneMapGeneration() {
    return actOneMapGeneration_;
}


const EncounterDatabase& ContentRegistry::encountersForFloor(const std::string& floorId) const {
    const auto iterator = floorEncounters_.find(floorId);
    if (iterator != floorEncounters_.end()) {
        return iterator->second;
    }

    throw std::runtime_error("No encounter table loaded for floor: " + floorId);
}

const RunMapGenerationConfig& ContentRegistry::mapGenerationForFloor(const std::string& floorId) const {
    const auto iterator = floorMapGeneration_.find(floorId);
    if (iterator != floorMapGeneration_.end()) {
        return iterator->second;
    }

    throw std::runtime_error("No run map generation config loaded for floor: " + floorId);
}
