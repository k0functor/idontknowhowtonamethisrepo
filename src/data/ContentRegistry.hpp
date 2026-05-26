#pragma once

#include "actors/PlayerActorDatabase.hpp"
#include "archetypes/PlayableArchetypeDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "data/EnemyDatabase.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "drones/DroneDatabase.hpp"
#include "relics/RelicDatabase.hpp"
#include "run/DifficultyDatabase.hpp"
#include "statuses/StatusDatabase.hpp"

#include <filesystem>

class ContentRegistry {
public:
    void clear();

    void loadFromDataDirectory(const std::filesystem::path& dataDirectory);

    const CardDatabase& cards() const;
    CardDatabase& cards();

    const EnemyDatabase& enemies() const;
    EnemyDatabase& enemies();

    const StatusDatabase& statuses() const;
    StatusDatabase& statuses();

    const RelicDatabase& relics() const;
    RelicDatabase& relics();

    const PlayableArchetypeDatabase& archetypes() const;
    PlayableArchetypeDatabase& archetypes();

    const PlayerActorDatabase& actors() const;
    PlayerActorDatabase& actors();

    const DifficultyDatabase& difficulties() const;
    DifficultyDatabase& difficulties();

    const ConsumableDatabase& consumables() const;
    ConsumableDatabase& consumables();

    const DroneDatabase& drones() const;
    DroneDatabase& drones();

private:
    CardDatabase cards_;
    EnemyDatabase enemies_;
    StatusDatabase statuses_;
    RelicDatabase relics_;
    PlayableArchetypeDatabase archetypes_;
    PlayerActorDatabase actors_;
    DifficultyDatabase difficulties_;
    ConsumableDatabase consumables_;
    DroneDatabase drones_;
};
