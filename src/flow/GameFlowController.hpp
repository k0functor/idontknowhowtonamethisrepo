#pragma once

#include "archetypes/PlayableArchetypeId.hpp"
#include "combat/CombatResult.hpp"
#include "core/Random.hpp"
#include "data/ContentRegistry.hpp"
#include "flow/SceneManager.hpp"
#include "localization/LocalizationManager.hpp"
#include "profile/ProfileManager.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "run/DifficultyId.hpp"
#include "run/RunController.hpp"
#include "ui/UiFont.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

class GameFlowController {
public:
    GameFlowController(
        const ContentRegistry& content,
        const LocalizationManager& localization,
        Random& random,
        const std::filesystem::path& assetsPath
    );

    void update(float deltaSeconds);
    void render() const;

    bool exitRequested() const;

private:
    void queueTransition(std::function<void()> transition);
    void executePendingTransition();

    void setSplashScene();
    void setMainMenuScene();
    void setSaveSlotScene();
    void setProfileHubScene();
    void setDifficultySelectScene();
    void setRunMapScene();
    void setCombatScene(int nodeId);

    void showMainMenu();
    void showSaveSlots();
    void selectProfileSlot(std::size_t slotIndex);
    void selectArchetype(PlayableArchetypeId archetypeId);
    void selectDifficulty(DifficultyId difficultyId);
    void startMapNode(int nodeId);
    void finishReward(const RewardState& reward, RewardSelection selection);
    void requestExit();

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
    Random& random_;

    UiFont uiFont_;
    SceneManager sceneManager_;
    ProfileManager profileManager_;
    RunController runController_;

    std::optional<PlayableArchetypeId> selectedArchetypeId_;
    std::optional<DifficultyId> selectedDifficultyId_;

    std::function<void()> pendingTransition_;
    bool exitRequested_ = false;
};
