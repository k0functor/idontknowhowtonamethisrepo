#pragma once

#include "run/RunState.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

class RunSaveSystem {
public:
    explicit RunSaveSystem(std::filesystem::path savesRoot = "saves");

    void setSavesRoot(std::filesystem::path savesRoot);

    bool hasRunSave(std::size_t slotIndex) const;
    std::filesystem::path savePath(std::size_t slotIndex) const;

    void saveRun(std::size_t slotIndex, const RunState& run) const;
    RunState loadRun(std::size_t slotIndex) const;
    void deleteRun(std::size_t slotIndex) const;

private:
    std::filesystem::path slotDirectory(std::size_t slotIndex) const;

private:
    std::filesystem::path savesRoot_;
};
