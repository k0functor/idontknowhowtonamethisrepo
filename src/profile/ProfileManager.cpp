#include "ProfileManager.hpp"

#include "run/RunState.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
constexpr std::array<const char*, 2> starterArchetypeIds{
    "rusted_knight",
    "herbalist"
};

void pushUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) {
        return;
    }

    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

void normalizeStringList(std::vector<std::string>& values) {
    values.erase(
        std::remove_if(values.begin(), values.end(), [](const std::string& value) {
            return value.empty();
        }),
        values.end()
    );
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

void normalizeCountList(std::vector<ProfileCountEntry>& values) {
    values.erase(
        std::remove_if(values.begin(), values.end(), [](const ProfileCountEntry& entry) {
            return entry.contentId.empty() || entry.count <= 0;
        }),
        values.end()
    );
    std::sort(
        values.begin(),
        values.end(),
        [](const ProfileCountEntry& left, const ProfileCountEntry& right) {
            return left.contentId < right.contentId;
        }
    );

    std::vector<ProfileCountEntry> merged;
    merged.reserve(values.size());
    for (const ProfileCountEntry& entry : values) {
        if (!merged.empty() && merged.back().contentId == entry.contentId) {
            merged.back().count += entry.count;
        } else {
            merged.push_back(entry);
        }
    }

    values = std::move(merged);
}

bool incrementCount(std::vector<ProfileCountEntry>& values, const std::string& contentId, const int amount) {
    if (contentId.empty() || amount <= 0) {
        return false;
    }

    for (ProfileCountEntry& entry : values) {
        if (entry.contentId == contentId) {
            entry.count = std::max(0, entry.count) + amount;
            return true;
        }
    }

    values.push_back(ProfileCountEntry{contentId, amount});
    return true;
}


void addRunStats(ProfileRunStatistics& totals, const RunStats& run) {
    totals.combatsWon += std::max(0, run.combatsWon);
    totals.combatsLost += std::max(0, run.combatsLost);
    totals.enemiesKilled += std::max(0, run.enemiesKilled);
    totals.elitesKilled += std::max(0, run.elitesKilled);
    totals.bossesKilled += std::max(0, run.bossesKilled);
    totals.eventsCompleted += std::max(0, run.eventsCompleted);
    totals.shopsVisited += std::max(0, run.shopsVisited);
    totals.chestsOpened += std::max(0, run.chestsOpened);
    totals.restsUsed += std::max(0, run.restsUsed);
    totals.damageTaken += std::max(0, run.damageTaken);
    totals.damageDealt += std::max(0, run.damageDealt);
    totals.damageBlocked += std::max(0, run.damageBlocked);
    totals.blockGained += std::max(0, run.blockGained);
    totals.combatTurns += std::max(0, run.combatTurns);
    totals.cardsPlayedInCombat += std::max(0, run.cardsPlayedInCombat);
    totals.energySpentOnCards += std::max(0, run.energySpentOnCards);
    totals.maximumSingleHit = std::max(totals.maximumSingleHit, std::max(0, run.maximumSingleHit));
    totals.longestCombatTurns = std::max(totals.longestCombatTurns, std::max(0, run.longestCombatTurns));
    totals.mostCardsPlayedInCombat = std::max(
        totals.mostCardsPlayedInCombat,
        std::max(0, run.mostCardsPlayedInCombat)
    );
    totals.consumablesUsed += std::max(0, run.consumablesUsed);
    totals.goldGained += std::max(0, run.goldGained);
    totals.goldSpent += std::max(0, run.goldSpent);
    totals.cardsAdded += std::max(0, run.cardsAdded);
    totals.cardsRemoved += std::max(0, run.cardsRemoved);
    totals.cardsUpgraded += std::max(0, run.cardsUpgraded);
    totals.cardsSkipped += std::max(0, run.cardsSkipped);
    totals.rewardsSkipped += std::max(0, run.rewardsSkipped);
    totals.relicsGained += std::max(0, run.relicsGained);
    totals.consumablesGained += std::max(0, run.consumablesGained);
    totals.nodesCompleted += std::max(0, run.nodesCompleted);
}

void normalizeRunStats(ProfileRunStatistics& stats) {
    stats.combatsWon = std::max(stats.combatsWon, 0);
    stats.combatsLost = std::max(stats.combatsLost, 0);
    stats.enemiesKilled = std::max(stats.enemiesKilled, 0);
    stats.elitesKilled = std::max(stats.elitesKilled, 0);
    stats.bossesKilled = std::max(stats.bossesKilled, 0);
    stats.eventsCompleted = std::max(stats.eventsCompleted, 0);
    stats.shopsVisited = std::max(stats.shopsVisited, 0);
    stats.chestsOpened = std::max(stats.chestsOpened, 0);
    stats.restsUsed = std::max(stats.restsUsed, 0);
    stats.damageTaken = std::max(stats.damageTaken, 0);
    stats.damageDealt = std::max(stats.damageDealt, 0);
    stats.damageBlocked = std::max(stats.damageBlocked, 0);
    stats.blockGained = std::max(stats.blockGained, 0);
    stats.combatTurns = std::max(stats.combatTurns, 0);
    stats.cardsPlayedInCombat = std::max(stats.cardsPlayedInCombat, 0);
    stats.energySpentOnCards = std::max(stats.energySpentOnCards, 0);
    stats.maximumSingleHit = std::max(stats.maximumSingleHit, 0);
    stats.longestCombatTurns = std::max(stats.longestCombatTurns, 0);
    stats.mostCardsPlayedInCombat = std::max(stats.mostCardsPlayedInCombat, 0);
    stats.consumablesUsed = std::max(stats.consumablesUsed, 0);
    stats.goldGained = std::max(stats.goldGained, 0);
    stats.goldSpent = std::max(stats.goldSpent, 0);
    stats.cardsAdded = std::max(stats.cardsAdded, 0);
    stats.cardsRemoved = std::max(stats.cardsRemoved, 0);
    stats.cardsUpgraded = std::max(stats.cardsUpgraded, 0);
    stats.cardsSkipped = std::max(stats.cardsSkipped, 0);
    stats.rewardsSkipped = std::max(stats.rewardsSkipped, 0);
    stats.relicsGained = std::max(stats.relicsGained, 0);
    stats.consumablesGained = std::max(stats.consumablesGained, 0);
    stats.nodesCompleted = std::max(stats.nodesCompleted, 0);
}

void normalizeProgressLog(std::vector<ProfileProgressEntry>& entries) {
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

    entries = std::move(uniqueEntries);
}
}

ProfileManager::ProfileManager() {
    resetSlotsToEmpty();
    loadAllProfiles();
}

ProfileManager::ProfileManager(std::filesystem::path savesRoot)
    : saveSystem_(std::move(savesRoot)) {
    resetSlotsToEmpty();
    loadAllProfiles();
}

void ProfileManager::setSavesRoot(std::filesystem::path savesRoot) {
    saveSystem_.setSavesRoot(std::move(savesRoot));
    loadAllProfiles();
}

void ProfileManager::loadAllProfiles() {
    resetSlotsToEmpty();

    for (std::size_t i = 0; i < slots_.size(); ++i) {
        ProfileSaveLoadResult result = saveSystem_.tryLoadProfile(i);
        if (!result.loaded) {
            continue;
        }

        normalizeProfile(result.profile, i);
        slots_[i].data = std::move(result.profile);

        if (result.loadedFromBackup) {
            try {
                saveSystem_.restoreBackupAsPrimary(i);
                std::cout << "Restored profile save for slot " << (i + 1) // NOL10N: developer diagnostic
                          << " from backup.\n"; // NOL10N: developer diagnostic
            } catch (const std::exception& error) {
                std::cout << "Loaded profile save for slot " << (i + 1) // NOL10N: developer diagnostic
                          << " from backup, but failed to restore the primary profile: " // NOL10N: developer diagnostic
                          << error.what() << '\n';
            }
        }
    }

    if (hasSelectedSlot_ && selectedSlotIndex_ >= slots_.size()) {
        hasSelectedSlot_ = false;
        selectedSlotIndex_ = 0;
    }
}

const std::array<ProfileSlot, 3>& ProfileManager::slots() const {
    return slots_;
}

ProfileData& ProfileManager::selectSlot(const std::size_t index) {
    if (index >= slots_.size()) {
        throw std::runtime_error("Profile slot index is out of range");
    }

    selectedSlotIndex_ = index;
    hasSelectedSlot_ = true;

    ProfileData& profile = slots_[index].data;
    profile.isEmpty = false;
    normalizeProfile(profile, index);
    saveProfile(index);

    return profile;
}

const ProfileData* ProfileManager::selectedProfile() const {
    if (!hasSelectedSlot_) {
        return nullptr;
    }

    return &slots_[selectedSlotIndex_].data;
}

ProfileData* ProfileManager::selectedProfile() {
    if (!hasSelectedSlot_) {
        return nullptr;
    }

    return &slots_[selectedSlotIndex_].data;
}

std::size_t ProfileManager::selectedSlotIndex() const {
    return selectedSlotIndex_;
}

void ProfileManager::saveProfile(const std::size_t index) const {
    if (index >= slots_.size()) {
        throw std::runtime_error("Profile slot index is out of range");
    }

    saveSystem_.saveProfile(index, slots_[index].data);
}

void ProfileManager::saveSelectedProfile() const {
    if (!hasSelectedSlot_) {
        return;
    }

    saveProfile(selectedSlotIndex_);
}

void ProfileManager::deleteProfile(const std::size_t index) {
    if (index >= slots_.size()) {
        throw std::runtime_error("Profile slot index is out of range");
    }

    saveSystem_.deleteProfile(index);
    slots_[index].index = static_cast<int>(index);
    slots_[index].data = ProfileData{};
    slots_[index].data.slotIndex = static_cast<int>(index);
    slots_[index].data.isEmpty = true;

    if (hasSelectedSlot_ && selectedSlotIndex_ == index) {
        hasSelectedSlot_ = false;
        selectedSlotIndex_ = 0;
    }
}

bool ProfileManager::isArchetypeUnlocked(const ProfileData& profile, const std::string& archetypeId) const {
    if (archetypeId.empty()) {
        return false;
    }

    return std::find(
        profile.unlockedArchetypeIds.begin(),
        profile.unlockedArchetypeIds.end(),
        archetypeId
    ) != profile.unlockedArchetypeIds.end();
}

bool ProfileManager::isSelectedArchetypeUnlocked(const std::string& archetypeId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isArchetypeUnlocked(*profile, archetypeId);
}


bool ProfileManager::isCardUnlocked(const ProfileData& profile, const std::string& cardId) const {
    if (cardId.empty()) {
        return false;
    }

    return std::find(
        profile.unlockedCardIds.begin(),
        profile.unlockedCardIds.end(),
        cardId
    ) != profile.unlockedCardIds.end();
}

bool ProfileManager::isSelectedCardUnlocked(const std::string& cardId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isCardUnlocked(*profile, cardId);
}

bool ProfileManager::isRelicUnlocked(const ProfileData& profile, const std::string& relicId) const {
    if (relicId.empty()) {
        return false;
    }

    return std::find(
        profile.unlockedRelicIds.begin(),
        profile.unlockedRelicIds.end(),
        relicId
    ) != profile.unlockedRelicIds.end();
}

bool ProfileManager::isSelectedRelicUnlocked(const std::string& relicId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isRelicUnlocked(*profile, relicId);
}

bool ProfileManager::isEnemyDiscovered(const ProfileData& profile, const std::string& enemyId) const {
    if (enemyId.empty()) {
        return false;
    }

    return std::find(
        profile.discoveredEnemyIds.begin(),
        profile.discoveredEnemyIds.end(),
        enemyId
    ) != profile.discoveredEnemyIds.end();
}

bool ProfileManager::isSelectedEnemyDiscovered(const std::string& enemyId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isEnemyDiscovered(*profile, enemyId);
}

bool ProfileManager::isStatusDiscovered(const ProfileData& profile, const std::string& statusId) const {
    if (statusId.empty()) {
        return false;
    }

    return std::find(
        profile.discoveredStatusIds.begin(),
        profile.discoveredStatusIds.end(),
        statusId
    ) != profile.discoveredStatusIds.end();
}

bool ProfileManager::isSelectedStatusDiscovered(const std::string& statusId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isStatusDiscovered(*profile, statusId);
}

bool ProfileManager::isConsumableDiscovered(const ProfileData& profile, const std::string& consumableId) const {
    if (consumableId.empty()) {
        return false;
    }

    return std::find(
        profile.discoveredConsumableIds.begin(),
        profile.discoveredConsumableIds.end(),
        consumableId
    ) != profile.discoveredConsumableIds.end();
}

bool ProfileManager::isSelectedConsumableDiscovered(const std::string& consumableId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isConsumableDiscovered(*profile, consumableId);
}

bool ProfileManager::isChallengeCompleted(const ProfileData& profile, const std::string& challengeId) const {
    if (challengeId.empty()) {
        return false;
    }

    return std::find(
        profile.completedChallengeIds.begin(),
        profile.completedChallengeIds.end(),
        challengeId
    ) != profile.completedChallengeIds.end();
}

bool ProfileManager::isSelectedChallengeCompleted(const std::string& challengeId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isChallengeCompleted(*profile, challengeId);
}


bool ProfileManager::isAchievementCompleted(const ProfileData& profile, const std::string& achievementId) const {
    if (achievementId.empty()) {
        return false;
    }

    return std::find(
        profile.completedAchievementIds.begin(),
        profile.completedAchievementIds.end(),
        achievementId
    ) != profile.completedAchievementIds.end();
}

bool ProfileManager::isSelectedAchievementCompleted(const std::string& achievementId) const {
    const ProfileData* profile = selectedProfile();
    return profile != nullptr && isAchievementCompleted(*profile, achievementId);
}

bool ProfileManager::unlockSelectedArchetype(const std::string& archetypeId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || archetypeId.empty()) {
        return false;
    }

    const bool wasLocked = !isArchetypeUnlocked(*profile, archetypeId);
    if (wasLocked) {
        profile->unlockedArchetypeIds.push_back(archetypeId);
        appendProgressEntry(*profile, "archetype_unlocked", archetypeId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasLocked;
}


bool ProfileManager::unlockSelectedArchetypes(const std::vector<std::string>& archetypeIds) {
    bool changed = false;
    for (const std::string& archetypeId : archetypeIds) {
        changed = unlockSelectedArchetype(archetypeId) || changed;
    }

    return changed;
}


bool ProfileManager::unlockSelectedCard(const std::string& cardId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || cardId.empty()) {
        return false;
    }

    const bool wasLocked = !isCardUnlocked(*profile, cardId);
    if (wasLocked) {
        profile->unlockedCardIds.push_back(cardId);
        appendProgressEntry(*profile, "card_unlocked", cardId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasLocked;
}

bool ProfileManager::unlockSelectedRelic(const std::string& relicId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || relicId.empty()) {
        return false;
    }

    const bool wasLocked = !isRelicUnlocked(*profile, relicId);
    if (wasLocked) {
        profile->unlockedRelicIds.push_back(relicId);
        appendProgressEntry(*profile, "relic_unlocked", relicId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasLocked;
}

bool ProfileManager::discoverSelectedEnemy(const std::string& enemyId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || enemyId.empty()) {
        return false;
    }

    const bool wasUnknown = !isEnemyDiscovered(*profile, enemyId);
    if (wasUnknown) {
        profile->discoveredEnemyIds.push_back(enemyId);
        appendProgressEntry(*profile, "enemy_discovered", enemyId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasUnknown;
}

bool ProfileManager::discoverSelectedStatus(const std::string& statusId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || statusId.empty()) {
        return false;
    }

    const bool wasUnknown = !isStatusDiscovered(*profile, statusId);
    if (wasUnknown) {
        profile->discoveredStatusIds.push_back(statusId);
        appendProgressEntry(*profile, "status_discovered", statusId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasUnknown;
}

bool ProfileManager::discoverSelectedConsumable(const std::string& consumableId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || consumableId.empty()) {
        return false;
    }

    const bool wasUnknown = !isConsumableDiscovered(*profile, consumableId);
    if (wasUnknown) {
        profile->discoveredConsumableIds.push_back(consumableId);
        appendProgressEntry(*profile, "consumable_discovered", consumableId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasUnknown;
}

bool ProfileManager::applySelectedUnlockReward(const UnlockReward& reward) {
    bool changed = false;

    changed = unlockSelectedArchetypes(reward.archetypeIds) || changed;
    for (const std::string& cardId : reward.cardIds) {
        changed = unlockSelectedCard(cardId) || changed;
    }
    for (const std::string& relicId : reward.relicIds) {
        changed = unlockSelectedRelic(relicId) || changed;
    }

    return changed;
}

bool ProfileManager::completeSelectedChallenge(const std::string& challengeId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || challengeId.empty()) {
        return false;
    }

    const bool wasIncomplete = !isChallengeCompleted(*profile, challengeId);
    if (wasIncomplete) {
        profile->completedChallengeIds.push_back(challengeId);
        appendProgressEntry(*profile, "challenge_completed", challengeId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasIncomplete;
}

std::vector<std::string> ProfileManager::completeSelectedChallenges(const std::vector<std::string>& challengeIds) {
    std::vector<std::string> completed;
    for (const std::string& challengeId : challengeIds) {
        if (completeSelectedChallenge(challengeId)) {
            completed.push_back(challengeId);
        }
    }

    return completed;
}

bool ProfileManager::completeSelectedAchievement(const std::string& achievementId) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr || achievementId.empty()) {
        return false;
    }

    const bool wasIncomplete = !isAchievementCompleted(*profile, achievementId);
    if (wasIncomplete) {
        profile->completedAchievementIds.push_back(achievementId);
        appendProgressEntry(*profile, "achievement_completed", achievementId);
        normalizeProfile(*profile, selectedSlotIndex_);
        saveSelectedProfile();
    }

    return wasIncomplete;
}

std::vector<std::string> ProfileManager::completeSelectedAchievements(const std::vector<std::string>& achievementIds) {
    std::vector<std::string> completed;
    for (const std::string& achievementId : achievementIds) {
        if (completeSelectedAchievement(achievementId)) {
            completed.push_back(achievementId);
        }
    }

    return completed;
}

bool ProfileManager::recordSelectedCardPlayed(const std::string& cardId, const int amount) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr) {
        return false;
    }

    if (!incrementCount(profile->cardPlayCounts, cardId, amount)) {
        return false;
    }

    normalizeProfile(*profile, selectedSlotIndex_);
    saveSelectedProfile();
    return true;
}

bool ProfileManager::recordSelectedRelicTaken(const std::string& relicId, const int amount) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr) {
        return false;
    }

    if (!incrementCount(profile->relicPickCounts, relicId, amount)) {
        return false;
    }

    normalizeProfile(*profile, selectedSlotIndex_);
    saveSelectedProfile();
    return true;
}


bool ProfileManager::recordSelectedRunFinished(const RunState& run, const bool victory) {
    ProfileData* profile = selectedProfile();
    if (profile == nullptr) {
        return false;
    }

    if (victory) {
        ++profile->victories;
    } else {
        ++profile->defeats;
    }
    addRunStats(profile->lifetimeRunStats, run.stats);

    normalizeProfile(*profile, selectedSlotIndex_);
    saveSelectedProfile();
    return true;
}

void ProfileManager::resetSlotsToEmpty() {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i].index = static_cast<int>(i);
        slots_[i].data = ProfileData{};
        slots_[i].data.slotIndex = static_cast<int>(i);
        slots_[i].data.isEmpty = true;
    }
}

void ProfileManager::ensureStarterUnlocks(ProfileData& profile) const {
    for (const char* archetypeId : starterArchetypeIds) {
        pushUnique(profile.unlockedArchetypeIds, archetypeId);
    }
}

void ProfileManager::appendProgressEntry(ProfileData& profile, const std::string& type, const std::string& contentId) const {
    if (type.empty() || contentId.empty()) {
        return;
    }

    const auto existing = std::find_if(
        profile.progressLog.begin(),
        profile.progressLog.end(),
        [&type, &contentId](const ProfileProgressEntry& entry) {
            return entry.type == type && entry.contentId == contentId;
        }
    );
    if (existing == profile.progressLog.end()) {
        profile.progressLog.push_back(ProfileProgressEntry{type, contentId});
    }
}

void ProfileManager::normalizeProfile(ProfileData& profile, const std::size_t index) const {
    profile.slotIndex = static_cast<int>(index);

    if (!profile.isEmpty) {
        ensureStarterUnlocks(profile);
    }

    profile.victories = std::max(profile.victories, 0);
    profile.defeats = std::max(profile.defeats, 0);
    normalizeRunStats(profile.lifetimeRunStats);

    normalizeStringList(profile.unlockedCardIds);
    normalizeStringList(profile.unlockedRelicIds);
    normalizeStringList(profile.unlockedArchetypeIds);
    normalizeStringList(profile.discoveredEnemyIds);
    normalizeStringList(profile.discoveredStatusIds);
    normalizeStringList(profile.discoveredConsumableIds);
    normalizeStringList(profile.completedChallengeIds);
    normalizeStringList(profile.completedAchievementIds);
    normalizeProgressLog(profile.progressLog);
    normalizeCountList(profile.cardPlayCounts);
    normalizeCountList(profile.relicPickCounts);
}
