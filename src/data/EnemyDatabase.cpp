#include "EnemyDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/EnemyDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void EnemyDatabase::clear() {
    enemies_.clear();
}

void EnemyDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& enemyJson : root) {
        add(EnemyDefinitionParser::parse(enemyJson, filePath));
    }
}

void EnemyDatabase::loadFromDirectory(
    const std::filesystem::path& directoryPath
) {
    if (!std::filesystem::exists(directoryPath)) {
        throw std::runtime_error(
            "Enemy directory does not exist: '" + directoryPath.string() + "'"
        );
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error(
            "Enemy path is not a directory: '" + directoryPath.string() + "'"
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

void EnemyDatabase::add(EnemyDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Enemy id must not be empty");
    }

    if (enemies_.contains(id)) {
        throw std::runtime_error("Duplicate enemy id: '" + id + "'");
    }

    enemies_.emplace(id, std::move(definition));
}

bool EnemyDatabase::contains(const EnemyId& id) const {
    return enemies_.contains(id.value);
}

const EnemyDefinition& EnemyDatabase::get(const EnemyId& id) const {
    const auto iterator = enemies_.find(id.value);

    if (iterator == enemies_.end()) {
        throw std::runtime_error("Unknown enemy id: '" + id.value + "'");
    }

    return iterator->second;
}

std::vector<const EnemyDefinition*> EnemyDatabase::all() const {
    std::vector<const EnemyDefinition*> result;
    result.reserve(enemies_.size());

    for (const auto& [id, definition] : enemies_) {
        result.push_back(&definition);
    }

    return result;
}

std::size_t EnemyDatabase::size() const {
    return enemies_.size();
}
