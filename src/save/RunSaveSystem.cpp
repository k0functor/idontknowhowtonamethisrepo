#include "RunSaveSystem.hpp"

#include "data/JsonLoader.hpp"
#include "save/RunStateSerializer.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {
void throwSaveIoError(const std::filesystem::path& path, const std::string& message) {
    throw std::runtime_error("Run save IO error for '" + path.string() + "': " + message);
}

void removeIfExists(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::remove(path, error);
    if (error) {
        throwSaveIoError(path, error.message());
    }
}

void copyFileReplacing(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
) {
    std::error_code error;
    std::filesystem::copy_file(
        source,
        destination,
        std::filesystem::copy_options::overwrite_existing,
        error
    );
    if (error) {
        throwSaveIoError(destination, "failed to copy backup from '" + source.string() + "': " + error.message());
    }
}

RunState loadRunFromPath(const std::filesystem::path& path) {
    return RunStateSerializer::fromJson(JsonLoader::loadObjectFromFile(path), path);
}
}

RunSaveSystem::RunSaveSystem(std::filesystem::path savesRoot)
    : savesRoot_(std::move(savesRoot)) {}

void RunSaveSystem::setSavesRoot(std::filesystem::path savesRoot) {
    savesRoot_ = std::move(savesRoot);
}

bool RunSaveSystem::hasRunSave(const std::size_t slotIndex) const {
    return std::filesystem::is_regular_file(savePath(slotIndex)) ||
           std::filesystem::is_regular_file(backupSavePath(slotIndex));
}

std::filesystem::path RunSaveSystem::savePath(const std::size_t slotIndex) const {
    return slotDirectory(slotIndex) / "run_save.json";
}

std::filesystem::path RunSaveSystem::backupSavePath(const std::size_t slotIndex) const {
    return slotDirectory(slotIndex) / "run_save.backup.json";
}

std::filesystem::path RunSaveSystem::temporarySavePath(const std::size_t slotIndex) const {
    return slotDirectory(slotIndex) / "run_save.tmp.json";
}

void RunSaveSystem::saveRun(const std::size_t slotIndex, const RunState& run) const {
    const std::filesystem::path path = savePath(slotIndex);
    const std::filesystem::path backupPath = backupSavePath(slotIndex);
    const std::filesystem::path temporaryPath = temporarySavePath(slotIndex);

    std::filesystem::create_directories(path.parent_path());
    removeIfExists(temporaryPath);

    {
        std::ofstream file(temporaryPath, std::ios::trunc);
        if (!file.is_open()) {
            throwSaveIoError(temporaryPath, "failed to open temporary save file for writing");
        }

        file << RunStateSerializer::toJson(run).dump(4) << '\n';
        file.flush();
        if (!file.good()) {
            throwSaveIoError(temporaryPath, "failed to write temporary save file");
        }
    }

    if (std::filesystem::is_regular_file(path)) {
        try {
            (void)loadRunFromPath(path);
            copyFileReplacing(path, backupPath);
        } catch (const std::exception&) {
            // Keep the existing backup instead of replacing it with a broken primary save.
        }
    }

    std::error_code error;
    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        if (std::filesystem::exists(path)) {
            removeIfExists(path);
            error.clear();
            std::filesystem::rename(temporaryPath, path, error);
        }
    }

    if (error) {
        if (std::filesystem::is_regular_file(backupPath) && !std::filesystem::is_regular_file(path)) {
            copyFileReplacing(backupPath, path);
        }
        throwSaveIoError(path, "failed to replace run save atomically enough for this platform: " + error.message());
    }
}

RunState RunSaveSystem::loadRun(const std::size_t slotIndex) const {
    const RunSaveLoadResult result = tryLoadRun(slotIndex);
    if (!result.loaded) {
        throw std::runtime_error(result.errorMessage);
    }

    return result.run;
}

RunSaveLoadResult RunSaveSystem::tryLoadRun(const std::size_t slotIndex) const {
    const std::filesystem::path path = savePath(slotIndex);
    const std::filesystem::path backupPath = backupSavePath(slotIndex);

    RunSaveLoadResult result;

    if (std::filesystem::is_regular_file(path)) {
        try {
            result.run = loadRunFromPath(path);
            result.loaded = true;
            return result;
        } catch (const std::exception& error) {
            result.errorMessage = error.what();
        }
    } else {
        result.errorMessage = "No run save exists in slot " + std::to_string(slotIndex + 1);
    }

    if (std::filesystem::is_regular_file(backupPath)) {
        try {
            result.run = loadRunFromPath(backupPath);
            result.loaded = true;
            result.loadedFromBackup = true;
            return result;
        } catch (const std::exception& error) {
            if (!result.errorMessage.empty()) {
                result.errorMessage += "; backup also failed: ";
            }
            result.errorMessage += error.what();
        }
    }

    return result;
}


void RunSaveSystem::restoreBackupAsPrimary(const std::size_t slotIndex) const {
    const std::filesystem::path path = savePath(slotIndex);
    const std::filesystem::path backupPath = backupSavePath(slotIndex);
    const std::filesystem::path temporaryPath = temporarySavePath(slotIndex);

    if (!std::filesystem::is_regular_file(backupPath)) {
        throwSaveIoError(backupPath, "backup save does not exist");
    }

    std::filesystem::create_directories(path.parent_path());
    removeIfExists(temporaryPath);
    copyFileReplacing(backupPath, temporaryPath);

    if (std::filesystem::exists(path)) {
        removeIfExists(path);
    }

    std::error_code error;
    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        throwSaveIoError(path, "failed to restore backup save: " + error.message());
    }
}

void RunSaveSystem::deleteRun(const std::size_t slotIndex) const {
    removeIfExists(savePath(slotIndex));
    removeIfExists(backupSavePath(slotIndex));
    removeIfExists(temporarySavePath(slotIndex));
}

std::filesystem::path RunSaveSystem::slotDirectory(const std::size_t slotIndex) const {
    return savesRoot_ / ("slot_" + std::to_string(slotIndex + 1));
}
