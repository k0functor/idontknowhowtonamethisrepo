#include "AppConfig.hpp"

#include <data/JsonLoader.hpp>

#include <limits>
#include <stdexcept>
#include <string>

namespace {
using Json = nlohmann::json;

[[noreturn]] void throwConfigError(
    const std::filesystem::path& filePath,
    const std::string& message
) {
    throw std::runtime_error(
        "Config error in '" + filePath.string() + "': " + message
    );
}

const Json& readOptionalObject(
    const Json& parent,
    const std::string& key,
    const std::filesystem::path& filePath
) {
    static const Json emptyObject = Json::object();

    if (!parent.contains(key)) {
        return emptyObject;
    }

    const Json& value = parent.at(key);

    if (!value.is_object()) {
        throwConfigError(filePath, "'" + std::string(key) + "' must be an object");
    }

    return value;
}

std::string readString(
    const Json& object,
    const std::string& key,
    std::string currentValue,
    const std::filesystem::path& filePath
) {
    if (!object.contains(key)) {
        return currentValue;
    }

    const Json& value = object.at(key);

    if (!value.is_string()) {
        throwConfigError(filePath, "'" + std::string(key) + "' must be a string");
    }

    return value.get<std::string>();
}

bool readBool(
    const Json& object,
    const std::string& key,
    bool currentValue,
    const std::filesystem::path& filePath
) {
    if (!object.contains(key)) {
        return currentValue;
    }

    const Json& value = object.at(key);

    if (!value.is_boolean()) {
        throwConfigError(filePath, "'" + std::string(key) + "' must be a boolean");
    }

    return value.get<bool>();
}

std::uint32_t readUnsigned(
    const Json& object,
    const std::string& key,
    std::uint32_t currentValue,
    std::uint32_t minValue,
    std::uint32_t maxValue,
    const std::filesystem::path& filePath
) {
    if (!object.contains(key)) {
        return currentValue;
    }

    const Json& value = object.at(key);

    if (!value.is_number_integer()) {
        throwConfigError(filePath, "'" + std::string(key) + "' must be an integer");
    }

    const long long number = value.get<long long>();

    if (number < static_cast<long long>(minValue) ||
        number > static_cast<long long>(maxValue)) {
        throwConfigError(
            filePath,
            "'" + std::string(key) + "' must be between " +
                std::to_string(minValue) + " and " + std::to_string(maxValue)
        );
    }

    return static_cast<std::uint32_t>(number);
}

std::filesystem::path readPath(
    const Json& object,
    const std::string& key,
    std::filesystem::path currentValue,
    const std::filesystem::path& filePath
) {
    const std::string pathString = readString(
        object,
        key,
        currentValue.string(),
        filePath
    );

    return std::filesystem::path(pathString);
}
}

AppConfig AppConfig::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadObjectFromFile(filePath);

    AppConfig config;

    const Json& window = readOptionalObject(root, "window", filePath);

    config.window.title = readString(
        window,
        "title",
        config.window.title,
        filePath
    );

    config.window.width = readUnsigned(
        window,
        "width",
        config.window.width,
        320,
        7680,
        filePath
    );

    config.window.height = readUnsigned(
        window,
        "height",
        config.window.height,
        240,
        4320,
        filePath
    );

    config.window.fullscreen = readBool(
        window,
        "fullscreen",
        config.window.fullscreen,
        filePath
    );

    config.window.verticalSync = readBool(
        window,
        "vertical_sync",
        config.window.verticalSync,
        filePath
    );

    config.window.frameRateLimit = readUnsigned(
        window,
        "framerate_limit",
        config.window.frameRateLimit,
        0,
        1000,
        filePath
    );

    config.locale = Locale(
        readString(root, "locale", config.locale.code(), filePath)
    );

    const Json& paths = readOptionalObject(root, "paths", filePath);

    config.paths.assets = readPath(
        paths,
        "assets",
        config.paths.assets,
        filePath
    );

    config.paths.data = readPath(
        paths,
        "data",
        config.paths.data,
        filePath
    );

    config.paths.saves = readPath(
        paths,
        "saves",
        config.paths.saves,
        filePath
    );

    const Json& debug = readOptionalObject(root, "debug", filePath);

    config.debug.enabled = readBool(
        debug,
        "enabled",
        config.debug.enabled,
        filePath
    );

    return config;
}