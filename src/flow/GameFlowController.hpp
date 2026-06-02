#pragma once

#include <cstddef>

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
#include "shop/ShopState.hpp"
#include "run/DifficultyId.hpp"
#include "run/RunController.hpp"
#include "save/RunSaveSystem.hpp"
#include "settings/UserSettings.hpp"
#include "ui/UiFont.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
    bool shouldShowInGameSettingsButton() const;
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
    void setProfileHubScene();
    void setDifficultySelectScene();
    void setRunMapScene();
    void setFloorCompleteScene();
    void setRunDefeatScene();
    bool setPendingRoomSceneIfNeeded();
    bool setPendingRoomSceneForNodeIfNeeded(int nodeId);
    void setCombatScene(int nodeId);
    void setCombatRewardScene(int nodeId, RewardState reward);
    void setChestRewardScene(int nodeId, RewardState reward);
    void setShopScene(int nodeId, ShopState shopState);
    void setMerchantRestScene(int nodeId, ShopState shopState);
    void setEventScene(int nodeId, const RunEventDefinition& event);

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
    void finishCompletedRunAndDeleteSave();
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
    std::string saveSlotStatusMessage_;
};
