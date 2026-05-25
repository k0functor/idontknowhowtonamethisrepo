#include "CardDatabase.hpp"

#include "data/JsonLoader.hpp"
#include "data/parsers/CardDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>

void CardDatabase::clear() {
    cards_.clear();
}

void CardDatabase::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadArrayFromFile(filePath);

    for (const Json& cardJson : root) {
        add(CardDefinitionParser::parse(cardJson, filePath));
    }
}

void CardDatabase::loadFromDirectory(
    const std::filesystem::path& directoryPath
) {
    if (!std::filesystem::exists(directoryPath)) {
        throw std::runtime_error(
            "Card directory does not exist: '" + directoryPath.string() + "'"
        );
    }

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error(
            "Card path is not a directory: '" + directoryPath.string() + "'"
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

void CardDatabase::add(CardDefinition definition) {
    const std::string id = definition.id.value;

    if (id.empty()) {
        throw std::runtime_error("Card id must not be empty");
    }

    if (cards_.contains(id)) {
        throw std::runtime_error("Duplicate card id: '" + id + "'");
    }

    cards_.emplace(id, std::move(definition));
}

bool CardDatabase::contains(const CardId& id) const {
    return cards_.contains(id.value);
}

const CardDefinition& CardDatabase::get(const CardId& id) const {
    const auto iterator = cards_.find(id.value);

    if (iterator == cards_.end()) {
        throw std::runtime_error("Unknown card id: '" + id.value + "'");
    }

    return iterator->second;
}

std::vector<const CardDefinition*> CardDatabase::all() const {
    std::vector<const CardDefinition*> result;
    result.reserve(cards_.size());

    for (const auto& [id, definition] : cards_) {
        result.push_back(&definition);
    }

    return result;
}

std::size_t CardDatabase::size() const {
    return cards_.size();
}
