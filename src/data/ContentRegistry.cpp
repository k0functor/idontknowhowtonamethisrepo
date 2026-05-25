#include "ContentRegistry.hpp"

void ContentRegistry::clear() {
    cards_.clear();
    enemies_.clear();
    statuses_.clear();
    archetypes_.clear();
    actors_.clear();
    difficulties_.clear();
}

void ContentRegistry::loadFromDataDirectory(const std::filesystem::path& dataDirectory) {
    clear();

    statuses_.loadFromDirectory(dataDirectory / "statuses");
    cards_.loadFromDirectory(dataDirectory / "cards");
    enemies_.loadFromDirectory(dataDirectory / "enemies");
    actors_.loadFromDirectory(dataDirectory / "actors");
    archetypes_.loadFromDirectory(dataDirectory / "archetypes");
    difficulties_.loadFromDirectory(dataDirectory / "run");
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
