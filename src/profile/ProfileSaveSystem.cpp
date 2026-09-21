#include "ProfileSaveSystem.hpp"

#include "data/JsonLoader.hpp"
#include "data/JsonReader.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {
constexpr int profileSaveVersion = 9;
constexpr int minimumSupportedProfileSaveVersion = 1;

void throwProfileIoError(const std::filesystem::path& path, const std::string& message) {
    throw std::runtime_error("Profile save IO error for '" + path.string() + "': " + message);
}

void removeIfExists(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::remove(path, error);
    if (error) {
        throwProfileIoError(path, error.message());
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
        throwProfileIoError(destination, "failed to copy backup from '" + source.string() + "': " + error.message());
    }
}

std::vector<std::string> normalizedStringList(std::vector<std::string> values) {
    values.erase(
        std::remove_if(values.begin(), values.end(), [](const std::string& value) {
            return value.empty();
        }),
        values.end()
    );
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

std::vector<ProfileProgressEntry> normalizedProgressLog(std::vector<ProfileProgressEntry> entries) {
    entries.erase(
        std::remove_if(entries.begin(), entries.end(), [](const ProfileProgressEntry& entry) {
            return entry.type.empty() || entry.contentId.empty();
        }),
        entries.end()
    );

    std::vector<ProfileProgressEntry> uniqueEntries;
    uniqueEntries.reserve(entries.size());
    for (const ProfileProgressEntry& entry : entries) {
        const auto existing = std::find_if(
            uniqueEntries.begin(),
            uniqueEntries.end(),
            [&entry](const ProfileProgressEntry& known) {
                return known.type == entry.type && known.contentId == entry.contentId;
            }
        );
        if (existing == uniqueEntries.end()) {
            uniqueEntries.push_back(entry);
        }
    }

    constexpr std::size_t maxProgressLogEntries = 160u;
    if (uniqueEntries.size() > maxProgressLogEntries) {
        uniqueEntries.erase(
            uniqueEntries.begin(),
            uniqueEntries.begin() + static_cast<std::ptrdiff_t>(uniqueEntries.size() - maxProgressLogEntries)
        );
    }
    return uniqueEntries;
}

Json countListToJson(const std::vector<ProfileCountEntry>& counts) {
    Json array = Json::array();
    for (const ProfileCountEntry& entry : counts) {
        if (entry.contentId.empty() || entry.count <= 0) {
            continue;
        }
        Json item = Json::object();
        item["content_id"] = entry.contentId;
        item["count"] = entry.count;
        array.push_back(std::move(item));
    }
    return array;
}

std::vector<ProfileCountEntry> countListFromJson(const Json& json, const std::filesystem::path& path, const std::string& fieldName) {
    if (!json.is_array()) {
        throw std::runtime_error(path.string() + ": " + fieldName + " must be an array");
    }

    std::vector<ProfileCountEntry> counts;
    counts.reserve(json.size());
    for (std::size_t index = 0; index < json.size(); ++index) {
        const Json& item = json.at(index);
        if (!item.is_object()) {
            throw std::runtime_error(path.string() + ": " + fieldName + "[" + std::to_string(index) + "] must be an object");
        }
        const auto idIt = item.find("content_id");
        const auto countIt = item.find("count");
        if (idIt == item.end() || !idIt->is_string()) {
            throw std::runtime_error(path.string() + ": " + fieldName + "[" + std::to_string(index) + "].content_id must be a string");
        }
        if (countIt == item.end() || !countIt->is_number_integer()) {
            throw std::runtime_error(path.string() + ": " + fieldName + "[" + std::to_string(index) + "].count must be an integer");
        }
        const int count = countIt->get<int>();
        if (count <= 0) {
            throw std::runtime_error(path.string() + ": " + fieldName + "[" + std::to_string(index) + "].count must be positive");
        }
        counts.push_back(ProfileCountEntry{idIt->get<std::string>(), count});
    }

    return counts;
}


Json runStatisticsToJson(const ProfileRunStatistics& stats) {
    Json json = Json::object();
    json["combats_won"] = stats.combatsWon;
    json["combats_lost"] = stats.combatsLost;
    json["enemies_killed"] = stats.enemiesKilled;
    json["elites_killed"] = stats.elitesKilled;
    json["bosses_killed"] = stats.bossesKilled;
    json["events_completed"] = stats.eventsCompleted;
    json["shops_visited"] = stats.shopsVisited;
    json["chests_opened"] = stats.chestsOpened;
    json["rests_used"] = stats.restsUsed;
    json["damage_taken"] = stats.damageTaken;
    json["damage_dealt"] = stats.damageDealt;
    json["damage_blocked"] = stats.damageBlocked;
    json["block_gained"] = stats.blockGained;
    json["combat_turns"] = stats.combatTurns;
    json["cards_played_in_combat"] = stats.cardsPlayedInCombat;
    json["energy_spent_on_cards"] = stats.energySpentOnCards;
    json["maximum_single_hit"] = stats.maximumSingleHit;
    json["longest_combat_turns"] = stats.longestCombatTurns;
    json["most_cards_played_in_combat"] = stats.mostCardsPlayedInCombat;
    json["consumables_used"] = stats.consumablesUsed;
    json["gold_gained"] = stats.goldGained;
    json["gold_spent"] = stats.goldSpent;
    json["cards_added"] = stats.cardsAdded;
    json["cards_removed"] = stats.cardsRemoved;
    json["cards_upgraded"] = stats.cardsUpgraded;
    json["cards_skipped"] = stats.cardsSkipped;
    json["rewards_skipped"] = stats.rewardsSkipped;
    json["relics_gained"] = stats.relicsGained;
    json["consumables_gained"] = stats.consumablesGained;
    json["nodes_completed"] = stats.nodesCompleted;
    return json;
}

ProfileRunStatistics runStatisticsFromJson(const Json& json, const std::filesystem::path& path) {
    JsonReader reader(json, path);
    ProfileRunStatistics stats;
    stats.combatsWon = reader.optionalInt("combats_won", 0);
    stats.combatsLost = reader.optionalInt("combats_lost", 0);
    stats.enemiesKilled = reader.optionalInt("enemies_killed", 0);
    stats.elitesKilled = reader.optionalInt("elites_killed", 0);
    stats.bossesKilled = reader.optionalInt("bosses_killed", 0);
    stats.eventsCompleted = reader.optionalInt("events_completed", 0);
    stats.shopsVisited = reader.optionalInt("shops_visited", 0);
    stats.chestsOpened = reader.optionalInt("chests_opened", 0);
    stats.restsUsed = reader.optionalInt("rests_used", 0);
    stats.damageTaken = reader.optionalInt("damage_taken", 0);
    stats.damageDealt = reader.optionalInt("damage_dealt", 0);
    stats.damageBlocked = reader.optionalInt("damage_blocked", 0);
    stats.blockGained = reader.optionalInt("block_gained", 0);
    stats.combatTurns = reader.optionalInt("combat_turns", 0);
    stats.cardsPlayedInCombat = reader.optionalInt("cards_played_in_combat", 0);
    stats.energySpentOnCards = reader.optionalInt("energy_spent_on_cards", 0);
    stats.maximumSingleHit = reader.optionalInt("maximum_single_hit", 0);
    stats.longestCombatTurns = reader.optionalInt("longest_combat_turns", 0);
    stats.mostCardsPlayedInCombat = reader.optionalInt("most_cards_played_in_combat", 0);
    stats.consumablesUsed = reader.optionalInt("consumables_used", 0);
    stats.goldGained = reader.optionalInt("gold_gained", 0);
    stats.goldSpent = reader.optionalInt("gold_spent", 0);
    stats.cardsAdded = reader.optionalInt("cards_added", 0);
    stats.cardsRemoved = reader.optionalInt("cards_removed", 0);
    stats.cardsUpgraded = reader.optionalInt("cards_upgraded", 0);
    stats.cardsSkipped = reader.optionalInt("cards_skipped", 0);
    stats.rewardsSkipped = reader.optionalInt("rewards_skipped", 0);
    stats.relicsGained = reader.optionalInt("relics_gained", 0);
    stats.consumablesGained = reader.optionalInt("consumables_gained", 0);
    stats.nodesCompleted = reader.optionalInt("nodes_completed", 0);

    if (stats.combatsWon < 0 || stats.combatsLost < 0 || stats.enemiesKilled < 0 ||
        stats.elitesKilled < 0 || stats.bossesKilled < 0 || stats.eventsCompleted < 0 ||
        stats.shopsVisited < 0 || stats.chestsOpened < 0 || stats.restsUsed < 0 ||
        stats.damageTaken < 0 || stats.damageDealt < 0 || stats.damageBlocked < 0 ||
        stats.blockGained < 0 || stats.combatTurns < 0 || stats.cardsPlayedInCombat < 0 ||
        stats.energySpentOnCards < 0 || stats.maximumSingleHit < 0 ||
        stats.longestCombatTurns < 0 || stats.mostCardsPlayedInCombat < 0 ||
        stats.consumablesUsed < 0 || stats.goldGained < 0 ||
        stats.goldSpent < 0 || stats.cardsAdded < 0 || stats.cardsRemoved < 0 ||
        stats.cardsUpgraded < 0 || stats.cardsSkipped < 0 || stats.rewardsSkipped < 0 ||
        stats.relicsGained < 0 || stats.consumablesGained < 0 || stats.nodesCompleted < 0) {
        throw std::runtime_error(path.string() + ": lifetime_run_stats values must be non-negative");
    }

    return stats;
}

Json progressLogToJson(const std::vector<ProfileProgressEntry>& progressLog) {
    Json array = Json::array();
    for (const ProfileProgressEntry& entry : normalizedProgressLog(progressLog)) {
        Json item = Json::object();
        item["type"] = entry.type;
        item["content_id"] = entry.contentId;
        array.push_back(std::move(item));
    }
    return array;
}

std::vector<ProfileProgressEntry> progressLogFromJson(const Json& json, const std::filesystem::path& path) {
    if (!json.is_array()) {
        throw std::runtime_error(path.string() + ": progress_log must be an array");
    }

    std::vector<ProfileProgressEntry> entries;
    entries.reserve(json.size());
    for (std::size_t index = 0; index < json.size(); ++index) {
        const Json& item = json.at(index);
        if (!item.is_object()) {
            throw std::runtime_error(path.string() + ": progress_log[" + std::to_string(index) + "] must be an object");
        }
        const auto typeIt = item.find("type");
        const auto contentIt = item.find("content_id");
        if (typeIt == item.end() || !typeIt->is_string()) {
            throw std::runtime_error(path.string() + ": progress_log[" + std::to_string(index) + "].type must be a string");
        }
        if (contentIt == item.end() || !contentIt->is_string()) {
            throw std::runtime_error(path.string() + ": progress_log[" + std::to_string(index) + "].content_id must be a string");
        }
        entries.push_back(ProfileProgressEntry{typeIt->get<std::string>(), contentIt->get<std::string>()});
    }
    return normalizedProgressLog(std::move(entries));
}

Json profileToJson(const ProfileData& profile) {
    Json json = Json::object();
    json["version"] = profileSaveVersion;
    json["slot_index"] = profile.slotIndex;
    json["is_empty"] = profile.isEmpty;
    json["victories"] = profile.victories;
    json["defeats"] = profile.defeats;
    json["unlocked_cards"] = normalizedStringList(profile.unlockedCardIds);
    json["unlocked_relics"] = normalizedStringList(profile.unlockedRelicIds);
    json["unlocked_archetypes"] = normalizedStringList(profile.unlockedArchetypeIds);
    json["discovered_enemies"] = normalizedStringList(profile.discoveredEnemyIds);
    json["discovered_statuses"] = normalizedStringList(profile.discoveredStatusIds);
    json["discovered_consumables"] = normalizedStringList(profile.discoveredConsumableIds);
    json["completed_challenges"] = normalizedStringList(profile.completedChallengeIds);
    json["completed_achievements"] = normalizedStringList(profile.completedAchievementIds);
    json["onboarding_hints_seen"] = normalizedStringList(profile.seenOnboardingHintIds);
    json["progress_log"] = progressLogToJson(profile.progressLog);
    json["card_play_counts"] = countListToJson(profile.cardPlayCounts);
    json["relic_pick_counts"] = countListToJson(profile.relicPickCounts);
    json["lifetime_run_stats"] = runStatisticsToJson(profile.lifetimeRunStats);
    return json;
}

ProfileData profileFromJson(const Json& json, const std::filesystem::path& path) {
    JsonReader reader(json, path);

    const int version = reader.requiredInt("version");
    if (version < minimumSupportedProfileSaveVersion || version > profileSaveVersion) {
        throw std::runtime_error(path.string() + ": unsupported profile save version " + std::to_string(version));
    }

    ProfileData profile;
    profile.slotIndex = reader.requiredInt("slot_index");
    profile.isEmpty = reader.optionalBool("is_empty", false);
    profile.victories = reader.optionalInt("victories", 0);
    profile.defeats = reader.optionalInt("defeats", 0);
    profile.unlockedCardIds = normalizedStringList(reader.optionalStringArray("unlocked_cards"));
    profile.unlockedRelicIds = normalizedStringList(reader.optionalStringArray("unlocked_relics"));
    profile.unlockedArchetypeIds = normalizedStringList(reader.optionalStringArray("unlocked_archetypes"));
    profile.discoveredEnemyIds = normalizedStringList(reader.optionalStringArray("discovered_enemies"));
    profile.discoveredStatusIds = normalizedStringList(reader.optionalStringArray("discovered_statuses"));
    profile.discoveredConsumableIds = normalizedStringList(reader.optionalStringArray("discovered_consumables"));
    profile.completedChallengeIds = normalizedStringList(reader.optionalStringArray("completed_challenges"));
    profile.completedAchievementIds = normalizedStringList(reader.optionalStringArray("completed_achievements"));
    profile.seenOnboardingHintIds = normalizedStringList(reader.optionalStringArray("onboarding_hints_seen"));
    profile.progressLog = reader.has("progress_log")
        ? progressLogFromJson(reader.requiredArray("progress_log"), path)
        : std::vector<ProfileProgressEntry>{};
    profile.cardPlayCounts = reader.has("card_play_counts")
        ? countListFromJson(reader.requiredArray("card_play_counts"), path, "card_play_counts")
        : std::vector<ProfileCountEntry>{};
    profile.relicPickCounts = reader.has("relic_pick_counts")
        ? countListFromJson(reader.requiredArray("relic_pick_counts"), path, "relic_pick_counts")
        : std::vector<ProfileCountEntry>{};
    profile.lifetimeRunStats = reader.has("lifetime_run_stats")
        ? runStatisticsFromJson(reader.requiredObject("lifetime_run_stats"), path)
        : ProfileRunStatistics{};

    if (version < 8) {
        const bool clearedThirdFloor = profile.lifetimeRunStats.bossesKilled >= 3;
        const bool completedThirdFloorAchievement = std::find(
            profile.completedAchievementIds.begin(),
            profile.completedAchievementIds.end(),
            "ashen_conservatory_survivor"
        ) != profile.completedAchievementIds.end();
        const bool explicitlyUnlocked = std::any_of(
            profile.progressLog.begin(),
            profile.progressLog.end(),
            [](const ProfileProgressEntry& entry) {
                return entry.type == "archetype_unlocked" && entry.contentId == "sadist_masochist";
            }
        );

        if (!clearedThirdFloor && !completedThirdFloorAchievement && !explicitlyUnlocked) {
            profile.unlockedArchetypeIds.erase(
                std::remove(
                    profile.unlockedArchetypeIds.begin(),
                    profile.unlockedArchetypeIds.end(),
                    "sadist_masochist"
                ),
                profile.unlockedArchetypeIds.end()
            );
        }
    }

    if (profile.slotIndex < 0) {
        throw std::runtime_error(path.string() + ": slot_index must be non-negative");
    }
    if (profile.victories < 0) {
        throw std::runtime_error(path.string() + ": victories must be non-negative");
    }
    if (profile.defeats < 0) {
        throw std::runtime_error(path.string() + ": defeats must be non-negative");
    }

    return profile;
}

ProfileData loadProfileFromPath(const std::filesystem::path& path) {
    return profileFromJson(JsonLoader::loadObjectFromFile(path), path);
}
}

ProfileSaveSystem::ProfileSaveSystem(std::filesystem::path savesRoot)
    : savesRoot_(std::move(savesRoot)) {}

void ProfileSaveSystem::setSavesRoot(std::filesystem::path savesRoot) {
    savesRoot_ = std::move(savesRoot);
}

bool ProfileSaveSystem::hasProfileSave(const std::size_t slotIndex) const {
    return std::filesystem::is_regular_file(savePath(slotIndex)) ||
           std::filesystem::is_regular_file(backupSavePath(slotIndex));
}

std::filesystem::path ProfileSaveSystem::savePath(const std::size_t slotIndex) const {
    return slotDirectory(slotIndex) / "profile.json";
}

std::filesystem::path ProfileSaveSystem::backupSavePath(const std::size_t slotIndex) const {
    return slotDirectory(slotIndex) / "profile.backup.json";
}

std::filesystem::path ProfileSaveSystem::temporarySavePath(const std::size_t slotIndex) const {
    return slotDirectory(slotIndex) / "profile.tmp.json";
}

void ProfileSaveSystem::saveProfile(const std::size_t slotIndex, const ProfileData& profile) const {
    const std::filesystem::path path = savePath(slotIndex);
    const std::filesystem::path backupPath = backupSavePath(slotIndex);
    const std::filesystem::path temporaryPath = temporarySavePath(slotIndex);

    ProfileData savedProfile = profile;
    savedProfile.slotIndex = static_cast<int>(slotIndex);

    std::filesystem::create_directories(path.parent_path());
    removeIfExists(temporaryPath);

    {
        std::ofstream file(temporaryPath, std::ios::trunc);
        if (!file.is_open()) {
            throwProfileIoError(temporaryPath, "failed to open temporary profile file for writing");
        }

        file << profileToJson(savedProfile).dump(4) << '\n';
        file.flush();
        if (!file.good()) {
            throwProfileIoError(temporaryPath, "failed to write temporary profile file");
        }
    }

    if (std::filesystem::is_regular_file(path)) {
        try {
            (void)loadProfileFromPath(path);
            copyFileReplacing(path, backupPath);
        } catch (const std::exception&) {
            // Keep the last valid backup instead of replacing it with a broken primary profile.
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
        throwProfileIoError(path, "failed to replace profile save atomically enough for this platform: " + error.message());
    }
}

ProfileData ProfileSaveSystem::loadProfile(const std::size_t slotIndex) const {
    const ProfileSaveLoadResult result = tryLoadProfile(slotIndex);
    if (!result.loaded) {
        throw std::runtime_error(result.errorMessage);
    }

    return result.profile;
}

ProfileSaveLoadResult ProfileSaveSystem::tryLoadProfile(const std::size_t slotIndex) const {
    const std::filesystem::path path = savePath(slotIndex);
    const std::filesystem::path backupPath = backupSavePath(slotIndex);

    ProfileSaveLoadResult result;

    if (std::filesystem::is_regular_file(path)) {
        try {
            result.profile = loadProfileFromPath(path);
            result.loaded = true;
            return result;
        } catch (const std::exception& error) {
            result.errorMessage = error.what();
        }
    } else {
        result.errorMessage = "No profile save exists in slot " + std::to_string(slotIndex + 1);
    }

    if (std::filesystem::is_regular_file(backupPath)) {
        try {
            result.profile = loadProfileFromPath(backupPath);
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

void ProfileSaveSystem::restoreBackupAsPrimary(const std::size_t slotIndex) const {
    const std::filesystem::path path = savePath(slotIndex);
    const std::filesystem::path backupPath = backupSavePath(slotIndex);
    const std::filesystem::path temporaryPath = temporarySavePath(slotIndex);

    if (!std::filesystem::is_regular_file(backupPath)) {
        throwProfileIoError(backupPath, "backup profile does not exist");
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
        throwProfileIoError(path, "failed to restore backup profile: " + error.message());
    }
}

void ProfileSaveSystem::deleteProfile(const std::size_t slotIndex) const {
    removeIfExists(savePath(slotIndex));
    removeIfExists(backupSavePath(slotIndex));
    removeIfExists(temporarySavePath(slotIndex));

    const std::filesystem::path directory = slotDirectory(slotIndex);
    std::error_code error;
    if (std::filesystem::is_directory(directory) && std::filesystem::is_empty(directory, error)) {
        if (!error) {
            std::filesystem::remove(directory, error);
        }
        if (error) {
            throwProfileIoError(directory, error.message());
        }
    }
}

std::filesystem::path ProfileSaveSystem::slotDirectory(const std::size_t slotIndex) const {
    return savesRoot_ / ("slot_" + std::to_string(slotIndex + 1));
}
