#pragma once

#include "events/RunEventDefinition.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class EventDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(RunEventDefinition definition);

    bool contains(const std::string& id) const;
    const RunEventDefinition& get(const std::string& id) const;

    std::vector<const RunEventDefinition*> all() const;
    std::size_t size() const;

private:
    std::unordered_map<std::string, RunEventDefinition> events_;
};
