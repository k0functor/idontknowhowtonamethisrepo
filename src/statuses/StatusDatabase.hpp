#pragma once

#include "statuses/StatusDefinition.hpp"
#include "statuses/StatusId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class StatusDatabase {
public:
    void clear();

    void loadFromFile(const std::filesystem::path& filePath);
    void loadFromDirectory(const std::filesystem::path& directoryPath);

    void add(StatusDefinition definition);

    bool contains(const StatusId& id) const;
    const StatusDefinition& get(const StatusId& id) const;

    std::vector<const StatusDefinition*> all() const;

    std::size_t size() const;

private:
    std::unordered_map<std::string, StatusDefinition> statuses_;
};
