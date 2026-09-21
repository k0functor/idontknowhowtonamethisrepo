#include "GameFlowController.hpp"

#include "active_items/ActiveItemAcquisitionSystem.hpp"
#include "active_items/ActiveItemId.hpp"
#include "active_items/ActiveItemSystem.hpp"
#include "active_items/ActiveItemContextSystem.hpp"
#include "active_items/ActiveItemRerollSystem.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardDefinition.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "consumables/ConsumableId.hpp"
#include "events/RunEventSelector.hpp"
#include "challenges/ChallengeRules.hpp"
#include "relics/RelicDefinition.hpp"
#include "relics/RelicId.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "progression/AchievementEvaluator.hpp"
#include "progression/ChallengeEvaluator.hpp"
#include "progression/UnlockEvaluator.hpp"
#include "run/RunCardEligibility.hpp"
#include "scenes/AchievementScene.hpp"
#include "scenes/ChallengeScene.hpp"
#include "scenes/CompendiumScene.hpp"
#include "scenes/CombatScene.hpp"
#include "scenes/DifficultySelectScene.hpp"
#include "scenes/EventScene.hpp"
#include "scenes/EventOutcomeScene.hpp"
#include "scenes/FloorCompleteScene.hpp"
#include "run/RunCompletion.hpp"
#include "run/RunRelicOwnership.hpp"
#include "run/RunPacingTracker.hpp"
#include "scenes/MainMenuScene.hpp"
#include "scenes/MerchantRestScene.hpp"
#include "scenes/ProfileHubScene.hpp"
#include "scenes/ProfileProgressScene.hpp"
#include "scenes/RewardScene.hpp"
#include "scenes/RunMapScene.hpp"
#include "scenes/RunDefeatScene.hpp"
#include "scenes/RunCompleteScene.hpp"
#include "scenes/SaveSlotScene.hpp"
#include "scenes/SettingsScene.hpp"
#include "scenes/ShopScene.hpp"
#include "scenes/SplashScene.hpp"
#include "shop/ShopTuning.hpp"
#include "shop/ShopGenerator.hpp"
#include "statuses/StatusDefinition.hpp"
#include "telemetry/RunTelemetryWriter.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <raylib.h>

namespace {

Rectangle inGameSettingsButtonBounds() {
    constexpr float width = 170.f;
    constexpr float height = 42.f;
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) - width - 24.f,
        24.f,
        width,
        height
    };
}

Rectangle debugPanelBounds() {
    const float width = std::min(760.f, static_cast<float>(VirtualViewport::width()) - 64.f);
    const float height = std::min(520.f, static_cast<float>(VirtualViewport::height()) - 64.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

std::string toLowerAscii(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

std::vector<std::string> splitCommand(const std::string& command) {
    std::istringstream input(command);
    std::vector<std::string> result;
    std::string token;
    while (input >> token) {
        result.push_back(toLowerAscii(token));
    }
    return result;
}

int parseIntOr(const std::vector<std::string>& tokens, const std::size_t index, const int fallback) {
    if (index >= tokens.size()) {
        return fallback;
    }

    try {
        return std::stoi(tokens[index]);
    } catch (...) {
        return fallback;
    }
}

std::string joinChallengeProgressParts(const std::vector<std::string>& parts) {
    std::string result;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index > 0) {
            result += "; ";
        }
        result += parts[index];
    }
    return result;
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back({});
    }
    return lines;
}

bool startsWithText(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string trimLeft(std::string value) {
    value.erase(
        value.begin(),
        std::find_if(value.begin(), value.end(), [](const unsigned char character) {
            return !std::isspace(character);
        })
    );
    return value;
}

std::string partialAfterPrefix(const std::string& lowerInput, const std::string& prefix) {
    if (!startsWithText(lowerInput, prefix)) {
        return {};
    }
    return trimLeft(lowerInput.substr(prefix.size()));
}

void pushLimited(std::vector<std::string>& output, std::string value, const std::size_t limit) {
    if (value.empty() || output.size() >= limit) {
        return;
    }

    if (std::find(output.begin(), output.end(), value) == output.end()) {
        output.push_back(std::move(value));
    }
}

std::string commandText(std::initializer_list<std::string_view> tokens) {
    std::string result;
    for (const std::string_view token : tokens) {
        if (!result.empty()) {
            result.push_back(' ');
        }
        result.append(token.data(), token.size());
    }
    return result;
}

std::string commandPrefix(std::initializer_list<std::string_view> tokens) {
    std::string result = commandText(tokens);
    result.push_back(' ');
    return result;
}

const char* debugRunPhaseName(const RunPhase phase) {
    switch (phase) {
        case RunPhase::Map: return "map";
        case RunPhase::Combat: return "combat";
        case RunPhase::Reward: return "reward";
        case RunPhase::Event: return "event";
        case RunPhase::Shop: return "shop";
        case RunPhase::Rest: return "rest";
        case RunPhase::Chest: return "chest";
        case RunPhase::FloorComplete: return "floor_complete";
        case RunPhase::RunComplete: return "run_complete";
    }
    return "unknown";
}

const char* debugPendingRoomName(const RunPendingRoomType type) {
    switch (type) {
        case RunPendingRoomType::None: return "none";
        case RunPendingRoomType::CombatReward: return "combat_reward";
        case RunPendingRoomType::ChestReward: return "chest_reward";
        case RunPendingRoomType::Shop: return "shop";
        case RunPendingRoomType::MerchantRest: return "merchant_rest";
        case RunPendingRoomType::Event: return "event";
    }
    return "unknown";
}
}

GameFlowController::GameFlowController(
    const ContentRegistry& content,
    const LocalizationManager& localization,
    Random& random,
    const std::filesystem::path& assetsPath,
    const std::filesystem::path& savesPath,
    UserSettings& userSettings,
    std::function<void(const UserSettings&)> onUserSettingsChanged
)
    : content_(content),
      localization_(localization),
      random_(random),
      userSettings_(userSettings),
      onUserSettingsChanged_(std::move(onUserSettingsChanged)),
      profileManager_(savesPath),
      profileProgression_(content_, localization_, profileManager_),
      runSaveSystem_(savesPath) {
    if (!uiFont_.loadFromAssetsDirectory(assetsPath)) {
        std::cout << "UI font was not found. Put a Unicode font at assets/fonts/main.ttf.\n";
    } else {
        std::cout << "Loaded UI font: " << uiFont_.loadedPath().string() << '\n';
    }

    setSplashScene();
}

void GameFlowController::update(const float deltaSeconds) {
    profileProgression_.update(deltaSeconds);

    if (debugPanelEnabled() && IsKeyPressed(KEY_F1)) {
        toggleDebugPanel();
        return;
    }

    if (debugPanelOpen_) {
        updateDebugPanel();
        executePendingTransition();
        return;
    }

    if (settingsOverlay_ != nullptr) {
        settingsOverlay_->update(deltaSeconds);
        executePendingTransition();
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (shouldShowInGameSettingsButton() &&
        BasicUi::contains(inGameSettingsButtonBounds(), mouse) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        openSettingsOverlay();
        return;
    }

    if (shouldShowInGameSettingsButton() && IsKeyPressed(KEY_F10)) {
        openSettingsOverlay();
        return;
    }

    if (handleActiveItemShortcut()) {
        executePendingTransition();
        return;
    }

    if (runController_.hasActiveRun()) {
        RunPacingTracker::tick(runController_.run(), deltaSeconds);
    }

    sceneManager_.update(deltaSeconds);
    executePendingTransition();
}

void GameFlowController::render() const {
    sceneManager_.render();

    if (settingsOverlay_ != nullptr) {
        DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 170});
        settingsOverlay_->render();
        return;
    }

    if (debugPanelOpen_) {
        DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 150});
        renderDebugPanel();
        return;
    }

    renderActiveItemHud();
    renderProfileToasts();

    if (shouldShowInGameSettingsButton()) {
        BasicUi::drawButton(
            uiFont_,
            inGameSettingsButtonBounds(),
            localization_.get(TextId("ui.settings")),
            GetMousePosition()
        );
    }

}

bool GameFlowController::exitRequested() const {
    return exitRequested_;
}

void GameFlowController::notifyLocalizationChanged() {
    sceneManager_.notifyLocalizationChanged();

    if (settingsOverlay_ != nullptr) {
        settingsOverlay_->onLocalizationChanged();
    }
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
            localization_,
            [this]() { showSaveSlots(); },
            [this]() { showSettings(); },
            [this]() { requestExit(); }
        )
    );
}


void GameFlowController::setSettingsScene() {
    sceneManager_.setScene(
        std::make_unique<SettingsScene>(
            uiFont_,
            localization_,
            userSettings_,
            [this](const UserSettings& settings) {
                userSettings_ = settings;
                if (onUserSettingsChanged_) {
                    onUserSettingsChanged_(userSettings_);
                }
            },
            [this]() { showMainMenu(); }
        )
    );
}

void GameFlowController::openSettingsOverlay() {
    settingsOverlay_ = std::make_unique<SettingsScene>(
        uiFont_,
        localization_,
        userSettings_,
        [this](const UserSettings& settings) {
            userSettings_ = settings;
            if (onUserSettingsChanged_) {
                onUserSettingsChanged_(userSettings_);
            }
        },
        [this]() { closeSettingsOverlay(); },
        [this]() { saveAndExitRunToSaveSlots(); }
    );
}

void GameFlowController::closeSettingsOverlay() {
    settingsOverlay_.reset();
}

void GameFlowController::saveAndExitRunToSaveSlots() {
    if (runController_.hasActiveRun() && runController_.isActCompleted()) {
        saveAndCloseCompletedRunForLater();
    } else {
        saveActiveRun();
        runController_.clearActiveRun();
    }

    settingsOverlay_.reset();
    selectedArchetypeId_.reset();
    selectedDifficultyId_.reset();
    queueTransition([this]() { setSaveSlotScene(); });
}

void GameFlowController::saveAndExitRunToProfileHub() {
    if (runController_.hasActiveRun() && runController_.isActCompleted()) {
        saveAndCloseCompletedRunForLater();
    } else {
        saveActiveRun();
        runController_.clearActiveRun();
    }

    settingsOverlay_.reset();
    debugPanelOpen_ = false;
    selectedArchetypeId_.reset();
    selectedDifficultyId_.reset();
    queueTransition([this]() { setProfileHubScene(); });
}

void GameFlowController::abandonActiveRun() {
    queueTransition([this]() {
        if (!runController_.hasActiveRun()) {
            setProfileHubScene();
            return;
        }

        RunState completedRun = runController_.run();
        finishRun(RunEndReason::Abandoned);
        setRunCompleteScene(std::move(completedRun), RunEndReason::Abandoned);
    });
}

bool GameFlowController::shouldShowInGameSettingsButton() const {
    return runController_.hasActiveRun() && !runController_.isActCompleted();
}

std::optional<ActiveItemUseContext> GameFlowController::currentActiveItemContext() const {
    if (!runController_.hasActiveRun() || runController_.isActCompleted()) {
        return std::nullopt;
    }

    switch (runController_.run().phase) {
        case RunPhase::Map: return ActiveItemUseContext::Map;
        // Combat active items need access to the live CombatState rather than the
        // serialized RunState. They are deliberately delegated to CombatScene.
        case RunPhase::Combat: return std::nullopt;
        case RunPhase::Reward: return ActiveItemUseContext::Reward;
        case RunPhase::Shop: return ActiveItemUseContext::Shop;
        case RunPhase::Chest: return ActiveItemUseContext::Chest;
        case RunPhase::Event: return ActiveItemUseContext::Event;
        case RunPhase::Rest: return ActiveItemUseContext::Rest;
        case RunPhase::FloorComplete:
        case RunPhase::RunComplete:
            return std::nullopt;
    }
    return std::nullopt;
}

bool GameFlowController::handleActiveItemShortcut() {
    if (!IsKeyPressed(KEY_SPACE) || !runController_.hasActiveRun()) {
        return false;
    }

    RunState& run = runController_.run();
    if (run.activeItem.empty()) {
        return false;
    }

    const ActiveItemId itemId(run.activeItem.itemId);
    if (!content_.activeItems().contains(itemId)) {
        profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.unknown")));
        return true;
    }

    const ActiveItemDefinition& definition = content_.activeItems().get(itemId);
    const std::optional<ActiveItemUseContext> context = currentActiveItemContext();
    if (!context.has_value() || std::find(definition.useContexts.begin(), definition.useContexts.end(), *context) == definition.useContexts.end()) {
        return false;
    }

    if (ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::RerollOffers) ||
        ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::CopyCard)) {
        return false;
    }

    if (ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::CreateConsumable)) {
        if (!ActiveItemSystem::canUse(run.activeItem, definition, *context)) {
            profileProgression_.pushToast(localization_.format(
                TextId("active_item.feedback.not_charged"),
                {{"charge", std::to_string(run.activeItem.charge)}, {"cost", std::to_string(definition.chargeCost)}}
            ));
            return true;
        }

        const std::optional<std::string> created = ActiveItemContextSystem::createRandomConsumable(
            run,
            content_.consumables(),
            runController_.random()
        );
        if (!created.has_value()) {
            profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.consumables_full")));
            return true;
        }
        if (!ActiveItemSystem::spendCharge(run, definition, *context)) {
            return true;
        }
        saveActiveRun();
        const std::string consumableName = content_.consumables().contains(ConsumableId(*created))
            ? localization_.get(content_.consumables().get(ConsumableId(*created)).nameTextId)
            : *created;
        profileProgression_.pushToast(localization_.format(
            TextId("active_item.feedback.created_consumable"),
            {{"item", localization_.get(definition.nameTextId)}, {"consumable", consumableName}}
        ));
        return true;
    }

    if (ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::RerollMapChoices)) {
        if (!ActiveItemSystem::canUse(run.activeItem, definition, *context)) {
            profileProgression_.pushToast(localization_.format(
                TextId("active_item.feedback.not_charged"),
                {{"charge", std::to_string(run.activeItem.charge)}, {"cost", std::to_string(definition.chargeCost)}}
            ));
            return true;
        }
        const int changed = ActiveItemContextSystem::rerollAvailableMapNodes(run, runController_.random());
        if (changed <= 0) {
            profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.no_map_choices")));
            return true;
        }
        if (!ActiveItemSystem::spendCharge(run, definition, *context)) {
            return true;
        }
        saveActiveRun();
        profileProgression_.pushToast(localization_.format(
            TextId("active_item.feedback.map_rerolled"),
            {{"item", localization_.get(definition.nameTextId)}, {"count", std::to_string(changed)}}
        ));
        queueTransition([this]() { setRunMapScene(); });
        return true;
    }

    const ActiveItemUseResult result = ActiveItemSystem::use(run, definition, *context);
    if (result.status == ActiveItemUseStatus::NotEnoughCharge) {
        profileProgression_.pushToast(localization_.format(
            TextId("active_item.feedback.not_charged"),
            {{"charge", std::to_string(run.activeItem.charge)}, {"cost", std::to_string(definition.chargeCost)}}
        ));
        return true;
    }
    if (result.status == ActiveItemUseStatus::NoEffect) {
        profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.no_effect")));
        return true;
    }
    if (!result.used()) {
        return false;
    }

    saveActiveRun();
    profileProgression_.pushToast(localization_.format(
        TextId("active_item.feedback.used"),
        {{"item", localization_.get(definition.nameTextId)}}
    ));
    return true;
}

bool GameFlowController::rerollRewardWithActiveItem(
    RewardState& reward,
    const RewardSelection& selection
) {
    if (!runController_.hasActiveRun() || !runController_.hasPendingRoom()) {
        return false;
    }

    const RunPendingRoomState& pending = runController_.pendingRoom();
    const bool isCombatReward = pending.type == RunPendingRoomType::CombatReward;
    const bool isChestReward = pending.type == RunPendingRoomType::ChestReward;
    if (!isCombatReward && !isChestReward) {
        return false;
    }

    if (!selection.empty()) {
        profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.reward_selection_locked")));
        return false;
    }

    RunState& run = runController_.run();
    if (run.activeItem.empty() || !content_.activeItems().contains(ActiveItemId(run.activeItem.itemId))) {
        return false;
    }

    const ActiveItemDefinition& definition = content_.activeItems().get(ActiveItemId(run.activeItem.itemId));
    const ActiveItemUseContext context = isChestReward
        ? ActiveItemUseContext::Chest
        : ActiveItemUseContext::Reward;
    if (!ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::RerollOffers) ||
        std::find(definition.useContexts.begin(), definition.useContexts.end(), context) == definition.useContexts.end()) {
        return false;
    }

    if (!ActiveItemSystem::canUse(run.activeItem, definition, context)) {
        profileProgression_.pushToast(localization_.format(
            TextId("active_item.feedback.not_charged"),
            {{"charge", std::to_string(run.activeItem.charge)}, {"cost", std::to_string(definition.chargeCost)}}
        ));
        return false;
    }

    const ActiveItemRerollResult reroll = ActiveItemRerollSystem::rerollReward(
        reward,
        run,
        content_.cards(),
        content_.relics(),
        content_.consumables(),
        content_.activeItems(),
        content_.rewardTuning().node(reward.sourceNodeType),
        runController_.random()
    );
    if (!reroll.changed()) {
        profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.no_effect")));
        return false;
    }

    if (!ActiveItemSystem::spendCharge(run, definition, context)) {
        return false;
    }

    const int nodeId = pending.nodeId;
    if (isChestReward) {
        runController_.setPendingChestReward(nodeId, reward);
    } else {
        runController_.setPendingCombatReward(nodeId, reward);
    }
    saveActiveRun();
    profileProgression_.pushToast(localization_.format(
        TextId("active_item.feedback.rerolled_reward"),
        {{"item", localization_.get(definition.nameTextId)}, {"count", std::to_string(reroll.totalChanged())}}
    ));
    return true;
}

bool GameFlowController::rerollShopWithActiveItem(ShopState& shop) {
    if (!runController_.hasActiveRun() || !runController_.hasPendingRoom()) {
        return false;
    }

    const RunPendingRoomState& pending = runController_.pendingRoom();
    if (pending.type != RunPendingRoomType::Shop || shop.isMerchantRest()) {
        return false;
    }

    RunState& run = runController_.run();
    if (run.activeItem.empty() || !content_.activeItems().contains(ActiveItemId(run.activeItem.itemId))) {
        return false;
    }

    const ActiveItemDefinition& definition = content_.activeItems().get(ActiveItemId(run.activeItem.itemId));
    if (!ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::RerollOffers) ||
        std::find(definition.useContexts.begin(), definition.useContexts.end(), ActiveItemUseContext::Shop) == definition.useContexts.end()) {
        return false;
    }

    if (!ActiveItemSystem::canUse(run.activeItem, definition, ActiveItemUseContext::Shop)) {
        profileProgression_.pushToast(localization_.format(
            TextId("active_item.feedback.not_charged"),
            {{"charge", std::to_string(run.activeItem.charge)}, {"cost", std::to_string(definition.chargeCost)}}
        ));
        return false;
    }

    const ShopTuning& tuning = content_.shopTuning();
    const ActiveItemRerollResult reroll = ActiveItemRerollSystem::rerollShop(
        shop,
        run,
        content_.cards(),
        content_.relics(),
        content_.consumables(),
        content_.activeItems(),
        tuning.minimumCardPrice(),
        tuning.minimumConsumablePrice(),
        [&tuning](const RelicRarity rarity) { return tuning.relicPrice(rarity); },
        runController_.random()
    );
    if (!reroll.changed()) {
        profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.no_effect")));
        return false;
    }

    if (!ActiveItemSystem::spendCharge(run, definition, ActiveItemUseContext::Shop)) {
        return false;
    }

    runController_.setPendingShop(pending.nodeId, shop);
    saveActiveRun();
    profileProgression_.pushToast(localization_.format(
        TextId("active_item.feedback.rerolled_shop"),
        {{"item", localization_.get(definition.nameTextId)}, {"count", std::to_string(reroll.totalChanged())}}
    ));
    return true;
}

bool GameFlowController::copyCardWithActiveItem(
    const CardId& cardId,
    const ActiveItemUseContext context
) {
    if (!runController_.hasActiveRun()) {
        return false;
    }

    RunState& run = runController_.run();
    if (run.activeItem.empty() || !content_.activeItems().contains(ActiveItemId(run.activeItem.itemId))) {
        return false;
    }

    const ActiveItemDefinition& definition = content_.activeItems().get(ActiveItemId(run.activeItem.itemId));
    if (!ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::CopyCard) ||
        !ActiveItemSystem::canUse(run.activeItem, definition, context)) {
        if (ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::CopyCard)) {
            profileProgression_.pushToast(localization_.format(
                TextId("active_item.feedback.not_charged"),
                {{"charge", std::to_string(run.activeItem.charge)}, {"cost", std::to_string(definition.chargeCost)}}
            ));
        }
        return false;
    }

    if (cardId.value.empty()) {
        profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.select_card")));
        return false;
    }
    if (!ActiveItemContextSystem::copyCard(run, content_.cards(), cardId)) {
        profileProgression_.pushToast(localization_.get(TextId("active_item.feedback.no_effect")));
        return false;
    }
    if (!ActiveItemSystem::spendCharge(run, definition, context)) {
        return false;
    }

    saveActiveRun();
    const std::string cardName = content_.cards().contains(cardId)
        ? localization_.get(content_.cards().get(cardId).nameTextId)
        : cardId.value;
    profileProgression_.pushToast(localization_.format(
        TextId("active_item.feedback.card_copied"),
        {{"item", localization_.get(definition.nameTextId)}, {"card", cardName}}
    ));
    return true;
}

void GameFlowController::renderActiveItemHud() const {
    if (!runController_.hasActiveRun() || runController_.isActCompleted()) {
        return;
    }

    const Rectangle panel{24.f, 88.f, 360.f, 70.f};
    DrawRectangleRounded(panel, 0.16f, 8, Color{16, 19, 29, 225});
    DrawRectangleRoundedLinesEx(panel, 0.16f, 8, 1.5f, Color{104, 118, 164, 220});

    const RunState& run = runController_.run();
    if (run.activeItem.empty()) {
        BasicUi::drawText(uiFont_, localization_.get(TextId("active_item.slot.empty")), Vector2{panel.x + 12.f, panel.y + 10.f}, 18.f, Color{180, 186, 205, 255});
        BasicUi::drawText(uiFont_, localization_.get(TextId("active_item.hint.space")), Vector2{panel.x + 12.f, panel.y + 40.f}, 14.f, Color{124, 132, 156, 255});
        return;
    }

    const ActiveItemId itemId(run.activeItem.itemId);
    if (!content_.activeItems().contains(itemId)) {
        BasicUi::drawText(uiFont_, run.activeItem.itemId, Vector2{panel.x + 12.f, panel.y + 9.f}, 17.f, Color{225, 135, 135, 255});
        return;
    }

    const ActiveItemDefinition& definition = content_.activeItems().get(itemId);
    BasicUi::drawText(uiFont_, localization_.get(definition.nameTextId), Vector2{panel.x + 12.f, panel.y + 8.f}, 18.f, Color{235, 224, 185, 255});

    const Rectangle bar{panel.x + 12.f, panel.y + 36.f, 206.f, 16.f};
    DrawRectangleRounded(bar, 0.35f, 8, Color{42, 47, 65, 255});
    const float ratio = definition.maxCharge > 0
        ? std::clamp(static_cast<float>(run.activeItem.charge) / static_cast<float>(definition.maxCharge), 0.f, 1.f)
        : 0.f;
    if (ratio > 0.f) {
        Rectangle fill = bar;
        fill.width *= ratio;
        DrawRectangleRounded(fill, 0.35f, 8, Color{151, 116, 218, 255});
    }
    if (definition.maxCharge > 0) {
        const float useMarkerX = bar.x + bar.width * std::clamp(
            static_cast<float>(definition.chargeCost) / static_cast<float>(definition.maxCharge),
            0.f,
            1.f
        );
        DrawLineEx(Vector2{useMarkerX, bar.y - 3.f}, Vector2{useMarkerX, bar.y + bar.height + 3.f}, 2.f, Color{244, 213, 126, 255});
    }
    BasicUi::drawText(
        uiFont_,
        std::to_string(run.activeItem.charge) + "/" + std::to_string(definition.maxCharge) +
            "  (" + std::to_string(definition.chargeCost) + ")",
        Vector2{bar.x + 58.f, bar.y - 1.f},
        14.f,
        Color{242, 242, 248, 255}
    );

    const int roomsUntilReady = ActiveItemSystem::normalCombatRoomsUntilUsable(run.activeItem, definition);
    std::string readiness;
    Color readinessColor{170, 177, 202, 255};
    if (roomsUntilReady == 0) {
        readiness = localization_.get(TextId("active_item.charge.ready")) + "  " +
            localization_.get(TextId("active_item.hint.space"));
        readinessColor = Color{154, 222, 164, 255};
    } else if (roomsUntilReady > 0) {
        readiness = localization_.format(
            TextId("active_item.charge.rooms_short"),
            {{"count", std::to_string(roomsUntilReady)}}
        );
    } else {
        readiness = localization_.get(TextId("active_item.charge.no_combat_gain"));
    }
    BasicUi::drawText(uiFont_, readiness, Vector2{panel.x + 232.f, panel.y + 37.f}, 14.f, readinessColor);
}

bool GameFlowController::debugPanelEnabled() const {
    return userSettings_.debug.enabled;
}

void GameFlowController::toggleDebugPanel() {
    if (!debugPanelEnabled()) {
        debugPanelOpen_ = false;
        return;
    }

    debugPanelOpen_ = !debugPanelOpen_;
    if (debugPanelOpen_ && debugMessages_.empty()) {
        addDebugMessage(localization_.get(TextId("debug.panel.opened")));
    }
}

void GameFlowController::updateDebugPanel() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        debugPanelOpen_ = false;
        return;
    }

    int character = GetCharPressed();
    while (character > 0) {
        if (character >= 32 && character <= 126 && debugInput_.size() < 180u) {
            debugInput_.push_back(static_cast<char>(character));
        }
        character = GetCharPressed();
    }

    constexpr float backspaceInitialDelaySeconds = 0.32f;
    constexpr float backspaceRepeatIntervalSeconds = 0.045f;

    if (!IsKeyDown(KEY_BACKSPACE)) {
        debugBackspaceHeldSeconds_ = 0.f;
        debugBackspaceRepeatSeconds_ = 0.f;
    } else if (IsKeyPressed(KEY_BACKSPACE)) {
        if (!debugInput_.empty()) {
            debugInput_.pop_back();
        }
        debugBackspaceHeldSeconds_ = 0.f;
        debugBackspaceRepeatSeconds_ = 0.f;
    } else {
        const float deltaSeconds = GetFrameTime();
        debugBackspaceHeldSeconds_ += deltaSeconds;
        if (debugBackspaceHeldSeconds_ >= backspaceInitialDelaySeconds && !debugInput_.empty()) {
            debugBackspaceRepeatSeconds_ += deltaSeconds;
            while (debugBackspaceRepeatSeconds_ >= backspaceRepeatIntervalSeconds && !debugInput_.empty()) {
                debugInput_.pop_back();
                debugBackspaceRepeatSeconds_ -= backspaceRepeatIntervalSeconds;
            }
        }
    }

    if (IsKeyPressed(KEY_TAB)) {
        acceptDebugAutocompleteSuggestion();
        return;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        submitDebugCommand();
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = debugPanelBounds();
    const float quickY = panel.y + panel.height - 104.f;
    const Rectangle goldButton{panel.x + 24.f, quickY, 140.f, 38.f};
    const Rectangle healButton{panel.x + 176.f, quickY, 140.f, 38.f};
    const Rectangle saveButton{panel.x + 328.f, quickY, 140.f, 38.f};
    const Rectangle closeButton{panel.x + panel.width - 124.f, panel.y + 20.f, 96.f, 34.f};

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (BasicUi::contains(closeButton, mouse)) {
            debugPanelOpen_ = false;
            return;
        }
        if (BasicUi::contains(goldButton, mouse)) {
            debugInput_ = commandText({"give", "gold", "50"});
            submitDebugCommand();
            return;
        }
        if (BasicUi::contains(healButton, mouse)) {
            debugInput_ = "fullheal";
            submitDebugCommand();
            return;
        }
        if (BasicUi::contains(saveButton, mouse)) {
            debugInput_ = "save";
            submitDebugCommand();
            return;
        }
    }
}

void GameFlowController::renderDebugPanel() const {
    const Rectangle panel = debugPanelBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangleRounded(panel, 0.035f, 12, Color{22, 24, 32, 245});
    DrawRectangleRoundedLinesEx(panel, 0.035f, 12, 2.f, Color{125, 132, 158, 255});

    BasicUi::drawText(uiFont_, localization_.get(TextId("debug.panel.title")), Vector2{panel.x + 24.f, panel.y + 22.f}, 28.f, Color{240, 242, 250, 255});
    BasicUi::drawText(uiFont_, localization_.get(TextId("debug.panel.close_hint")), Vector2{panel.x + 24.f, panel.y + 56.f}, 17.f, Color{165, 172, 196, 255});
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + panel.width - 124.f, panel.y + 20.f, 96.f, 34.f}, localization_.get(TextId("ui.close")), mouse);

    const Rectangle input{panel.x + 24.f, panel.y + 88.f, panel.width - 48.f, 42.f};
    DrawRectangleRounded(input, 0.15f, 8, Color{12, 14, 20, 255});
    DrawRectangleRoundedLinesEx(input, 0.15f, 8, 2.f, Color{80, 86, 112, 255});
    BasicUi::drawText(uiFont_, "> " + debugInput_ + "_", Vector2{input.x + 12.f, input.y + 10.f}, 20.f, Color{235, 237, 245, 255});

    const std::vector<std::string> suggestions = debugAutocompleteSuggestions();
    if (!suggestions.empty()) {
        std::string suggestionBody = suggestions.front();
        const std::size_t previewCount = std::min<std::size_t>(suggestions.size(), 4u);
        for (std::size_t i = 1; i < previewCount; ++i) {
            suggestionBody += "  |  " + suggestions[i];
        }
        const std::string suggestionText = localization_.format(TextId("debug.panel.autocomplete"), {{"suggestions", suggestionBody}});
        BasicUi::drawText(uiFont_, suggestionText, Vector2{input.x + 12.f, input.y + 48.f}, 15.f, Color{170, 188, 235, 255});
    }

    const float logTop = panel.y + 166.f;
    const float logBottom = panel.y + panel.height - 124.f;
    const int maxLines = static_cast<int>((logBottom - logTop) / 22.f);
    const int start = std::max(0, static_cast<int>(debugMessages_.size()) - maxLines);
    float y = logTop;
    for (int i = start; i < static_cast<int>(debugMessages_.size()); ++i) {
        BasicUi::drawText(uiFont_, debugMessages_[static_cast<std::size_t>(i)], Vector2{panel.x + 24.f, y}, 17.f, Color{205, 211, 232, 255});
        y += 22.f;
    }

    const float quickY = panel.y + panel.height - 104.f;
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + 24.f, quickY, 140.f, 38.f}, localization_.get(TextId("debug.panel.button.gold_50")), mouse);
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + 176.f, quickY, 140.f, 38.f}, localization_.get(TextId("debug.panel.button.full_heal")), mouse);
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + 328.f, quickY, 140.f, 38.f}, localization_.get(TextId("debug.panel.button.save")), mouse);

    BasicUi::drawText(
        uiFont_,
        localization_.get(TextId("debug.panel.examples")),
        Vector2{panel.x + 24.f, panel.y + panel.height - 46.f},
        15.f,
        Color{150, 156, 180, 255}
    );
}

void GameFlowController::submitDebugCommand() {
    if (debugInput_.empty()) {
        return;
    }

    const std::string command = debugInput_;
    addDebugMessage("> " + command);
    for (const std::string& line : splitLines(executeDebugCommand(command))) {
        addDebugMessage(line);
    }
    debugInput_.clear();
}

std::vector<std::string> GameFlowController::debugAutocompleteSuggestions() const {
    constexpr std::size_t maxSuggestions = 12u;
    std::vector<std::string> suggestions;

    const std::string lowerInput = toLowerAscii(debugInput_);
    const std::vector<std::string> commands{
        "help",
        "save",
        "fullheal",
        commandText({"give", "gold", "100"}),
        commandPrefix({"give", "card"}),
        commandPrefix({"give", "relic"}),
        commandPrefix({"give", "consumable"}),
        commandPrefix({"give", "active"}),
        commandText({"charge", "active", "3"}),
        commandPrefix({"status"}),
        commandPrefix({"apply", "status"}),
        commandText({"stress", "10"}),
        commandText({"stress", "-10"}),
        commandText({"heal", "10"}),
        commandText({"damage", "10"}),
        commandText({"block", "10"}),
        commandText({"energy", "3"}),
        commandText({"win", "combat"}),
        commandText({"lose", "combat"}),
        commandText({"clear", "pending"}),
        commandText({"unlock", "map"}),
        commandText({"run", "state"}),
        commandText({"goto", "boss"}),
        commandText({"finish", "floor"}),
        commandText({"finish", "run", "victory"}),
        commandText({"finish", "run", "defeat"})
    };

    if (lowerInput.empty() || lowerInput.find(' ') == std::string::npos) {
        for (const std::string& command : commands) {
            if (lowerInput.empty() || startsWithText(command, lowerInput)) {
                pushLimited(suggestions, command, maxSuggestions);
            }
        }
        return suggestions;
    }

    auto suggestIds = [&](const std::string& prefix, const std::vector<std::string>& ids) {
        const std::string partial = partialAfterPrefix(lowerInput, prefix);
        if (!startsWithText(lowerInput, prefix)) {
            return;
        }

        for (const std::string& id : ids) {
            if (partial.empty() || startsWithText(toLowerAscii(id), partial)) {
                pushLimited(suggestions, prefix + id, maxSuggestions);
            }
        }
    };

    std::vector<std::string> cardIds;
    for (const CardDefinition* card : content_.cards().all()) {
        if (card != nullptr) {
            cardIds.push_back(card->id.value);
        }
    }
    std::sort(cardIds.begin(), cardIds.end());

    std::vector<std::string> relicIds;
    for (const RelicDefinition* relic : content_.relics().all()) {
        if (relic != nullptr) {
            relicIds.push_back(relic->id.value);
        }
    }
    std::sort(relicIds.begin(), relicIds.end());

    std::vector<std::string> consumableIds;
    for (const ConsumableDefinition* consumable : content_.consumables().all()) {
        if (consumable != nullptr) {
            consumableIds.push_back(consumable->id.value);
        }
    }
    std::sort(consumableIds.begin(), consumableIds.end());

    std::vector<std::string> activeItemIds;
    for (const ActiveItemDefinition* item : content_.activeItems().all()) {
        if (item != nullptr) activeItemIds.push_back(item->id.value);
    }
    std::sort(activeItemIds.begin(), activeItemIds.end());

    std::vector<std::string> statusIds;
    for (const StatusDefinition* status : content_.statuses().all()) {
        if (status != nullptr) {
            statusIds.push_back(status->id.value);
        }
    }
    std::sort(statusIds.begin(), statusIds.end());

    suggestIds(commandPrefix({"give", "card"}), cardIds);
    suggestIds(commandPrefix({"give", "relic"}), relicIds);
    suggestIds(commandPrefix({"give", "consumable"}), consumableIds);
    suggestIds(commandPrefix({"give", "potion"}), consumableIds);
    suggestIds(commandPrefix({"give", "active"}), activeItemIds);
    suggestIds(commandPrefix({"give", "active_item"}), activeItemIds);
    suggestIds(commandPrefix({"status"}), statusIds);
    suggestIds(commandPrefix({"apply", "status"}), statusIds);

    if (suggestions.empty()) {
        for (const std::string& command : commands) {
            if (startsWithText(command, lowerInput)) {
                pushLimited(suggestions, command, maxSuggestions);
            }
        }
    }

    return suggestions;
}

void GameFlowController::acceptDebugAutocompleteSuggestion() {
    const std::vector<std::string> suggestions = debugAutocompleteSuggestions();
    if (!suggestions.empty()) {
        debugInput_ = suggestions.front();
    }
}

std::string GameFlowController::executeDebugCommand(const std::string& command) {
    const std::vector<std::string> tokens = splitCommand(command);
    if (tokens.empty()) {
        return localization_.get(TextId("debug.command.empty"));
    }

    std::string sceneOutput;
    if (sceneManager_.handleDebugCommand(tokens, sceneOutput)) {
        return sceneOutput;
    }

    std::string runOutput;
    if (executeRunDebugCommand(tokens, runOutput)) {
        return runOutput;
    }

    return localization_.get(TextId("debug.command.unknown"));
}

bool GameFlowController::executeRunDebugCommand(const std::vector<std::string>& tokens, std::string& output) {
    if (tokens.empty()) {
        return false;
    }

    const std::string& command = tokens.front();

    if (command == "help") {
        output = localization_.get(TextId("debug.command.help"));
        return true;
    }

    if (command == "save") {
        saveActiveRun();
        output = runController_.hasActiveRun()
            ? localization_.get(TextId("debug.run_saved"))
            : localization_.get(TextId("debug.no_active_run_to_save"));
        return true;
    }

    if (command == "clear" && tokens.size() >= 2u && tokens[1] == "pending") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }
        runController_.clearPendingRoom();
        saveActiveRun();
        output = localization_.get(TextId("debug.pending_room_cleared"));
        return true;
    }

    if (command == "unlock" && tokens.size() >= 2u && tokens[1] == "map") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }
        for (RunMapNode& node : runController_.run().map.nodes) {
            if (node.state == RunMapNodeState::Locked) {
                node.state = RunMapNodeState::Available;
            }
        }
        saveActiveRun();
        output = localization_.get(TextId("debug.map_unlocked"));
        return true;
    }

    if (command == "run" && tokens.size() >= 2u && tokens[1] == "state") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }

        const RunState& run = runController_.run();
        std::string currentNode = "-";
        for (const RunMapNode& node : run.map.nodes) {
            if (node.state == RunMapNodeState::Current) {
                currentNode = std::to_string(node.id);
                break;
            }
        }

        output = localization_.format(
            TextId("debug.run_state"),
            {
                {"phase", debugRunPhaseName(run.phase)},
                {"floor", run.currentFloorId},
                {"node", currentNode},
                {"pending", debugPendingRoomName(run.pendingRoom.type)}
            }
        );
        return true;
    }

    if (command == "goto" && tokens.size() >= 2u && tokens[1] == "boss") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }

        RunState& run = runController_.run();
        bool foundBoss = false;
        for (RunMapNode& node : run.map.nodes) {
            if (node.type == RunMapNodeType::Boss) {
                node.state = RunMapNodeState::Available;
                foundBoss = true;
            } else {
                node.state = RunMapNodeState::Completed;
            }
        }

        if (!foundBoss) {
            output = localization_.get(TextId("debug.no_boss_node"));
            return true;
        }

        run.pendingRoom.clear();
        run.actCompleted = false;
        run.completedAct = 0;
        run.completionType = RunCompletionType::InProgress;
        run.phase = RunPhase::Map;
        saveActiveRun();
        queueTransition([this]() { setRunMapScene(); });
        output = localization_.get(TextId("debug.goto_boss_done"));
        return true;
    }

    if (command == "finish" && tokens.size() >= 2u && tokens[1] == "floor") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }

        runController_.clearPendingRoom();
        runController_.completeCurrentAct();
        saveActiveRun();
        queueTransition([this]() { setFloorCompleteScene(); });
        output = localization_.get(TextId("debug.floor_finished"));
        return true;
    }

    if (command == "finish" && tokens.size() >= 2u && tokens[1] == "run") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }
        if (tokens.size() < 3u || (tokens[2] != "victory" && tokens[2] != "defeat")) {
            output = localization_.get(TextId("debug.usage.finish_run"));
            return true;
        }

        RunState completedRun = runController_.run();
        const bool victory = tokens[2] == "victory";
        const RunEndReason reason = victory
            ? completedRunEndReason(completedRun)
            : defeatedRunEndReason(completedRun);
        queueTransition([this, completedRun = std::move(completedRun), reason]() mutable {
            finishRun(reason);
            setRunCompleteScene(std::move(completedRun), reason);
        });
        output = localization_.get(TextId(victory ? "debug.run_finished_victory" : "debug.run_finished_defeat"));
        return true;
    }

    if ((command == "give" || command == "add") && tokens.size() >= 3u) {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }

        RunState& run = runController_.run();
        const std::string& type = tokens[1];
        const std::string& idOrAmount = tokens[2];

        if (type == "gold" || type == "money") {
            const int amount = parseIntOr(tokens, 2u, 0);
            run.gold = std::max(0, run.gold + amount);
            if (amount > 0) {
                run.stats.goldGained += amount;
            }
            saveActiveRun();
            output = localization_.format(
                TextId("debug.gold_adjusted"),
                {{"amount", std::to_string(amount)}, {"gold", std::to_string(run.gold)}}
            );
            return true;
        }

        if (type == "card") {
            const CardId cardId(idOrAmount);
            if (!content_.cards().contains(cardId)) {
                output = localization_.format(TextId("debug.unknown_card_id"), {{"id", idOrAmount}});
                return true;
            }
            const int count = std::max(1, parseIntOr(tokens, 3u, 1));
            for (int i = 0; i < count; ++i) {
                run.deckCardIds.push_back(cardId);
                ++run.stats.cardsAdded;
            }
            profileProgression_.unlockCardAndToast(cardId.value);
            saveActiveRun();
            output = localization_.format(
                TextId("debug.card_added"),
                {{"id", idOrAmount}, {"count", std::to_string(count)}}
            );
            return true;
        }

        if (type == "relic") {
            const RelicId relicId(idOrAmount);
            if (!content_.relics().contains(relicId)) {
                output = localization_.format(TextId("debug.unknown_relic_id"), {{"id", idOrAmount}});
                return true;
            }
            if (std::find(run.relicIds.begin(), run.relicIds.end(), idOrAmount) == run.relicIds.end()) {
                run.relicIds.push_back(idOrAmount);
            }
            profileProgression_.unlockRelicAndToast(idOrAmount);
            saveActiveRun();
            output = localization_.format(TextId("debug.relic_added"), {{"id", idOrAmount}});
            return true;
        }

        if (type == "active" || type == "active_item") {
            const ActiveItemId activeItemId(idOrAmount);
            if (!content_.activeItems().contains(activeItemId)) {
                output = localization_.format(TextId("debug.unknown_active_item_id"), {{"id", idOrAmount}});
                return true;
            }
            const ActiveItemDefinition& definition = content_.activeItems().get(activeItemId);
            const int initialCharge = std::max(0, parseIntOr(tokens, 3u, 0));
            ActiveItemSystem::equip(run, definition, initialCharge);
            saveActiveRun();
            output = localization_.format(TextId("debug.active_item_added"), {{"id", idOrAmount}});
            return true;
        }

        if (type == "consumable" || type == "potion") {
            const ConsumableId consumableId(idOrAmount);
            if (!content_.consumables().contains(consumableId)) {
                output = localization_.format(TextId("debug.unknown_consumable_id"), {{"id", idOrAmount}});
                return true;
            }
            if (static_cast<int>(run.consumableIds.size()) >= run.maxConsumables) {
                output = localization_.get(TextId("debug.consumable_slots_full"));
                return true;
            }
            run.consumableIds.push_back(idOrAmount);
            profileProgression_.discoverConsumableAndToast(idOrAmount);
            saveActiveRun();
            output = localization_.format(TextId("debug.consumable_added"), {{"id", idOrAmount}});
            return true;
        }
    }

    if (command == "charge" && tokens.size() >= 2u && (tokens[1] == "active" || tokens[1] == "active_item")) {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }
        RunState& run = runController_.run();
        if (run.activeItem.empty() || !content_.activeItems().contains(ActiveItemId(run.activeItem.itemId))) {
            output = localization_.get(TextId("debug.no_active_item"));
            return true;
        }
        const ActiveItemDefinition& definition = content_.activeItems().get(ActiveItemId(run.activeItem.itemId));
        const int amount = parseIntOr(tokens, 2u, 0);
        run.activeItem.charge = std::clamp(run.activeItem.charge + amount, 0, definition.maxCharge);
        saveActiveRun();
        output = localization_.format(TextId("debug.active_item_charged"), {{"charge", std::to_string(run.activeItem.charge)}});
        return true;
    }

    if (command == "heal" || command == "damage") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }
        const int amount = parseIntOr(tokens, 1u, 0);
        if (amount <= 0) {
            output = localization_.format(TextId("debug.usage.run_heal_damage"), {{"command", command}});
            return true;
        }
        for (RunActorState& actor : runController_.run().actorStates) {
            if (command == "heal") {
                actor.currentHp = std::min(actor.maxHp, actor.currentHp + amount);
            } else {
                actor.currentHp = std::max(0, actor.currentHp - amount);
            }
        }
        saveActiveRun();
        output = command == "heal"
            ? localization_.get(TextId("debug.run_actors_healed"))
            : localization_.get(TextId("debug.run_actors_damaged"));
        return true;
    }

    if (command == "stress") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }
        if (tokens.size() < 2u) {
            output = localization_.get(TextId("debug.usage.run_stress"));
            return true;
        }
        const int delta = parseIntOr(tokens, 1u, 0);
        runController_.adjustAllActorsStress(delta, &runController_.random());
        saveActiveRun();
        output = localization_.format(TextId("debug.run_actor_stress_adjusted"), {{"amount", std::to_string(delta)}});
        return true;
    }

    if (command == "fullheal") {
        if (!runController_.hasActiveRun()) {
            output = localization_.get(TextId("debug.no_active_run"));
            return true;
        }
        for (RunActorState& actor : runController_.run().actorStates) {
            actor.currentHp = std::max(1, actor.maxHp);
            actor.stress = 0;
            actor.resolveCheckTriggered = false;
        }
        saveActiveRun();
        output = localization_.get(TextId("debug.full_heal_done"));
        return true;
    }

    return false;
}

void GameFlowController::addDebugMessage(std::string message) {
    constexpr std::size_t maxMessages = 80u;
    debugMessages_.push_back(std::move(message));
    if (debugMessages_.size() > maxMessages) {
        debugMessages_.erase(debugMessages_.begin(), debugMessages_.begin() + static_cast<std::ptrdiff_t>(debugMessages_.size() - maxMessages));
    }
}

void GameFlowController::setSaveSlotScene() {
    sceneManager_.setScene(
        std::make_unique<SaveSlotScene>(
            uiFont_,
            localization_,
            profileManager_,
            [this](const std::size_t slotIndex) { return hasRunSave(slotIndex); },
            [this](const std::size_t slotIndex) { return runSaveSummaryText(slotIndex); },
            [this](const std::size_t slotIndex) { startNewRunInSlot(slotIndex); },
            [this](const std::size_t slotIndex) { continueRunInSlot(slotIndex); },
            [this](const std::size_t slotIndex) { deleteRunInSlot(slotIndex); },
            [this]() { showMainMenu(); },
            saveSlotStatusMessage_
        )
    );
}

std::string GameFlowController::runSaveSummaryText(const std::size_t slotIndex) const {
    const RunSaveLoadResult loadResult = runSaveSystem_.tryLoadRun(slotIndex);
    if (!loadResult.loaded) {
        return localization_.get(TextId("save_slot.run_save_preview_failed"));
    }

    const RunState& run = loadResult.run;

    std::string archetypeName = run.archetypeId.value.empty() ? "?" : run.archetypeId.value;
    if (content_.archetypes().contains(run.archetypeId)) {
        archetypeName = localization_.get(content_.archetypes().get(run.archetypeId).nameTextId);
    }

    std::string difficultyName = run.difficultyId.value.empty() ? "?" : run.difficultyId.value;
    if (content_.difficulties().contains(run.difficultyId)) {
        difficultyName = localization_.get(content_.difficulties().get(run.difficultyId).nameTextId);
    }

    std::string floorName = run.currentFloorId.empty() ? "?" : run.currentFloorId;
    if (content_.floors().contains(run.currentFloorId)) {
        floorName = localization_.get(TextId(content_.floors().get(run.currentFloorId).nameTextId));
    }

    std::string nextFloorName;
    const RunCompletionStatus completion = RunCompletion::evaluate(run, content_.floors());
    if (completion.canContinueToNextFloor) {
        nextFloorName = completion.nextFloorId;
        if (content_.floors().contains(completion.nextFloorId)) {
            nextFloorName = localization_.get(TextId(content_.floors().get(completion.nextFloorId).nameTextId));
        }
    }

    if (!run.challengeId.empty()) {
        std::string challengeName = run.challengeId;
        if (content_.challenges().contains(run.challengeId)) {
            challengeName = localization_.get(content_.challenges().get(run.challengeId).nameTextId);
        }

        if (completion.canContinueToNextFloor) {
            return localization_.format(
                TextId("save_slot.run_summary.challenge_floor_clear"),
                {
                    {"challenge", challengeName},
                    {"archetype", archetypeName},
                    {"floor", floorName},
                    {"next_floor", nextFloorName},
                    {"difficulty", difficultyName},
                    {"gold", std::to_string(run.gold)}
                }
            );
        }

        return localization_.format(
            TextId("save_slot.run_summary.challenge"),
            {
                {"challenge", challengeName},
                {"archetype", archetypeName},
                {"floor", floorName},
                {"difficulty", difficultyName},
                {"gold", std::to_string(run.gold)}
            }
        );
    }

    if (completion.canContinueToNextFloor) {
        return localization_.format(
            TextId("save_slot.run_summary.normal_floor_clear"),
            {
                {"archetype", archetypeName},
                {"floor", floorName},
                {"next_floor", nextFloorName},
                {"difficulty", difficultyName},
                {"gold", std::to_string(run.gold)}
            }
        );
    }

    return localization_.format(
        TextId("save_slot.run_summary.normal"),
        {
            {"archetype", archetypeName},
            {"floor", floorName},
            {"difficulty", difficultyName},
            {"gold", std::to_string(run.gold)}
        }
    );
}

void GameFlowController::setProfileHubScene() {
    const std::size_t slotIndex = profileManager_.selectedSlotIndex();
    const bool hasSavedRun = hasRunSave(slotIndex);

    sceneManager_.setScene(
        std::make_unique<ProfileHubScene>(
            uiFont_,
            localization_,
            content_.actors(),
            content_.cards(),
            content_.relics(),
            content_.archetypes().all(),
            profileManager_.selectedProfile() != nullptr ? profileManager_.selectedProfile()->unlockedArchetypeIds : std::vector<std::string>{},
            hasSavedRun,
            [this](PlayableArchetypeId archetypeId) { selectArchetype(std::move(archetypeId)); },
            [this]() { continueRunInSlot(profileManager_.selectedSlotIndex()); },
            [this]() { queueTransition([this]() { setChallengeScene(); }); },
            [this]() { queueTransition([this]() { setAchievementScene(); }); },
            [this]() { queueTransition([this]() { setCompendiumScene(); }); },
            [this]() { queueTransition([this]() { setProfileProgressScene(); }); },
            [this]() { showSaveSlots(); }
        )
    );
}

void GameFlowController::setChallengeScene() {
    sceneManager_.setScene(
        std::make_unique<ChallengeScene>(
            uiFont_,
            localization_,
            content_,
            content_.challenges().all(),
            profileManager_.selectedProfile(),
            hasRunSave(profileManager_.selectedSlotIndex()),
            [this](std::string challengeId) { startChallengeRun(std::move(challengeId)); },
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::startChallengeRun(const std::string& challengeId) {
    queueTransition([this, challengeId]() {
        const ProfileData* profile = profileManager_.selectedProfile();
        if (profile == nullptr || !content_.challenges().contains(challengeId)) {
            setChallengeScene();
            return;
        }

        const ChallengeDefinition& challenge = content_.challenges().get(challengeId);
        if (!ChallengeRules::isUnlocked(challenge, *profile)) {
            setChallengeScene();
            return;
        }

        const std::string archetypeId = challenge.startingArchetypeId.empty()
            ? std::string("rusted_knight")
            : challenge.startingArchetypeId;
        const std::string difficultyId = challenge.startingDifficultyId.empty()
            ? std::string("normal")
            : challenge.startingDifficultyId;
        const std::string floorId = challenge.startingFloorId.empty()
            ? content_.floors().startingFloor().id
            : challenge.startingFloorId;

        if (!content_.archetypes().contains(PlayableArchetypeId(archetypeId)) ||
            !content_.difficulties().contains(DifficultyId(difficultyId)) ||
            !content_.floors().contains(floorId) ||
            !profileManager_.isSelectedArchetypeUnlocked(archetypeId)) {
            setChallengeScene();
            return;
        }

        const PlayableArchetypeDefinition& archetype = content_.archetypes().get(PlayableArchetypeId(archetypeId));
        const DifficultyDefinition& difficulty = content_.difficulties().get(DifficultyId(difficultyId));
        const FloorDefinition& startingFloor = content_.floors().get(floorId);

        const std::uint32_t runSeed =
            (static_cast<std::uint32_t>(random_.rangeInclusive(0, 65535)) << 16) |
            static_cast<std::uint32_t>(random_.rangeInclusive(0, 65535));

        runController_.clearActiveRun();
        selectedArchetypeId_.reset();
        selectedDifficultyId_.reset();
        runController_.startNewRun(
            archetype,
            difficulty,
            content_.actors(),
            content_.mapGenerationForFloor(startingFloor.id),
            startingFloor,
            runSeed
        );

        runController_.run().challengeId = challenge.id;
        applyChallengeLoadout(challenge);
        profileProgression_.synchronizeRunContent(runController_.run());
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::applyChallengeLoadout(const ChallengeDefinition& challenge) {
    if (!runController_.hasActiveRun()) {
        return;
    }

    RunState& run = runController_.run();

    if (challenge.startingGoldOverride >= 0) {
        run.gold = challenge.startingGoldOverride;
    }

    if (!challenge.fixedStartingDeckCardIds.empty()) {
        run.deckCardIds.clear();
        run.upgradedDeckIndices.clear();
        run.deckCardIds.reserve(challenge.fixedStartingDeckCardIds.size());
        for (const std::string& cardId : challenge.fixedStartingDeckCardIds) {
            if (content_.cards().contains(CardId(cardId))) {
                run.deckCardIds.emplace_back(cardId);
            }
        }
    }

    if (!challenge.fixedStartingRelicIds.empty()) {
        run.relicIds.clear();
        for (RunActorState& actor : run.actorStates) {
            actor.relicIds.clear();
        }

        for (const std::string& relicId : challenge.fixedStartingRelicIds) {
            if (content_.relics().contains(RelicId(relicId))) {
                (void)RunRelicOwnership::assignRelicToActor(run, relicId, RunRelicOwnership::defaultActorDefinitionId(run));
            }
        }
    }

    if (!challenge.fixedStartingConsumableIds.empty()) {
        run.consumableIds.clear();
        for (const std::string& consumableId : challenge.fixedStartingConsumableIds) {
            if (content_.consumables().contains(ConsumableId(consumableId))) {
                run.consumableIds.push_back(consumableId);
            }
        }
    }
}

void GameFlowController::setAchievementScene() {
    sceneManager_.setScene(
        std::make_unique<AchievementScene>(
            uiFont_,
            localization_,
            content_.achievements().all(),
            profileManager_.selectedProfile(),
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}


std::string GameFlowController::challengeRunLabel(const RunState& run) const {
    if (run.challengeId.empty()) {
        return {};
    }

    std::string challengeName = run.challengeId;
    if (content_.challenges().contains(run.challengeId)) {
        const ChallengeDefinition& challenge = content_.challenges().get(run.challengeId);
        challengeName = localization_.get(challenge.nameTextId);
    }

    return localization_.format(TextId("run.challenge_banner"), {{"name", challengeName}});
}

std::string GameFlowController::challengeRunGoalLabel(const RunState& run) const {
    if (run.challengeId.empty() || !content_.challenges().contains(run.challengeId)) {
        return {};
    }

    const ChallengeDefinition& challenge = content_.challenges().get(run.challengeId);
    return localization_.format(
        TextId("run.challenge_goal_banner"),
        {{"goal", localization_.get(challenge.goalTextId)}}
    );
}

std::string GameFlowController::challengeRunProgressLabel(const RunState& run) const {
    if (run.challengeId.empty() || !content_.challenges().contains(run.challengeId)) {
        return {};
    }

    const ChallengeDefinition& challenge = content_.challenges().get(run.challengeId);
    const ChallengeCompletionCondition& completion = challenge.completion;
    std::vector<std::string> parts;

    if (completion.minBossesKilled > 0) {
        parts.push_back(localization_.format(
            TextId("run.challenge_progress.bosses"),
            {{"current", std::to_string(std::max(0, run.stats.bossesKilled))}, {"target", std::to_string(completion.minBossesKilled)}}
        ));
    }

    if (completion.minElitesKilled > 0) {
        parts.push_back(localization_.format(
            TextId("run.challenge_progress.elites"),
            {{"current", std::to_string(std::max(0, run.stats.elitesKilled))}, {"target", std::to_string(completion.minElitesKilled)}}
        ));
    }

    if (completion.maxShopsVisited >= 0) {
        parts.push_back(localization_.format(
            TextId("run.challenge_progress.shops"),
            {{"current", std::to_string(std::max(0, run.stats.shopsVisited))}, {"maximum", std::to_string(completion.maxShopsVisited)}}
        ));
    }

    if (completion.maxDamageTaken >= 0) {
        parts.push_back(localization_.format(
            TextId("run.challenge_progress.damage"),
            {{"current", std::to_string(std::max(0, run.stats.damageTaken))}, {"maximum", std::to_string(completion.maxDamageTaken)}}
        ));
    }

    if (parts.empty()) {
        return {};
    }

    return localization_.format(
        TextId("run.challenge_progress_banner"),
        {{"progress", joinChallengeProgressParts(parts)}}
    );
}

void GameFlowController::setCompendiumScene() {
    sceneManager_.setScene(
        std::make_unique<CompendiumScene>(
            uiFont_,
            localization_,
            content_.cards().all(),
            content_.relics().all(),
            content_.enemies().all(),
            content_.statuses().all(),
            content_.consumables().all(),
            profileManager_.selectedProfile(),
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::setProfileProgressScene() {
    sceneManager_.setScene(
        std::make_unique<ProfileProgressScene>(
            uiFont_,
            localization_,
            content_,
            profileManager_.selectedProfile(),
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::setDifficultySelectScene() {
    sceneManager_.setScene(
        std::make_unique<DifficultySelectScene>(
            uiFont_,
            localization_,
            content_.difficulties().all(),
            hasRunSave(profileManager_.selectedSlotIndex()),
            [this](DifficultyId difficultyId) { selectDifficulty(std::move(difficultyId)); },
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::setRunMapScene() {
    if (!runController_.hasActiveRun()) {
        throw std::runtime_error("Cannot open run map: no active run");
    }

    if (runController_.isActCompleted()) {
        runController_.run().phase = RunPhase::FloorComplete;
        setFloorCompleteScene();
        return;
    }

    if (runController_.partyDefeated()) {
        runController_.run().phase = RunPhase::RunComplete;
        setRunDefeatScene();
        return;
    }

    if (runController_.run().phase != RunPhase::Rest) {
        runController_.run().phase = RunPhase::Map;
    }

    sceneManager_.setScene(
        std::make_unique<RunMapScene>(
            uiFont_,
            localization_,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            runController_.run(),
            challengeRunLabel(runController_.run()),
            [this](const int nodeId) { startMapNode(nodeId); },
            [this](const int nodeId) { restHeal(nodeId); },
            [this](const int nodeId) { restCalm(nodeId); },
            [this](const int nodeId, const std::size_t deckIndex) { restUpgrade(nodeId, deckIndex); },
            [this](const int nodeId) { restSkip(nodeId); },
            [this]() { saveAndExitRunToProfileHub(); },
            [this]() { abandonActiveRun(); }
        )
    );
}

void GameFlowController::setRunCompleteScene(RunState completedRun, const RunEndReason reason) {
    sceneManager_.setScene(
        std::make_unique<RunCompleteScene>(
            uiFont_,
            localization_,
            content_,
            std::move(completedRun),
            reason,
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

void GameFlowController::setFloorCompleteScene() {
    if (!runController_.hasActiveRun()) {
        throw std::runtime_error("Cannot open floor completion scene: no active run");
    }

    RunPacingTracker::finishFloor(runController_.run());
    runController_.run().phase = RunPhase::FloorComplete;
    const RunCompletionStatus completion = RunCompletion::evaluate(runController_.run(), content_.floors());
    const std::string nextFloorName = completion.nextFloorId.empty()
        ? std::string{}
        : (content_.floors().contains(completion.nextFloorId)
            ? localization_.get(TextId(content_.floors().get(completion.nextFloorId).nameTextId))
            : completion.nextFloorId);

    runController_.run().completionType = completion.type;

    sceneManager_.setScene(
        std::make_unique<FloorCompleteScene>(
            uiFont_,
            localization_,
            content_.enemies(),
            content_.relics(),
            runController_.run(),
            nextFloorName,
            challengeRunLabel(runController_.run()),
            completion.type,
            completion.canContinueToNextFloor,
            [this]() { finishFloorCompleteContinue(); },
            [this]() { finishFloorCompleteMainMenu(); }
        )
    );
}


void GameFlowController::setRunDefeatScene() {
    if (!runController_.hasActiveRun()) {
        throw std::runtime_error("Cannot open run defeat scene: no active run");
    }

    const RunState defeatedRun = runController_.run();
    finishDefeatedRunAndDeleteSave();

    sceneManager_.setScene(
        std::make_unique<RunDefeatScene>(
            uiFont_,
            localization_,
            content_.relics(),
            defeatedRun,
            [this]() { finishRunDefeatToProfileHub(); },
            [this]() { finishRunDefeatToMainMenu(); }
        )
    );
}

bool GameFlowController::setPendingRoomSceneIfNeeded() {
    if (!runController_.hasPendingRoom()) {
        return false;
    }

    if (!runController_.pendingRoomMatchesRunMap()) {
        std::cout << "Discarding stale pending room state for node " // NOL10N: developer diagnostic
                  << runController_.pendingRoom().nodeId
                  << ". Returning to the run map.\n"; // NOL10N: developer diagnostic
        runController_.clearPendingRoom();
        saveActiveRun();
        setRunMapScene();
        return true;
    }

    const RunPendingRoomState& pending = runController_.pendingRoom();
    switch (pending.type) {
        case RunPendingRoomType::CombatReward:
            sceneManager_.setScene(
                std::make_unique<RewardScene>(
                    uiFont_,
                    localization_,
                    content_.cards(),
                    content_.relics(),
                    content_.consumables(),
                    content_.activeItems(),
                    content_.actors(),
                    runController_.run(),
                    pending.reward,
                    [this](RewardState& reward, const RewardSelection& selection) {
                        return rerollRewardWithActiveItem(reward, selection);
                    },
                    [this](const CardId& cardId) {
                        return copyCardWithActiveItem(cardId, ActiveItemUseContext::Reward);
                    },
                    [this](RewardSelection selection) {
                        finishReward(runController_.pendingRoom().reward, std::move(selection));
                    }
                )
            );
            return true;

        case RunPendingRoomType::ChestReward:
            setChestRewardScene(pending.nodeId, pending.reward);
            return true;

        case RunPendingRoomType::Shop:
            setShopScene(pending.nodeId, pending.shop);
            return true;

        case RunPendingRoomType::MerchantRest:
            setMerchantRestScene(pending.nodeId, pending.shop);
            return true;

        case RunPendingRoomType::Event:
            if (!content_.events().contains(pending.eventId)) {
                std::cout << "Cannot restore pending event room: unknown event id '" // NOL10N: developer diagnostic
                          << pending.eventId
                          << "'. Returning to the run map.\n"; // NOL10N: developer diagnostic
                runController_.clearPendingRoom();
                saveActiveRun();
                setRunMapScene();
                return true;
            }
            setEventScene(pending.nodeId, content_.events().get(pending.eventId));
            return true;

        case RunPendingRoomType::None:
            return false;
    }

    return false;
}

bool GameFlowController::setPendingRoomSceneForNodeIfNeeded(const int nodeId) {
    if (!runController_.hasPendingRoomForNode(nodeId)) {
        return false;
    }

    return setPendingRoomSceneIfNeeded();
}

void GameFlowController::setCombatScene(const int nodeId) {
    if (!runController_.hasActiveRun()) {
        throw std::runtime_error("Cannot start combat: no active run");
    }

    sceneManager_.setScene(
        std::make_unique<CombatScene>(
            content_,
            localization_,
            runController_.random(),
            uiFont_,
            runController_.run(),
            [this](std::string message) {
                saveActiveRun();
                profileProgression_.pushToast(std::move(message));
            },
            [this](const std::string& hintId) {
                return profileManager_.isSelectedOnboardingHintSeen(hintId);
            },
            [this](std::string hintId) {
                profileManager_.markSelectedOnboardingHintSeen(hintId);
            },
            [this, nodeId](const CombatResult& result) {
                profileProgression_.discoverCombatContent(result);
                profileProgression_.recordCombatStats(result);

                RewardState reward = runController_.completeCombatAndCreateReward(
                    nodeId,
                    result,
                    content_.cards(),
                    content_.relics(),
                    content_.consumables(),
                    content_.activeItems(),
                    content_.rewardTuning(),
                    runController_.random()
                );

                runController_.setPendingCombatReward(nodeId, reward);
                saveActiveRun();
                queueTransition([this, nodeId, reward = std::move(reward)]() mutable {
                    setCombatRewardScene(nodeId, std::move(reward));
                });
            },
            [this](const CombatResult& result) {
                queueTransition([this, result]() {
                    if (!runController_.hasActiveRun()) {
                        setProfileHubScene();
                        return;
                    }

                    profileProgression_.discoverCombatContent(result);
                    profileProgression_.recordCombatStats(result);
                    runController_.recordCombatDefeat(result);
                    setRunDefeatScene();
                });
            }
        )
    );
}


void GameFlowController::setCombatRewardScene(const int nodeId, RewardState reward) {
    sceneManager_.setScene(
        std::make_unique<RewardScene>(
            uiFont_,
            localization_,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            content_.activeItems(),
            content_.actors(),
            runController_.run(),
            std::move(reward),
            [this](RewardState& currentReward, const RewardSelection& selection) {
                return rerollRewardWithActiveItem(currentReward, selection);
            },
            [this](const CardId& cardId) {
                return copyCardWithActiveItem(cardId, ActiveItemUseContext::Reward);
            },
            [this, nodeId](RewardSelection selection) {
                if (!runController_.hasPendingRoom() ||
                    runController_.pendingRoom().type != RunPendingRoomType::CombatReward ||
                    runController_.pendingRoom().nodeId != nodeId) {
                    queueTransition([this]() { setRunMapScene(); });
                    return;
                }

                finishReward(runController_.pendingRoom().reward, std::move(selection));
            }
        )
    );
}

void GameFlowController::setChestRewardScene(const int nodeId, RewardState reward) {
    sceneManager_.setScene(
        std::make_unique<RewardScene>(
            uiFont_,
            localization_,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            content_.activeItems(),
            content_.actors(),
            runController_.run(),
            std::move(reward),
            [this](RewardState& currentReward, const RewardSelection& selection) {
                return rerollRewardWithActiveItem(currentReward, selection);
            },
            [this](const CardId& cardId) {
                return copyCardWithActiveItem(cardId, ActiveItemUseContext::Reward);
            },
            [this, nodeId](RewardSelection selection) {
                finishChestReward(nodeId, std::move(selection));
            }
        )
    );
}


void GameFlowController::setShopScene(const int nodeId, ShopState shopState) {
    sceneManager_.setScene(
        std::make_unique<ShopScene>(
            uiFont_,
            localization_,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            content_.activeItems(),
            content_.actors(),
            runController_.run(),
            std::move(shopState),
            [this](const ShopPurchase& purchase) { return purchaseShopItem(purchase); },
            [this, nodeId](const ShopState& changedShopState) { updatePendingShopState(nodeId, changedShopState); },
            [this](ShopState& currentShop) { return rerollShopWithActiveItem(currentShop); },
            [this](const CardId& cardId) { return copyCardWithActiveItem(cardId, ActiveItemUseContext::Shop); },
            [this, nodeId]() { finishShop(nodeId); }
        )
    );
}

void GameFlowController::setMerchantRestScene(const int nodeId, ShopState shopState) {
    shopState.mode = ShopStateMode::MerchantRest;

    if (shopState.merchantRestCardShopOpen) {
        sceneManager_.setScene(
            std::make_unique<ShopScene>(
                uiFont_,
                localization_,
                content_.cards(),
                content_.relics(),
                content_.consumables(),
                content_.activeItems(),
                content_.actors(),
                runController_.run(),
                std::move(shopState),
                [this](const ShopPurchase& purchase) { return purchaseShopItem(purchase); },
                [this, nodeId](const ShopState& changedShopState) { updatePendingShopState(nodeId, changedShopState); },
                std::function<bool(ShopState&)>{},
                std::function<bool(const CardId&)>{},
                [this, nodeId]() { finishShop(nodeId); }
            )
        );
        return;
    }

    sceneManager_.setScene(
        std::make_unique<MerchantRestScene>(
            uiFont_,
            localization_,
            content_.cards(),
            runController_.run(),
            std::move(shopState),
            [this, nodeId](ShopState changedShopState) {
                changedShopState.mode = ShopStateMode::MerchantRest;
                changedShopState.merchantRestCardShopOpen = true;
                updatePendingShopState(nodeId, changedShopState);
                queueTransition([this, nodeId, changedShopState = std::move(changedShopState)]() mutable {
                    setMerchantRestScene(nodeId, std::move(changedShopState));
                });
            },
            [this, nodeId]() { restHeal(nodeId); },
            [this, nodeId]() { restCalm(nodeId); },
            [this, nodeId](const std::size_t deckIndex) { restUpgrade(nodeId, deckIndex); },
            [this, nodeId]() { restSkip(nodeId); }
        )
    );
}

void GameFlowController::setEventScene(const int nodeId, const RunEventDefinition& event) {
    sceneManager_.setScene(
        std::make_unique<EventScene>(
            uiFont_,
            localization_,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            runController_.run(),
            event,
            [this, nodeId](const RunEventChoiceDefinition& choice) { finishEvent(nodeId, choice); }
        )
    );
}

void GameFlowController::setEventOutcomeScene(RunEventChoiceResult result) {
    sceneManager_.setScene(
        std::make_unique<EventOutcomeScene>(
            uiFont_,
            localization_,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            std::move(result),
            [this]() {
                saveActiveRun();
                setRunMapScene();
            }
        )
    );
}

void GameFlowController::showMainMenu() {
    queueTransition([this]() { setMainMenuScene(); });
}

void GameFlowController::showSettings() {
    queueTransition([this]() { openSettingsOverlay(); });
}

void GameFlowController::showSaveSlots() {
    queueTransition([this]() { setSaveSlotScene(); });
}

void GameFlowController::startNewRunInSlot(const std::size_t slotIndex) {
    queueTransition([this, slotIndex]() {
        saveSlotStatusMessage_.clear();
        profileManager_.selectSlot(slotIndex);
        runController_.clearActiveRun();
        selectedArchetypeId_.reset();
        selectedDifficultyId_.reset();
        setProfileHubScene();
    });
}

void GameFlowController::continueRunInSlot(const std::size_t slotIndex) {
    queueTransition([this, slotIndex]() {
        profileManager_.selectSlot(slotIndex);

        const RunSaveLoadResult loadResult = runSaveSystem_.tryLoadRun(slotIndex);
        if (!loadResult.loaded) {
            runController_.clearActiveRun();
            selectedArchetypeId_.reset();
            selectedDifficultyId_.reset();
            saveSlotStatusMessage_ = localization_.format(
                TextId("save_slot.load_failed"),
                {{"slot", std::to_string(slotIndex + 1)}}
            );
            std::cout << "Failed to load run save from slot " << (slotIndex + 1) // NOL10N: developer diagnostic
                      << ": " << loadResult.errorMessage << '\n';
            setSaveSlotScene();
            return;
        }

        saveSlotStatusMessage_.clear();
        runController_.restoreRun(loadResult.run);
        profileProgression_.synchronizeRunContent(runController_.run());
        selectedArchetypeId_.reset();
        selectedDifficultyId_.reset();

        if (loadResult.loadedFromBackup) {
            try {
                runSaveSystem_.restoreBackupAsPrimary(slotIndex);
                std::cout << "Restored run save for slot " << (slotIndex + 1) // NOL10N: developer diagnostic
                          << " from backup.\n"; // NOL10N: developer diagnostic
            } catch (const std::exception& error) {
                std::cout << "Loaded run save for slot " << (slotIndex + 1) // NOL10N: developer diagnostic
                          << " from backup, but failed to restore the primary save: " // NOL10N: developer diagnostic
                          << error.what() << '\n';
            }
        }

        if (runController_.partyDefeated()) {
            setRunDefeatScene();
            return;
        }

        switch (runController_.run().phase) {
            case RunPhase::FloorComplete:
                runController_.clearPendingRoom();
                runController_.run().phase = RunPhase::FloorComplete;
                saveActiveRun();
                setFloorCompleteScene();
                return;

            case RunPhase::Combat: {
                const int nodeId = runController_.run().map.currentNodeId;
                const RunMapNode& node = runController_.node(nodeId);
                if (node.state == RunMapNodeState::Current &&
                    (node.type == RunMapNodeType::Combat || node.type == RunMapNodeType::Elite || node.type == RunMapNodeType::Boss)) {
                    setCombatScene(nodeId);
                    return;
                }
                runController_.run().phase = RunPhase::Map;
                saveActiveRun();
                setRunMapScene();
                return;
            }

            case RunPhase::Reward:
            case RunPhase::Event:
            case RunPhase::Shop:
            case RunPhase::Rest:
            case RunPhase::Chest:
                if (setPendingRoomSceneIfNeeded()) {
                    return;
                }
                setRunMapScene();
                return;

            case RunPhase::RunComplete:
                deleteSelectedRunSave();
                runController_.clearActiveRun();
                setProfileHubScene();
                return;

            case RunPhase::Map:
                setRunMapScene();
                return;
        }
    });
}

void GameFlowController::deleteRunInSlot(const std::size_t slotIndex) {
    queueTransition([this, slotIndex]() {
        saveSlotStatusMessage_.clear();
        const bool deletingSelectedSlot = profileManager_.selectedProfile() != nullptr &&
            profileManager_.selectedSlotIndex() == slotIndex;

        runSaveSystem_.deleteRun(slotIndex);
        profileManager_.deleteProfile(slotIndex);
        if (deletingSelectedSlot && runController_.hasActiveRun()) {
            runController_.clearActiveRun();
        }
        setSaveSlotScene();
    });
}

void GameFlowController::selectArchetype(PlayableArchetypeId archetypeId) {
    queueTransition([this, archetypeId = std::move(archetypeId)]() mutable {
        const PlayableArchetypeDefinition& archetype = content_.archetypes().get(archetypeId);
        if (!archetype.isAvailable || !profileManager_.isSelectedArchetypeUnlocked(archetype.id.value)) {
            selectedArchetypeId_.reset();
            setProfileHubScene();
            return;
        }

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

        const std::uint32_t runSeed =
            (static_cast<std::uint32_t>(random_.rangeInclusive(0, 65535)) << 16) |
            static_cast<std::uint32_t>(random_.rangeInclusive(0, 65535));

        const FloorDefinition& startingFloor = content_.floors().startingFloor();
        runController_.startNewRun(
            archetype,
            difficulty,
            content_.actors(),
            content_.mapGenerationForFloor(startingFloor.id),
            startingFloor,
            runSeed
        );
        profileProgression_.synchronizeRunContent(runController_.run());
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::startMapNode(const int nodeId) {
    queueTransition([this, nodeId]() {
        if (setPendingRoomSceneForNodeIfNeeded(nodeId)) {
            return;
        }

        if (runController_.hasPendingRoom()) {
            setPendingRoomSceneIfNeeded();
            return;
        }

        if (!runController_.canStartNode(nodeId)) {
            setRunMapScene();
            return;
        }

        const RunMapNodeType nodeType = runController_.node(nodeId).type;
        runController_.startNode(nodeId);
        saveActiveRun();

        switch (nodeType) {
            case RunMapNodeType::Combat:
            case RunMapNodeType::Elite:
            case RunMapNodeType::Boss:
                setCombatScene(nodeId);
                return;

            case RunMapNodeType::Chest: {
                RewardState reward;
                reward.sourceNodeType = RunMapNodeType::Chest;

                const NodeRewardTuning& chestTuning = content_.rewardTuning().node(RunMapNodeType::Chest);
                const bool offerActiveItem = chestTuning.activeItemChancePercent > 0 &&
                    runController_.random().chance(static_cast<double>(chestTuning.activeItemChancePercent) / 100.0);
                if (offerActiveItem) {
                    const std::optional<ActiveItemId> activeItem = runController_.chooseActiveItemReward(
                        content_.activeItems(),
                        runController_.random()
                    );
                    if (activeItem.has_value()) {
                        reward.options.push_back(RewardOption::activeItem(activeItem->value));
                    }
                }

                if (reward.options.empty()) {
                    const std::optional<RelicId> relic = runController_.chooseChestRelic(content_.relics(), runController_.random());
                    if (relic.has_value()) {
                        reward.options.push_back(RewardOption::relic(relic->value));
                    }
                }

                runController_.setPendingChestReward(nodeId, reward);
                saveActiveRun();
                setChestRewardScene(nodeId, std::move(reward));
                return;
            }

            case RunMapNodeType::Event: {
                const int combatChance = content_.mapGenerationForFloor(runController_.run().currentFloorId).questionMarkCombatChance();
                if (runController_.random().chance(static_cast<double>(combatChance) / 100.0)) {
                    runController_.revealNodeType(nodeId, RunMapNodeType::Combat);
                    saveActiveRun();
                    setCombatScene(nodeId);
                    return;
                }

                const FloorDefinition& floor = content_.floors().get(runController_.run().currentFloorId);
                const std::vector<const RunEventDefinition*> eventPool = content_.events().allForPool(floor.eventPoolId);
                const RunEventDefinition& event = chooseAvailableRunEvent(
                    eventPool,
                    runController_.run(),
                    runController_.random()
                );
                runController_.setPendingEvent(nodeId, event.id);
                saveActiveRun();
                setEventScene(nodeId, event);
                return;
            }

            case RunMapNodeType::Shop: {
                ShopState shopState = ShopGenerator::createShop(
                    runController_.run(),
                    content_.cards(),
                    content_.relics(),
                    content_.consumables(),
                    content_.activeItems(),
                    content_.shopTuning(),
                    runController_.random()
                );
                runController_.setPendingShop(nodeId, shopState);
                saveActiveRun();
                setShopScene(nodeId, std::move(shopState));
                return;
            }

            case RunMapNodeType::Rest:
                if (runController_.run().archetypeMechanicId == "merchant_progression") {
                    ShopState merchantRest = ShopGenerator::createMerchantRest(
                        runController_.run(),
                        content_.cards(),
                        content_.shopTuning(),
                        runController_.random()
                    );
                    runController_.setPendingMerchantRest(nodeId, merchantRest);
                    saveActiveRun();
                    setMerchantRestScene(nodeId, std::move(merchantRest));
                    return;
                }

                // Rest is normally handled inside RunMapScene through its modal.
                setRunMapScene();
                return;
        }
    });
}

void GameFlowController::restHeal(const int nodeId) {
    queueTransition([this, nodeId]() {
        if (!runController_.canUseRestNode(nodeId)) {
            setRunMapScene();
            return;
        }

        runController_.startNode(nodeId);
        runController_.completeRestHeal(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::restCalm(const int nodeId) {
    queueTransition([this, nodeId]() {
        if (!runController_.canUseRestNode(nodeId)) {
            setRunMapScene();
            return;
        }

        runController_.startNode(nodeId);
        runController_.completeRestCalm(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::restUpgrade(const int nodeId, const std::size_t deckIndex) {
    queueTransition([this, nodeId, deckIndex]() {
        if (!runController_.canUseRestNode(nodeId)) {
            setRunMapScene();
            return;
        }

        runController_.startNode(nodeId);
        if (runController_.completeRestUpgrade(nodeId, deckIndex)) {
            saveActiveRun();
        }
        setRunMapScene();
    });
}

void GameFlowController::restSkip(const int nodeId) {
    queueTransition([this, nodeId]() {
        if (!runController_.canUseRestNode(nodeId)) {
            setRunMapScene();
            return;
        }

        runController_.startNode(nodeId);
        runController_.completeRestSkip(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

bool GameFlowController::purchaseShopItem(const ShopPurchase& purchase) {
    const bool purchased = runController_.purchaseShopItem(purchase);
    if (purchased) {
        profileProgression_.applyShopPurchase(purchase);
        saveActiveRun();
    }
    return purchased;
}

void GameFlowController::updatePendingShopState(const int nodeId, const ShopState& shopState) {
    if (!runController_.hasPendingRoom() || runController_.pendingRoom().nodeId != nodeId) {
        return;
    }

    const RunPendingRoomType type = runController_.pendingRoom().type;
    if (type == RunPendingRoomType::Shop) {
        runController_.setPendingShop(nodeId, shopState);
        saveActiveRun();
    } else if (type == RunPendingRoomType::MerchantRest) {
        runController_.setPendingMerchantRest(nodeId, shopState);
        saveActiveRun();
    }
}

void GameFlowController::finishShop(const int nodeId) {
    queueTransition([this, nodeId]() {
        if (!runController_.hasPendingRoomForNode(nodeId)) {
            setRunMapScene();
            return;
        }

        const RunPendingRoomType type = runController_.pendingRoom().type;
        if (type == RunPendingRoomType::Shop) {
            runController_.completeShopNode(nodeId);
        } else if (type == RunPendingRoomType::MerchantRest) {
            runController_.completeMerchantRestNode(nodeId);
        } else {
            setRunMapScene();
            return;
        }

        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::finishEvent(const int nodeId, const RunEventChoiceDefinition& choice) {
    queueTransition([this, nodeId, choice]() {
        if (!runController_.hasPendingRoomForNode(nodeId) ||
            runController_.pendingRoom().type != RunPendingRoomType::Event) {
            setRunMapScene();
            return;
        }

        const std::string eventId = runController_.pendingRoom().eventId;
        RunEventChoiceResult result = runController_.completeEventChoice(
            nodeId,
            choice,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            runController_.random()
        );
        if (!result.completed) {
            setEventScene(nodeId, content_.events().get(eventId));
            return;
        }

        profileProgression_.applyEventOutcome(result);

        const std::string seenFlag = "event.seen." + eventId;
        std::vector<std::string>& eventFlags = runController_.run().eventFlags;
        if (std::find(eventFlags.begin(), eventFlags.end(), seenFlag) == eventFlags.end()) {
            eventFlags.push_back(seenFlag);
        }

        if (runController_.partyDefeated()) {
            setRunDefeatScene();
            return;
        }

        saveActiveRun();
        setEventOutcomeScene(std::move(result));
    });
}

void GameFlowController::finishReward(const RewardState&, RewardSelection selection) {
    queueTransition([this, selection = std::move(selection)]() mutable {
        if (!runController_.hasPendingRoom() ||
            runController_.pendingRoom().type != RunPendingRoomType::CombatReward) {
            setRunMapScene();
            return;
        }

        const RewardState pendingReward = runController_.pendingRoom().reward;
        const bool completedBoss = pendingReward.sourceNodeType == RunMapNodeType::Boss;

        runController_.applyReward(pendingReward, selection);
        profileProgression_.applyRewardSelection(selection);
        runController_.clearPendingRoom();

        if (completedBoss) {
            runController_.completeCurrentAct();
            saveActiveRun();
            setFloorCompleteScene();
            return;
        }

        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::finishChestReward(
    const int nodeId,
    RewardSelection selection
) {
    queueTransition([this, nodeId, selection = std::move(selection)]() mutable {
        if (!runController_.hasPendingRoomForNode(nodeId) ||
            runController_.pendingRoom().type != RunPendingRoomType::ChestReward) {
            setRunMapScene();
            return;
        }

        const RewardState reward = runController_.pendingRoom().reward;
        runController_.applyReward(reward, selection);
        profileProgression_.applyRewardSelection(selection);
        runController_.completeChestNode(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::finishFloorCompleteContinue() {
    queueTransition([this]() {
        if (!runController_.hasActiveRun() || !runController_.isActCompleted()) {
            setProfileHubScene();
            return;
        }

        profileProgression_.grantFloorCompletionUnlocks(runController_.run());

        if (advanceCompletedRunToNextFloor()) {
            setRunMapScene();
            return;
        }

        RunState completedRun = runController_.run();
        const RunEndReason reason = completedRunEndReason(completedRun);
        finishRun(reason);
        setRunCompleteScene(std::move(completedRun), reason);
    });
}

void GameFlowController::finishFloorCompleteMainMenu() {
    queueTransition([this]() {
        if (runController_.hasActiveRun() && runController_.isActCompleted()) {
            profileProgression_.grantFloorCompletionUnlocks(runController_.run());

            if (completedRunCanContinueToNextFloor()) {
                saveAndCloseCompletedRunForLater();
                setMainMenuScene();
                return;
            }
        }

        finishCompletedRunAndDeleteSave();
        setMainMenuScene();
    });
}

bool GameFlowController::advanceCompletedRunToNextFloor() {
    if (!runController_.hasActiveRun() || !runController_.isActCompleted()) {
        return false;
    }

    const RunCompletionStatus completion = RunCompletion::evaluate(runController_.run(), content_.floors());
    runController_.run().completionType = completion.type;
    if (!completion.canContinueToNextFloor || !content_.floors().contains(completion.nextFloorId)) {
        return false;
    }

    const FloorDefinition& nextFloor = content_.floors().get(completion.nextFloorId);
    if (!runController_.advanceToNextFloor(nextFloor, content_.mapGenerationForFloor(nextFloor.id))) {
        return false;
    }

    profileProgression_.synchronizeRunContent(runController_.run());
    saveActiveRun();
    return true;
}

bool GameFlowController::completedRunCanContinueToNextFloor() const {
    if (!runController_.hasActiveRun() || !runController_.isActCompleted()) {
        return false;
    }

    const RunCompletionStatus completion = RunCompletion::evaluate(runController_.run(), content_.floors());
    return completion.canContinueToNextFloor;
}

void GameFlowController::saveAndCloseCompletedRunForLater() {
    if (!runController_.hasActiveRun()) {
        return;
    }

    if (runController_.isActCompleted()) {
        const RunCompletionStatus completion = RunCompletion::evaluate(runController_.run(), content_.floors());
        runController_.run().completionType = completion.type;
        if (completion.canContinueToNextFloor) {
            saveActiveRun();
            runController_.clearActiveRun();
            return;
        }

        finishCompletedRunAndDeleteSave();
        return;
    }

    saveActiveRun();
    runController_.clearActiveRun();
}

RunEndReason GameFlowController::completedRunEndReason(const RunState& run) const {
    if (!run.challengeId.empty() && content_.challenges().contains(run.challengeId)) {
        const ChallengeDefinition& challenge = content_.challenges().get(run.challengeId);
        return ChallengeRules::isCompleted(challenge, run)
            ? RunEndReason::ChallengeCompleted
            : RunEndReason::ChallengeFailed;
    }

    return RunEndReason::Victory;
}

RunEndReason GameFlowController::defeatedRunEndReason(const RunState& run) const {
    return run.challengeId.empty()
        ? RunEndReason::Defeat
        : RunEndReason::ChallengeFailed;
}

void GameFlowController::finishRun(const RunEndReason reason) {
    if (runController_.hasActiveRun()) {
        RunState& run = runController_.run();
        runController_.syncRandomStateToRun();

        try {
            RunTelemetryWriter::append("saves/telemetry/run_history.jsonl", run, reason);
        } catch (const std::exception& error) {
            std::cout << "Failed to write local run telemetry: " << error.what() << '\n'; // NOL10N: developer diagnostic
        }

        const bool countsAsVictory = runEndReasonCountsAsVictory(reason);
        const bool countsAsDefeat = runEndReasonCountsAsDefeat(reason);
        if (countsAsVictory || countsAsDefeat) {
            if (profileManager_.recordSelectedRunFinished(run, countsAsVictory)) {
                if (countsAsVictory) {
                    profileProgression_.grantFloorCompletionUnlocks(run);
                } else {
                    profileProgression_.completeEligibleAchievements(&run);
                }
            }
        }
    }

    deleteSelectedRunSave();
    runController_.clearActiveRun();
}

void GameFlowController::finishCompletedRunAndDeleteSave() {
    if (runController_.hasActiveRun() && runController_.isActCompleted()) {
        const RunCompletionStatus completion = RunCompletion::evaluate(runController_.run(), content_.floors());
        runController_.run().completionType = completion.type;
        if (completion.canContinueToNextFloor) {
            saveAndCloseCompletedRunForLater();
            return;
        }

        finishRun(completedRunEndReason(runController_.run()));
        return;
    }

    deleteSelectedRunSave();
    runController_.clearActiveRun();
}

void GameFlowController::renderProfileToasts() const {
    const std::vector<ProfileToast>& toasts = profileProgression_.toasts();
    if (toasts.empty()) {
        return;
    }

    constexpr float width = 460.f;
    constexpr float height = 54.f;
    constexpr float gap = 10.f;
    const float x = static_cast<float>(VirtualViewport::width()) - width - 24.f;
    float y = 86.f;

    const std::size_t first = toasts.size() > 4u ? toasts.size() - 4u : 0u;
    for (std::size_t i = first; i < toasts.size(); ++i) {
        const ProfileToast& toast = toasts[i];
        const unsigned char alpha = static_cast<unsigned char>(toast.remainingSeconds < 0.8f ? 175 : 235);
        const Rectangle bounds{x, y, width, height};
        DrawRectangleRounded(bounds, 0.16f, 8, Color{20, 23, 32, alpha});
        DrawRectangleRoundedLinesEx(bounds, 0.16f, 8, 2.f, Color{116, 130, 180, alpha});
        BasicUi::drawTextFitted(
            uiFont_,
            toast.text,
            Vector2{bounds.x + 16.f, bounds.y + 16.f},
            bounds.width - 32.f,
            20.f,
            15.f,
            Color{238, 241, 250, 255}
        );
        y += height + gap;
    }
}


void GameFlowController::finishRunDefeatToProfileHub() {
    queueTransition([this]() {
        finishDefeatedRunAndDeleteSave();
        setProfileHubScene();
    });
}

void GameFlowController::finishRunDefeatToMainMenu() {
    queueTransition([this]() {
        finishDefeatedRunAndDeleteSave();
        setMainMenuScene();
    });
}

void GameFlowController::finishDefeatedRunAndDeleteSave() {
    if (!runController_.hasActiveRun()) {
        deleteSelectedRunSave();
        return;
    }

    finishRun(defeatedRunEndReason(runController_.run()));
}

bool GameFlowController::hasRunSave(const std::size_t slotIndex) const {
    return runSaveSystem_.hasRunSave(slotIndex);
}

void GameFlowController::saveActiveRun() {
    if (!runController_.hasActiveRun()) {
        return;
    }

    runController_.syncRandomStateToRun();
    runSaveSystem_.saveRun(profileManager_.selectedSlotIndex(), runController_.run());
}

void GameFlowController::deleteSelectedRunSave() {
    runSaveSystem_.deleteRun(profileManager_.selectedSlotIndex());
}

void GameFlowController::requestExit() {
    queueTransition([this]() { exitRequested_ = true; });
}
