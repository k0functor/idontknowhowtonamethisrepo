#pragma once

#include "drones/DroneDefinition.hpp"
#include "drones/DroneId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class DroneDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(DroneDefinition definition);

    bool contains(const DroneId& id) const;
    const DroneDefinition& get(const DroneId& id) const;

    std::vector<const DroneDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, DroneDefinition> drones_;
};
