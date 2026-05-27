#include "UserSettingsRepository.hpp"

#include "data/JsonLoader.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
using Json = nlohmann::json;

[[noreturn]] void throwSettingsError(
    const std::filesystem::path& filePath,
    const std::string& message
) {
    throw std::runtime_error(
        "Settings error in '" + filePath.string() + "': " + message
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
        throwSettingsError(filePath, "'" + key + "' must be an object");
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
        throwSettingsError(filePath, "'" + key + "' must be a string");
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
        throwSettingsError(filePath, "'" + key + "' must be a boolean");
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
        throwSettingsError(filePath, "'" + key + "' must be an integer");
    }

    const long long number = value.get<long long>();
    if (number < static_cast<long long>(minValue) || number > static_cast<long long>(maxValue)) {
        throwSettingsError(
            filePath,
            "'" + key + "' must be between " + std::to_string(minValue) +
                " and " + std::to_string(maxValue)
        );
    }

    return static_cast<std::uint32_t>(number);
}

float readFloat(
    const Json& object,
    const std::string& key,
    float currentValue,
    float minValue,
    float maxValue,
    const std::filesystem::path& filePath
) {
    if (!object.contains(key)) {
        return currentValue;
    }

    const Json& value = object.at(key);
    if (!value.is_number()) {
        throwSettingsError(filePath, "'" + key + "' must be a number");
    }

    const float number = value.get<float>();
    if (number < minValue || number > maxValue) {
        throwSettingsError(
            filePath,
            "'" + key + "' must be between " + std::to_string(minValue) +
                " and " + std::to_string(maxValue)
        );
    }

    return number;
}

Locale readLocale(
    const Json& root,
    const Locale& currentValue,
    const std::filesystem::path& filePath
) {
    const std::string code = readString(root, "locale", currentValue.code(), filePath);
    if (code != Locale::russian().code() && code != Locale::english().code()) {
        throwSettingsError(filePath, "'locale' must be 'ru' or 'en'");
    }

    return Locale(code);
}

Json toJson(const UserSettings& settings) {
    return Json{
        {"version", 1},
        {"locale", settings.locale.code()},
        {"window", Json{
            {"width", settings.window.width},
            {"height", settings.window.height},
            {"fullscreen", settings.window.fullscreen},
            {"vertical_sync", settings.window.verticalSync},
            {"framerate_limit", settings.window.frameRateLimit}
        }},
        {"audio", Json{
            {"master_volume", settings.audio.masterVolume},
            {"music_volume", settings.audio.musicVolume},
            {"sfx_volume", settings.audio.sfxVolume}
        }},
        {"debug", Json{
            {"enabled", settings.debug.enabled}
        }}
    };
}
}

UserSettings UserSettingsRepository::loadOrCreate(
    const std::filesystem::path& filePath,
    const UserSettings& defaults
) {
    if (!std::filesystem::exists(filePath)) {
        save(filePath, defaults);
        return defaults;
    }

    return load(filePath, defaults);
}

UserSettings UserSettingsRepository::load(
    const std::filesystem::path& filePath,
    const UserSettings& defaults
) {
    const Json root = JsonLoader::loadObjectFromFile(filePath);

    UserSettings settings = defaults;

    settings.locale = readLocale(root, settings.locale, filePath);

    const Json& window = readOptionalObject(root, "window", filePath);
    settings.window.width = readUnsigned(window, "width", settings.window.width, 320, 7680, filePath);
    settings.window.height = readUnsigned(window, "height", settings.window.height, 240, 4320, filePath);
    settings.window.fullscreen = readBool(window, "fullscreen", settings.window.fullscreen, filePath);
    settings.window.verticalSync = readBool(window, "vertical_sync", settings.window.verticalSync, filePath);
    settings.window.frameRateLimit = readUnsigned(
        window,
        "framerate_limit",
        settings.window.frameRateLimit,
        0,
        1000,
        filePath
    );

    const Json& audio = readOptionalObject(root, "audio", filePath);
    settings.audio.masterVolume = readFloat(audio, "master_volume", settings.audio.masterVolume, 0.f, 1.f, filePath);
    settings.audio.musicVolume = readFloat(audio, "music_volume", settings.audio.musicVolume, 0.f, 1.f, filePath);
    settings.audio.sfxVolume = readFloat(audio, "sfx_volume", settings.audio.sfxVolume, 0.f, 1.f, filePath);

    const Json& debug = readOptionalObject(root, "debug", filePath);
    settings.debug.enabled = readBool(debug, "enabled", settings.debug.enabled, filePath);

    return settings;
}

void UserSettingsRepository::save(
    const std::filesystem::path& filePath,
    const UserSettings& settings
) {
    if (!filePath.parent_path().empty()) {
        std::filesystem::create_directories(filePath.parent_path());
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open settings file '" + filePath.string() + "' for writing");
    }

    file << toJson(settings).dump(4) << '\n';
}
