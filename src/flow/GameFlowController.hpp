#pragma once

#include <cstddef>

#include "active_items/ActiveItemUseContext.hpp"
#include "active_items/ActiveItemRerollSystem.hpp"
#include "archetypes/PlayableArchetypeId.hpp"
#include "cards/CardId.hpp"
#include "combat/CombatResult.hpp"
#include "core/Random.hpp"
#include "data/ContentRegistry.hpp"
#include "flow/SceneManager.hpp"
#include "localization/LocalizationManager.hpp"
#include "profile/ProfileManager.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "events/RunEventDefinition.hpp"
#include "events/RunEventChoiceResult.hpp"
#include "shop/ShopState.hpp"
#include "run/DifficultyId.hpp"
#include "run/RunController.hpp"
#include "run/RunEndReason.hpp"
#include "save/RunSaveSystem.hpp"
#include "settings/UserSettings.hpp"
#include "ui/UiFont.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct ProfileToast {
    std::string text;
    float remainingSeconds = 0.f;
};

class GameFlowController {
public:
    GameFlowController(
        const ContentRegistry& content,
        const LocalizationManager& localization,
        Random& random,
        const std::filesystem::path& assetsPath,
        const std::filesystem::path& savesPath,
        UserSettings& userSettings,
        std::function<void(const UserSettings&)> onUserSettingsChanged
    );

    void update(float deltaSeconds);
    void render() const;

    bool exitRequested() const;
    void notifyLocalizationChanged();

private:
    void queueTransition(std::function<void()> transition);
    void executePendingTransition();

    void setSplashScene();
    void setMainMenuScene();
    void setSettingsScene();
    void openSettingsOverlay();
    void closeSettingsOverlay();
    void saveAndExitRunToSaveSlots();
    void saveAndExitRunToProfileHub();
    void abandonActiveRun();
    bool shouldShowInGameSettingsButton() const;
    std::optional<ActiveItemUseContext> currentActiveItemContext() const;
    bool handleActiveItemShortcut();
    bool rerollRewardWithActiveItem(RewardState& reward, const RewardSelection& selection);
    bool rerollShopWithActiveItem(ShopState& shop);
    bool copyCardWithActiveItem(const CardId& cardId, ActiveItemUseContext context);
    void renderActiveItemHud() const;
    bool debugPanelEnabled() const;
    void toggleDebugPanel();
    void updateDebugPanel();
    void renderDebugPanel() const;
    void submitDebugCommand();
    std::vector<std::string> debugAutocompleteSuggestions() const;
    void acceptDebugAutocompleteSuggestion();
    std::string executeDebugCommand(const std::string& command);
    bool executeRunDebugCommand(const std::vector<std::string>& tokens, std::string& output);
    void addDebugMessage(std::string message);
    void setSaveSlotScene();
    std::string runSaveSummaryText(std::size_t slotIndex) const;
    void setProfileHubScene();
    void setChallengeScene();
    void setAchievementScene();
    void startChallengeRun(const std::string& challengeId);
    void applyChallengeLoadout(const ChallengeDefinition& challenge);
    std::string challengeRunLabel(const RunState& run) const;
    std::string challengeRunGoalLabel(const RunState& run) const;
    std::string challengeRunProgressLabel(const RunState& run) const;
    void setCompendiumScene();
    void setProfileProgressScene();
    void setDifficultySelectScene();
    void setRunMapScene();
    void setFloorCompleteScene();
    void setRunDefeatScene();
    void setRunCompleteScene(RunState completedRun, RunEndReason reason);
    bool setPendingRoomSceneIfNeeded();
    bool setPendingRoomSceneForNodeIfNeeded(int nodeId);
    void setCombatScene(int nodeId);
    void setCombatRewardScene(int nodeId, RewardState reward);
    void setChestRewardScene(int nodeId, RewardState reward);
    void setShopScene(int nodeId, ShopState shopState);
    void setMerchantRestScene(int nodeId, ShopState shopState);
    void setEventScene(int nodeId, const RunEventDefinition& event);
    void setEventOutcomeScene(RunEventChoiceResult result);

    void showMainMenu();
    void showSettings();
    void showSaveSlots();
    void startNewRunInSlot(std::size_t slotIndex);
    void continueRunInSlot(std::size_t slotIndex);
    void deleteRunInSlot(std::size_t slotIndex);
    void selectArchetype(PlayableArchetypeId archetypeId);
    void selectDifficulty(DifficultyId difficultyId);
    void startMapNode(int nodeId);
    void restHeal(int nodeId);
    void restCalm(int nodeId);
    void restUpgrade(int nodeId, std::size_t deckIndex);
    void restSkip(int nodeId);
    bool purchaseShopItem(const ShopPurchase& purchase);
    void updatePendingShopState(int nodeId, const ShopState& shopState);
    void finishShop(int nodeId);
    void finishEvent(int nodeId, const RunEventChoiceDefinition& choice);
    void finishReward(const RewardState& reward, RewardSelection selection);
    void finishChestReward(int nodeId, RewardSelection selection);
    void finishFloorCompleteContinue();
    void finishFloorCompleteMainMenu();
    bool advanceCompletedRunToNextFloor();
    bool completedRunCanContinueToNextFloor() const;
    void saveAndCloseCompletedRunForLater();
    void finishCompletedRunAndDeleteSave();
    void finishRun(RunEndReason reason);
    RunEndReason completedRunEndReason(const RunState& run) const;
    RunEndReason defeatedRunEndReason(const RunState& run) const;
    void grantFloorCompletionUnlocks(const RunState& run);
    void recordProfileStatsFromCombatResult(const CombatResult& result);
    void discoverProfileContentFromCombatResult(const CombatResult& result);
    void unlockProfileContentFromRunState(const RunState& run);
    void unlockProfileContentFromRewardSelection(const RewardSelection& selection);
    void unlockProfileContentFromShopPurchase(const ShopPurchase& purchase);
    void unlockProfileContentFromEventOutcome(const RunEventChoiceResult& result);
    bool unlockArchetypeAndToast(const std::string& archetypeId);
    bool unlockCardAndToast(const std::string& cardId);
    bool unlockRelicAndToast(const std::string& relicId);
    bool discoverEnemyAndToast(const std::string& enemyId);
    bool discoverStatusAndToast(const std::string& statusId);
    bool discoverConsumableAndToast(const std::string& consumableId);
    void applyUnlockRewardAndToast(const UnlockReward& reward);
    void pushProfileToast(std::string message);
    void updateProfileToasts(float deltaSeconds);
    void renderProfileToasts() const;
    void completeEligibleChallenges(const RunState& run);
    void completeEligibleAchievements(const RunState* run);
    void finishRunDefeatToProfileHub();
    void finishRunDefeatToMainMenu();
    void finishDefeatedRunAndDeleteSave();
    void requestExit();

    bool hasRunSave(std::size_t slotIndex) const;
    void saveActiveRun();
    void deleteSelectedRunSave();

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
    Random& random_;
    UserSettings& userSettings_;
    std::function<void(const UserSettings&)> onUserSettingsChanged_;

    UiFont uiFont_;
    SceneManager sceneManager_;
    std::unique_ptr<Scene> settingsOverlay_;
    ProfileManager profileManager_;
    RunController runController_;
    RunSaveSystem runSaveSystem_;

    std::optional<PlayableArchetypeId> selectedArchetypeId_;
    std::optional<DifficultyId> selectedDifficultyId_;

    std::function<void()> pendingTransition_;
    bool exitRequested_ = false;

    bool debugPanelOpen_ = false;
    std::string debugInput_;
    float debugBackspaceHeldSeconds_ = 0.f;
    float debugBackspaceRepeatSeconds_ = 0.f;
    std::vector<std::string> debugMessages_;
    std::vector<ProfileToast> profileToasts_;
    std::string saveSlotStatusMessage_;
};
