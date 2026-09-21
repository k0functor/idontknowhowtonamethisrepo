#pragma once

#include <string>
#include <vector>

class ContentRegistry;
class LocalizationManager;
class ProfileManager;
struct CombatResult;
struct RewardSelection;
struct ShopPurchase;
struct RunEventChoiceResult;
struct RunState;
struct UnlockReward;

struct ProfileToast {
    std::string text;
    float remainingSeconds = 0.f;
};

class ProfileProgressionService {
public:
    ProfileProgressionService(
        const ContentRegistry& content,
        const LocalizationManager& localization,
        ProfileManager& profileManager
    );

    void recordCombatStats(const CombatResult& result);
    void discoverCombatContent(const CombatResult& result);
    void synchronizeRunContent(const RunState& run);
    void applyRewardSelection(const RewardSelection& selection);
    void applyShopPurchase(const ShopPurchase& purchase);
    void applyEventOutcome(const RunEventChoiceResult& result);
    void grantFloorCompletionUnlocks(const RunState& run);
    void completeEligibleChallenges(const RunState& run);
    void completeEligibleAchievements(const RunState* run);

    void update(float deltaSeconds);
    const std::vector<ProfileToast>& toasts() const;
    void pushToast(std::string message);

    bool unlockCardAndToast(const std::string& cardId);
    bool unlockRelicAndToast(const std::string& relicId);
    bool discoverConsumableAndToast(const std::string& consumableId);

private:
    bool unlockArchetypeAndToast(const std::string& archetypeId);
    bool discoverEnemyAndToast(const std::string& enemyId);
    bool discoverStatusAndToast(const std::string& statusId);
    void applyUnlockRewardAndToast(const UnlockReward& reward);

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
    ProfileManager& profileManager_;
    std::vector<ProfileToast> toasts_;
};
