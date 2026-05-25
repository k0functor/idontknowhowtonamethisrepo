#pragma once

#include "data/CardDatabase.hpp"
#include "data/EnemyDatabase.hpp"

#include <filesystem>

class ContentRegistry {
public:
    void clear();

    void loadFromDataDirectory(const std::filesystem::path& dataDirectory);

    const CardDatabase& cards() const;
    CardDatabase& cards();

    const EnemyDatabase& enemies() const;
    EnemyDatabase& enemies();

private:
    CardDatabase cards_;
    EnemyDatabase enemies_;
};
