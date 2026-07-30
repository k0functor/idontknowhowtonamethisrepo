#include "AchievementDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/AchievementDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

void AchievementDatabase::clear() {
    achievements_.clear();
}

void AchievementDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& item : root) {
        add(AchievementDefinitionParser::parse(item, filePath));
    }
}

void AchievementDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        return;
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Achievement path is not a directory: '" + directoryPath.string() + "'");
    }

    std::vector<std::filesystem::path> files;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& filePath : files) {
        loadFromFile(filePath);
    }
}

void AchievementDatabase::add(AchievementDefinition definition) {
    if (definition.id.empty()) {
        throw std::runtime_error("Achievement id must not be empty");
    }

    if (achievements_.contains(definition.id)) {
        throw std::runtime_error("Duplicate achievement id: '" + definition.id + "'");
    }

    achievements_.emplace(definition.id, std::move(definition));
}

bool AchievementDatabase::contains(const std::string& id) const {
    return achievements_.contains(id);
}

const AchievementDefinition& AchievementDatabase::get(const std::string& id) const {
    const auto iterator = achievements_.find(id);
    if (iterator == achievements_.end()) {
        throw std::runtime_error("Unknown achievement id: '" + id + "'");
    }

    return iterator->second;
}

std::vector<const AchievementDefinition*> AchievementDatabase::all() const {
    std::vector<const AchievementDefinition*> result;
    result.reserve(achievements_.size());

    for (const auto& [id, definition] : achievements_) {
        (void)id;
        result.push_back(&definition);
    }

    std::sort(result.begin(), result.end(), [](const AchievementDefinition* left, const AchievementDefinition* right) {
        if (left->selectionOrder != right->selectionOrder) {
            return left->selectionOrder < right->selectionOrder;
        }
        return left->id < right->id;
    });

    return result;
}

std::size_t AchievementDatabase::size() const {
    return achievements_.size();
}
