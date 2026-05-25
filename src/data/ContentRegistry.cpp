#include "ContentRegistry.hpp"

void ContentRegistry::clear() {
    cards_.clear();
    enemies_.clear();
}

void ContentRegistry::loadFromDataDirectory(const std::filesystem::path& dataDirectory) {
    clear();

    cards_.loadFromDirectory(dataDirectory / "cards");
    enemies_.loadFromDirectory(dataDirectory / "enemies");
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
