#include "GameFlowController.hpp"

#include "scenes/CombatScene.hpp"
#include "scenes/DifficultySelectScene.hpp"
#include "scenes/MainMenuScene.hpp"
#include "scenes/ProfileHubScene.hpp"
#include "scenes/RewardScene.hpp"
#include "scenes/RunMapScene.hpp"
#include "scenes/SaveSlotScene.hpp"
#include "scenes/SplashScene.hpp"

#include <iostream>
#include <stdexcept>

GameFlowController::GameFlowController(
    const ContentRegistry& content,
    const LocalizationManager& localization,
    Random& random,
    const std::filesystem::path& assetsPath
)
    : content_(content),
      localization_(localization),
      random_(random) {
    if (!uiFont_.loadFromAssetsDirectory(assetsPath)) {
        std::cout << "UI font was not found. Put a Unicode font at assets/fonts/main.ttf.\n";
    } else {
        std::cout << "Loaded UI font: " << uiFont_.loadedPath().string() << '\n';
    }

    setSplashScene();
}

void GameFlowController::update(const float deltaSeconds) {
    sceneManager_.update(deltaSeconds);
    executePendingTransition();
}

void GameFlowController::render() const {
    sceneManager_.render();
}

bool GameFlowController::exitRequested() const {
    return exitRequested_;
}

void GameFlowController::queueTransition(std::function<void()> transition) {
    pendingTransition_ = std::move(transition);
}

void GameFlowController::executePendingTransition() {
    if (!pendingTransition_) {
        return;
    }

    std::function<void()> transition = std::move(pendingTransition_);
    pendingTransition_ = nullptr;
    transition();
}

void GameFlowController::setSplashScene() {
    sceneManager_.setScene(
        std::make_unique<SplashScene>(
            uiFont_,
            [this]() { showMainMenu(); }
        )
    );
}

void GameFlowController::setMainMenuScene() {
    sceneManager_.setScene(
        std::make_unique<MainMenuScene>(
            uiFont_,
            [this]() { showSaveSlots(); },
            [this]() { requestExit(); }
        )
    );
}

void GameFlowController::setSaveSlotScene() {
    sceneManager_.setScene(
        std::make_unique<SaveSlotScene>(
            uiFont_,
            profileManager_,
            [this](const std::size_t slotIndex) { selectProfileSlot(slotIndex); },
            [this]() { showMainMenu(); }
        )
    );
}

void GameFlowController::setProfileHubScene() {
    sceneManager_.setScene(
        std::make_unique<ProfileHubScene>(
            uiFont_,
            localization_,
            content_.actors(),
            content_.cards(),
            content_.archetypes().all(),
            [this](PlayableArchetypeId archetypeId) { selectArchetype(std::move(archetypeId)); },
            [this]() { showSaveSlots(); }
        )
    );
}

void GameFlowController::setDifficultySelectScene() {
    sceneManager_.setScene(
        std::make_unique<DifficultySelectScene>(
            uiFont_,
            localization_,
            content_.difficulties().all(),
            [this](DifficultyId difficultyId) { selectDifficulty(std::move(difficultyId)); },
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::setRunMapScene() {
    if (!runController_.hasActiveRun()) {
        throw std::runtime_error("Cannot open run map: no active run");
    }

    sceneManager_.setScene(
        std::make_unique<RunMapScene>(
            uiFont_,
            runController_.run(),
            [this](const int nodeId) { startMapNode(nodeId); },
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::setCombatScene(const int nodeId) {
    if (!runController_.hasActiveRun()) {
        throw std::runtime_error("Cannot start combat: no active run");
    }

    sceneManager_.setScene(
        std::make_unique<CombatScene>(
            content_,
            localization_,
            random_,
            uiFont_,
            runController_.run(),
            [this, nodeId](const CombatResult& result) {
                queueTransition([this, nodeId, result]() {
                    pendingReward_ = runController_.completeCombatAndCreateReward(
                        nodeId,
                        result,
                        content_.cards(),
                        content_.relics(),
                        random_
                    );
                    setRewardScene();
                });
            },
            [this](const CombatResult&) { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::setRewardScene() {
    if (!pendingReward_.has_value()) {
        throw std::runtime_error("Cannot open reward scene: no pending reward");
    }

    sceneManager_.setScene(
        std::make_unique<RewardScene>(
            uiFont_,
            localization_,
            content_.cards(),
            *pendingReward_,
            [this](RewardSelection selection) { finishReward(std::move(selection)); }
        )
    );
}

void GameFlowController::showMainMenu() {
    queueTransition([this]() { setMainMenuScene(); });
}

void GameFlowController::showSaveSlots() {
    queueTransition([this]() { setSaveSlotScene(); });
}

void GameFlowController::selectProfileSlot(const std::size_t slotIndex) {
    queueTransition([this, slotIndex]() {
        profileManager_.selectSlot(slotIndex);
        setProfileHubScene();
    });
}

void GameFlowController::selectArchetype(PlayableArchetypeId archetypeId) {
    queueTransition([this, archetypeId = std::move(archetypeId)]() mutable {
        selectedArchetypeId_ = std::move(archetypeId);
        setDifficultySelectScene();
    });
}

void GameFlowController::selectDifficulty(DifficultyId difficultyId) {
    queueTransition([this, difficultyId = std::move(difficultyId)]() mutable {
        if (!selectedArchetypeId_.has_value()) {
            throw std::runtime_error("Cannot select difficulty before archetype");
        }

        selectedDifficultyId_ = difficultyId;

        const PlayableArchetypeDefinition& archetype = content_.archetypes().get(*selectedArchetypeId_);
        const DifficultyDefinition& difficulty = content_.difficulties().get(difficultyId);

        runController_.startNewRun(archetype, difficulty, random_.seed());
        setRunMapScene();
    });
}

void GameFlowController::startMapNode(const int nodeId) {
    queueTransition([this, nodeId]() {
        runController_.startNode(nodeId);
        setCombatScene(nodeId);
    });
}

void GameFlowController::finishReward(RewardSelection selection) {
    queueTransition([this, selection = std::move(selection)]() mutable {
        if (!pendingReward_.has_value()) {
            throw std::runtime_error("Cannot finish reward: no pending reward");
        }

        runController_.applyReward(*pendingReward_, selection);
        pendingReward_.reset();
        setRunMapScene();
    });
}

void GameFlowController::requestExit() {
    queueTransition([this]() { exitRequested_ = true; });
}
