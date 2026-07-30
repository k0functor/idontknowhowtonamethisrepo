#pragma once

#include "profile/ProfileData.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

struct ProfileSaveLoadResult {
    bool loaded = false;
    bool loadedFromBackup = false;
    ProfileData profile;
    std::string errorMessage;
};

class ProfileSaveSystem {
public:
    explicit ProfileSaveSystem(std::filesystem::path savesRoot = "saves");

    void setSavesRoot(std::filesystem::path savesRoot);

    bool hasProfileSave(std::size_t slotIndex) const;
    std::filesystem::path savePath(std::size_t slotIndex) const;
    std::filesystem::path backupSavePath(std::size_t slotIndex) const;
    std::filesystem::path temporarySavePath(std::size_t slotIndex) const;

    void saveProfile(std::size_t slotIndex, const ProfileData& profile) const;
    ProfileData loadProfile(std::size_t slotIndex) const;
    ProfileSaveLoadResult tryLoadProfile(std::size_t slotIndex) const;
    void restoreBackupAsPrimary(std::size_t slotIndex) const;
    void deleteProfile(std::size_t slotIndex) const;

private:
    std::filesystem::path slotDirectory(std::size_t slotIndex) const;

private:
    std::filesystem::path savesRoot_;
};
