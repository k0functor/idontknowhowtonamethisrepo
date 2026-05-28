#include "ContentRegistry.hpp"

void ContentRegistry::clear() {
    cards_.clear();
    enemies_.clear();
    statuses_.clear();
    relics_.clear();
    archetypes_.clear();
    actors_.clear();
    difficulties_.clear();
    consumables_.clear();
    drones_.clear();
    events_.clear();
    encounters_.clear();
    rewardTuning_ = RewardTuning{};
    shopTuning_ = ShopTuning{};
    actOneMapGeneration_ = RunMapGenerationConfig{};
}

void ContentRegistry::loadFromDataDirectory(const std::filesystem::path& dataDirectory) {
    clear();

    statuses_.loadFromDirectory(dataDirectory / "statuses");
    relics_.loadFromDirectory(dataDirectory / "relics");
    cards_.loadFromDirectory(dataDirectory / "cards");
    enemies_.loadFromDirectory(dataDirectory / "enemies");
    actors_.loadFromDirectory(dataDirectory / "actors");
    archetypes_.loadFromDirectory(dataDirectory / "archetypes");
    difficulties_.loadFromDirectory(dataDirectory / "run");
    consumables_.loadFromDirectory(dataDirectory / "consumables");
    drones_.loadFromDirectory(dataDirectory / "drones");
    events_.loadFromDirectory(dataDirectory / "events");
    encounters_.loadFromFile(dataDirectory / "encounters" / "act1_encounters.json");
    rewardTuning_.loadFromFile(dataDirectory / "rewards" / "reward_tables.json");
    shopTuning_.loadFromFile(dataDirectory / "shop" / "shop_tables.json");
    actOneMapGeneration_.loadFromFile(dataDirectory / "run" / "acts" / "act1.json");
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


const DroneDatabase& ContentRegistry::drones() const {
    return drones_;
}

DroneDatabase& ContentRegistry::drones() {
    return drones_;
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

const RunMapGenerationConfig& ContentRegistry::actOneMapGeneration() const {
    return actOneMapGeneration_;
}

RunMapGenerationConfig& ContentRegistry::actOneMapGeneration() {
    return actOneMapGeneration_;
}
