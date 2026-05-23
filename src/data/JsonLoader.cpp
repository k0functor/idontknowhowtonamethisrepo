#include "JsonLoader.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace {
    std::string makePathMessage(const std::filesystem::path& filePath) {
        return "'" + filePath.string() + "'";
    }

    std::string makeCurrentDirectoryMessage() {
        return "Current working directory: '" + std::filesystem::current_path().string() + "'";
    }
}

Json JsonLoader::loadFromFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open JSON file " + makePathMessage(filePath) + ". " +
            makeCurrentDirectoryMessage()
        );
    }

    Json json;

    try {
        file >> json;
    } catch (const Json::parse_error& error) {
        throw std::runtime_error(
            "Failed to parse JSON file " + makePathMessage(filePath) + ": " +
            std::string(error.what())
        );
    }

    return json;
}

Json JsonLoader::loadObjectFromFile(const std::filesystem::path& filePath) {
    const Json json = loadFromFile(filePath);

    if (!json.is_object()) {
        throw std::runtime_error(
            "JSON file " + makePathMessage(filePath) + " must contain an object as root"
        );
    }

    return json;
}

Json JsonLoader::loadArrayFromFile(const std::filesystem::path& filePath) {
    const Json json = loadFromFile(filePath);

    if (!json.is_array()) {
        throw std::runtime_error(
            "JSON file " + makePathMessage(filePath) + " must contain an array as root"
        );
    }

    return json;
}
