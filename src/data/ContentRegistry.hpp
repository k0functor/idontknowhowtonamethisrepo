#pragma once

#include "active_items/ActiveItemDatabase.hpp"
#include "achievements/AchievementDatabase.hpp"
#include "actors/PlayerActorDatabase.hpp"
#include "challenges/ChallengeDatabase.hpp"
#include "archetypes/PlayableArchetypeDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "data/EnemyDatabase.hpp"
#include "data/EventDatabase.hpp"
#include "encounters/EncounterDatabase.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "drones/DroneDatabase.hpp"
#include "relics/RelicDatabase.hpp"
#include "rewards/RewardTuning.hpp"
#include "run/DifficultyDatabase.hpp"
#include "run/FloorDatabase.hpp"
#include "run/RunMapGenerationConfig.hpp"
#include "statuses/StatusDatabase.hpp"
#include "shop/ShopTuning.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

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

    const ActiveItemDatabase& activeItems() const;
    ActiveItemDatabase& activeItems();

    const DroneDatabase& drones() const;
    DroneDatabase& drones();

    const ChallengeDatabase& challenges() const;
    ChallengeDatabase& challenges();

    const AchievementDatabase& achievements() const;
    AchievementDatabase& achievements();

    const EventDatabase& events() const;
    EventDatabase& events();

    const EncounterDatabase& encounters() const;
    EncounterDatabase& encounters();
    const EncounterDatabase& encountersForFloor(const std::string& floorId) const;

    const RewardTuning& rewardTuning() const;
    RewardTuning& rewardTuning();

    const ShopTuning& shopTuning() const;
    ShopTuning& shopTuning();

    const FloorDatabase& floors() const;
    FloorDatabase& floors();

    const RunMapGenerationConfig& actOneMapGeneration() const;
    RunMapGenerationConfig& actOneMapGeneration();
    const RunMapGenerationConfig& mapGenerationForFloor(const std::string& floorId) const;

private:
    CardDatabase cards_;
    EnemyDatabase enemies_;
    StatusDatabase statuses_;
    RelicDatabase relics_;
    PlayableArchetypeDatabase archetypes_;
    PlayerActorDatabase actors_;
    DifficultyDatabase difficulties_;
    ConsumableDatabase consumables_;
    ActiveItemDatabase activeItems_;
    DroneDatabase drones_;
    ChallengeDatabase challenges_;
    AchievementDatabase achievements_;
    EventDatabase events_;
    EncounterDatabase encounters_;
    RewardTuning rewardTuning_;
    ShopTuning shopTuning_;
    FloorDatabase floors_;
    RunMapGenerationConfig actOneMapGeneration_;
    std::unordered_map<std::string, RunMapGenerationConfig> floorMapGeneration_;
    std::unordered_map<std::string, EncounterDatabase> floorEncounters_;
};
