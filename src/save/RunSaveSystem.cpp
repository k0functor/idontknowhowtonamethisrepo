#include "RunSaveSystem.hpp"

#include "data/JsonLoader.hpp"
#include "save/RunStateSerializer.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

RunSaveSystem::RunSaveSystem(std::filesystem::path savesRoot)
    : savesRoot_(std::move(savesRoot)) {}

void RunSaveSystem::setSavesRoot(std::filesystem::path savesRoot) {
    savesRoot_ = std::move(savesRoot);
}

bool RunSaveSystem::hasRunSave(const std::size_t slotIndex) const {
    return std::filesystem::is_regular_file(savePath(slotIndex));
}

std::filesystem::path RunSaveSystem::savePath(const std::size_t slotIndex) const {
    return slotDirectory(slotIndex) / "run_save.json";
}

void RunSaveSystem::saveRun(const std::size_t slotIndex, const RunState& run) const {
    const std::filesystem::path path = savePath(slotIndex);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open run save file '" + path.string() + "' for writing");
    }

    file << RunStateSerializer::toJson(run).dump(4) << '\n';
}

RunState RunSaveSystem::loadRun(const std::size_t slotIndex) const {
    const std::filesystem::path path = savePath(slotIndex);
    if (!hasRunSave(slotIndex)) {
        throw std::runtime_error("No run save exists in slot " + std::to_string(slotIndex + 1));
    }

    return RunStateSerializer::fromJson(JsonLoader::loadObjectFromFile(path), path);
}

void RunSaveSystem::deleteRun(const std::size_t slotIndex) const {
    const std::filesystem::path path = savePath(slotIndex);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
}

std::filesystem::path RunSaveSystem::slotDirectory(const std::size_t slotIndex) const {
    return savesRoot_ / ("slot_" + std::to_string(slotIndex + 1));
}
