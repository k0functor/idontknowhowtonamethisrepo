#include "ChallengeDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/ChallengeDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

void ChallengeDatabase::clear() {
    challenges_.clear();
}

void ChallengeDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& item : root) {
        add(ChallengeDefinitionParser::parse(item, filePath));
    }
}

void ChallengeDatabase::loadFromDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::exists(directoryPath)) {
        return;
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Challenge path is not a directory: '" + directoryPath.string() + "'");
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

void ChallengeDatabase::add(ChallengeDefinition definition) {
    if (definition.id.empty()) {
        throw std::runtime_error("Challenge id must not be empty");
    }

    if (challenges_.contains(definition.id)) {
        throw std::runtime_error("Duplicate challenge id: '" + definition.id + "'");
    }

    challenges_.emplace(definition.id, std::move(definition));
}

bool ChallengeDatabase::contains(const std::string& id) const {
    return challenges_.contains(id);
}

const ChallengeDefinition& ChallengeDatabase::get(const std::string& id) const {
    const auto iterator = challenges_.find(id);
    if (iterator == challenges_.end()) {
        throw std::runtime_error("Unknown challenge id: '" + id + "'");
    }

    return iterator->second;
}

std::vector<const ChallengeDefinition*> ChallengeDatabase::all() const {
    std::vector<const ChallengeDefinition*> result;
    result.reserve(challenges_.size());

    for (const auto& [id, definition] : challenges_) {
        (void)id;
        result.push_back(&definition);
    }

    std::sort(result.begin(), result.end(), [](const ChallengeDefinition* left, const ChallengeDefinition* right) {
        if (left->selectionOrder != right->selectionOrder) {
            return left->selectionOrder < right->selectionOrder;
        }
        return left->id < right->id;
    });

    return result;
}

std::size_t ChallengeDatabase::size() const {
    return challenges_.size();
}
