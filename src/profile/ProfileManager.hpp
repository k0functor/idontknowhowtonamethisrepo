#pragma once

#include "profile/ProfileSaveSystem.hpp"
#include "profile/ProfileSlot.hpp"
#include "profile/UnlockReward.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct RunState;

class ProfileManager {
public:
    ProfileManager();
    explicit ProfileManager(std::filesystem::path savesRoot);

    void setSavesRoot(std::filesystem::path savesRoot);
    void loadAllProfiles();

    const std::array<ProfileSlot, 3>& slots() const;

    ProfileData& selectSlot(std::size_t index);
    const ProfileData* selectedProfile() const;
    ProfileData* selectedProfile();

    std::size_t selectedSlotIndex() const;

    void saveProfile(std::size_t index) const;
    void saveSelectedProfile() const;
    void deleteProfile(std::size_t index);

    bool isArchetypeUnlocked(const ProfileData& profile, const std::string& archetypeId) const;
    bool isSelectedArchetypeUnlocked(const std::string& archetypeId) const;

    bool isCardUnlocked(const ProfileData& profile, const std::string& cardId) const;
    bool isSelectedCardUnlocked(const std::string& cardId) const;
    bool isRelicUnlocked(const ProfileData& profile, const std::string& relicId) const;
    bool isSelectedRelicUnlocked(const std::string& relicId) const;

    bool isEnemyDiscovered(const ProfileData& profile, const std::string& enemyId) const;
    bool isSelectedEnemyDiscovered(const std::string& enemyId) const;
    bool isStatusDiscovered(const ProfileData& profile, const std::string& statusId) const;
    bool isSelectedStatusDiscovered(const std::string& statusId) const;
    bool isConsumableDiscovered(const ProfileData& profile, const std::string& consumableId) const;
    bool isSelectedConsumableDiscovered(const std::string& consumableId) const;

    bool isChallengeCompleted(const ProfileData& profile, const std::string& challengeId) const;
    bool isSelectedChallengeCompleted(const std::string& challengeId) const;

    bool isAchievementCompleted(const ProfileData& profile, const std::string& achievementId) const;
    bool isSelectedAchievementCompleted(const std::string& achievementId) const;

    bool unlockSelectedArchetype(const std::string& archetypeId);
    bool unlockSelectedArchetypes(const std::vector<std::string>& archetypeIds);

    bool unlockSelectedCard(const std::string& cardId);
    bool unlockSelectedRelic(const std::string& relicId);
    bool discoverSelectedEnemy(const std::string& enemyId);
    bool discoverSelectedStatus(const std::string& statusId);
    bool discoverSelectedConsumable(const std::string& consumableId);
    bool applySelectedUnlockReward(const UnlockReward& reward);
    bool completeSelectedChallenge(const std::string& challengeId);
    std::vector<std::string> completeSelectedChallenges(const std::vector<std::string>& challengeIds);
    bool completeSelectedAchievement(const std::string& achievementId);
    std::vector<std::string> completeSelectedAchievements(const std::vector<std::string>& achievementIds);

    bool recordSelectedCardPlayed(const std::string& cardId, int amount = 1);
    bool recordSelectedRelicTaken(const std::string& relicId, int amount = 1);
    bool recordSelectedRunFinished(const RunState& run, bool victory);

private:
    void resetSlotsToEmpty();
    void ensureStarterUnlocks(ProfileData& profile) const;
    void appendProgressEntry(ProfileData& profile, const std::string& type, const std::string& contentId) const;
    void normalizeProfile(ProfileData& profile, std::size_t index) const;

private:
    std::array<ProfileSlot, 3> slots_{};
    std::size_t selectedSlotIndex_ = 0;
    bool hasSelectedSlot_ = false;
    ProfileSaveSystem saveSystem_;
};
