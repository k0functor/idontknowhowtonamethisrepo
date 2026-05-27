#pragma once

#include "settings/UserSettings.hpp"

#include <filesystem>

class UserSettingsRepository {
public:
    static UserSettings loadOrCreate(
        const std::filesystem::path& filePath,
        const UserSettings& defaults
    );

    static UserSettings load(
        const std::filesystem::path& filePath,
        const UserSettings& defaults
    );

    static void save(
        const std::filesystem::path& filePath,
        const UserSettings& settings
    );
};
