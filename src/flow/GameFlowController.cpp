#include "GameFlowController.hpp"

#include "relics/RelicId.hpp"
#include "cards/CardDefinition.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "consumables/ConsumableId.hpp"
#include "relics/RelicDefinition.hpp"
#include "run/RunCardEligibility.hpp"
#include "shop/ShopTuning.hpp"
#include "relics/RelicRarity.hpp"
#include "scenes/CombatScene.hpp"
#include "scenes/DifficultySelectScene.hpp"
#include "scenes/MainMenuScene.hpp"
#include "scenes/ProfileHubScene.hpp"
#include "scenes/RewardScene.hpp"
#include "scenes/RunMapScene.hpp"
#include "scenes/ShopScene.hpp"
#include "scenes/EventScene.hpp"
#include "scenes/SaveSlotScene.hpp"
#include "scenes/SettingsScene.hpp"
#include "scenes/SplashScene.hpp"
#include "ui/BasicUi.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cctype>
#include <cstddef>
#include <cstdint>

#include <raylib.h>


namespace {
bool canSellCard(const CardDefinition& card) {
    if (card.type == CardType::Status || card.type == CardType::Curse) {
        return false;
    }

    if (card.rarity == CardRarity::Starter || card.rarity == CardRarity::Special) {
        return false;
    }

    return true;
}

template <typename T>
void pickUniqueRandom(std::vector<const T*>& candidates, const int count, Random& random, std::vector<const T*>& out) {
    for (int i = 0; i < count && !candidates.empty(); ++i) {
        const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
        out.push_back(candidates[static_cast<std::size_t>(index)]);
        candidates.erase(candidates.begin() + index);
    }
}

ShopState createShopState(
    const RunState& run,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const ShopTuning& tuning,
    Random& random
) {
    ShopState shop;
    shop.cardRemovalPrice = tuning.cardRemovalPrice();

    std::vector<const CardDefinition*> cardCandidates;
    for (const CardDefinition* card : cards.all()) {
        if (card != nullptr && canSellCard(*card) && runCanReceiveCard(run, *card)) {
            cardCandidates.push_back(card);
        }
    }

    std::vector<const CardDefinition*> pickedCards;
    pickUniqueRandom(cardCandidates, tuning.cardOfferCount(), random, pickedCards);
    for (const CardDefinition* card : pickedCards) {
        ShopOffer offer;
        offer.type = ShopOfferType::Card;
        offer.contentId = card->id.value;
        offer.price = std::max(tuning.minimumCardPrice(), card->goldCost);
        shop.offers.push_back(offer);
    }

    std::vector<const RelicDefinition*> relicCandidates;
    for (const RelicDefinition* relic : relics.all()) {
        if (relic == nullptr) {
            continue;
        }

        if (relic->rarity == RelicRarity::Starter || relic->rarity == RelicRarity::Special) {
            continue;
        }

        const bool alreadyOwned = std::find(run.relicIds.begin(), run.relicIds.end(), relic->id.value) != run.relicIds.end();
        if (!alreadyOwned) {
            relicCandidates.push_back(relic);
        }
    }

    std::vector<const RelicDefinition*> pickedRelics;
    pickUniqueRandom(relicCandidates, tuning.relicOfferCount(), random, pickedRelics);
    for (const RelicDefinition* relic : pickedRelics) {
        ShopOffer offer;
        offer.type = ShopOfferType::Relic;
        offer.contentId = relic->id.value;
        offer.price = tuning.relicPrice(relic->rarity);
        shop.offers.push_back(offer);
    }

    std::vector<const ConsumableDefinition*> consumableCandidates;
    for (const ConsumableDefinition* consumable : consumables.all()) {
        if (consumable != nullptr) {
            consumableCandidates.push_back(consumable);
        }
    }

    std::vector<const ConsumableDefinition*> pickedConsumables;
    pickUniqueRandom(consumableCandidates, tuning.consumableOfferCount(), random, pickedConsumables);
    for (const ConsumableDefinition* consumable : pickedConsumables) {
        ShopOffer offer;
        offer.type = ShopOfferType::Consumable;
        offer.contentId = consumable->id.value;
        offer.price = std::max(tuning.minimumConsumablePrice(), consumable->goldCost);
        shop.offers.push_back(offer);
    }

    ShopOffer removal;
    removal.type = ShopOfferType::CardRemoval;
    removal.price = shop.cardRemovalPrice;
    shop.offers.push_back(removal);

    return shop;
}

const RunEventDefinition& chooseRunEvent(const EventDatabase& events, Random& random) {
    const std::vector<const RunEventDefinition*> all = events.all();
    if (all.empty()) {
        throw std::runtime_error("Cannot open event node: no run events loaded");
    }

    const int index = random.rangeInclusive(0, static_cast<int>(all.size()) - 1);
    return *all[static_cast<std::size_t>(index)];
}

Rectangle inGameSettingsButtonBounds() {
    constexpr float width = 170.f;
    constexpr float height = 42.f;
    return Rectangle{
        static_cast<float>(GetScreenWidth()) - width - 24.f,
        24.f,
        width,
        height
    };
}

Rectangle debugPanelBounds() {
    const float width = std::min(760.f, static_cast<float>(GetScreenWidth()) - 64.f);
    const float height = std::min(520.f, static_cast<float>(GetScreenHeight()) - 64.f);
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        (static_cast<float>(GetScreenHeight()) - height) * 0.5f,
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
      runSaveSystem_(savesPath) {
    if (!uiFont_.loadFromAssetsDirectory(assetsPath)) {
        std::cout << "UI font was not found. Put a Unicode font at assets/fonts/main.ttf.\n";
    } else {
        std::cout << "Loaded UI font: " << uiFont_.loadedPath().string() << '\n';
    }

    setSplashScene();
}

void GameFlowController::update(const float deltaSeconds) {
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

    sceneManager_.update(deltaSeconds);
    executePendingTransition();
}

void GameFlowController::render() const {
    sceneManager_.render();

    if (settingsOverlay_ != nullptr) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 170});
        settingsOverlay_->render();
        return;
    }

    if (debugPanelOpen_) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 150});
        renderDebugPanel();
        return;
    }

    if (shouldShowInGameSettingsButton()) {
        BasicUi::drawButton(
            uiFont_,
            inGameSettingsButtonBounds(),
            localization_.get(TextId("ui.settings")),
            GetMousePosition()
        );
    }

    if (debugPanelEnabled()) {
        BasicUi::drawText(
            uiFont_,
            "F1 Debug",
            Vector2{24.f, static_cast<float>(GetScreenHeight()) - 34.f},
            18.f,
            Color{170, 176, 198, 255}
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
        [this]() { closeSettingsOverlay(); }
    );
}

void GameFlowController::closeSettingsOverlay() {
    settingsOverlay_.reset();
}

bool GameFlowController::shouldShowInGameSettingsButton() const {
    return runController_.hasActiveRun();
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
        addDebugMessage("Debug panel opened. Type 'help' for commands.");
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

    if (IsKeyPressed(KEY_BACKSPACE) && !debugInput_.empty()) {
        debugInput_.pop_back();
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
            debugInput_ = "give gold 50";
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

    BasicUi::drawText(uiFont_, "Debug Panel", Vector2{panel.x + 24.f, panel.y + 22.f}, 28.f, Color{240, 242, 250, 255});
    BasicUi::drawText(uiFont_, "F1/Esc close. Enter runs command.", Vector2{panel.x + 24.f, panel.y + 56.f}, 17.f, Color{165, 172, 196, 255});
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + panel.width - 124.f, panel.y + 20.f, 96.f, 34.f}, "Close", mouse);

    const Rectangle input{panel.x + 24.f, panel.y + 88.f, panel.width - 48.f, 42.f};
    DrawRectangleRounded(input, 0.15f, 8, Color{12, 14, 20, 255});
    DrawRectangleRoundedLinesEx(input, 0.15f, 8, 2.f, Color{80, 86, 112, 255});
    BasicUi::drawText(uiFont_, "> " + debugInput_ + "_", Vector2{input.x + 12.f, input.y + 10.f}, 20.f, Color{235, 237, 245, 255});

    const float logTop = panel.y + 148.f;
    const float logBottom = panel.y + panel.height - 124.f;
    const int maxLines = static_cast<int>((logBottom - logTop) / 22.f);
    const int start = std::max(0, static_cast<int>(debugMessages_.size()) - maxLines);
    float y = logTop;
    for (int i = start; i < static_cast<int>(debugMessages_.size()); ++i) {
        BasicUi::drawText(uiFont_, debugMessages_[static_cast<std::size_t>(i)], Vector2{panel.x + 24.f, y}, 17.f, Color{205, 211, 232, 255});
        y += 22.f;
    }

    const float quickY = panel.y + panel.height - 104.f;
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + 24.f, quickY, 140.f, 38.f}, "+50 gold", mouse);
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + 176.f, quickY, 140.f, 38.f}, "Full heal", mouse);
    BasicUi::drawButton(uiFont_, Rectangle{panel.x + 328.f, quickY, 140.f, 38.f}, "Save", mouse);

    BasicUi::drawText(
        uiFont_,
        "Examples: give card cyborg_liquid_assets | give relic relic_id | status strength 3 | win combat",
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
    addDebugMessage(executeDebugCommand(command));
    debugInput_.clear();
}

std::string GameFlowController::executeDebugCommand(const std::string& command) {
    const std::vector<std::string> tokens = splitCommand(command);
    if (tokens.empty()) {
        return "Empty command";
    }

    std::string sceneOutput;
    if (sceneManager_.handleDebugCommand(tokens, sceneOutput)) {
        return sceneOutput;
    }

    std::string runOutput;
    if (executeRunDebugCommand(tokens, runOutput)) {
        return runOutput;
    }

    return "Unknown debug command. Type 'help'.";
}

bool GameFlowController::executeRunDebugCommand(const std::vector<std::string>& tokens, std::string& output) {
    if (tokens.empty()) {
        return false;
    }

    const std::string& command = tokens.front();

    if (command == "help") {
        output = "Commands: give gold N | give card ID [N] | give relic ID | give consumable ID | heal N | damage N | fullheal | status ID N | win combat | save | clear pending | unlock map";
        return true;
    }

    if (command == "save") {
        saveActiveRun();
        output = runController_.hasActiveRun() ? "Run saved" : "No active run to save";
        return true;
    }

    if (command == "clear" && tokens.size() >= 2u && tokens[1] == "pending") {
        if (!runController_.hasActiveRun()) {
            output = "No active run";
            return true;
        }
        runController_.clearPendingRoom();
        saveActiveRun();
        output = "Pending room cleared";
        return true;
    }

    if (command == "unlock" && tokens.size() >= 2u && tokens[1] == "map") {
        if (!runController_.hasActiveRun()) {
            output = "No active run";
            return true;
        }
        for (RunMapNode& node : runController_.run().map.nodes) {
            if (node.state == RunMapNodeState::Locked) {
                node.state = RunMapNodeState::Available;
            }
        }
        saveActiveRun();
        output = "All map nodes unlocked";
        return true;
    }

    if ((command == "give" || command == "add") && tokens.size() >= 3u) {
        if (!runController_.hasActiveRun()) {
            output = "No active run";
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
            output = "Gold adjusted by " + std::to_string(amount) + ". Current gold: " + std::to_string(run.gold);
            return true;
        }

        if (type == "card") {
            const CardId cardId(idOrAmount);
            if (!content_.cards().contains(cardId)) {
                output = "Unknown card id: " + idOrAmount;
                return true;
            }
            const int count = std::max(1, parseIntOr(tokens, 3u, 1));
            for (int i = 0; i < count; ++i) {
                run.deckCardIds.push_back(cardId);
                ++run.stats.cardsAdded;
            }
            saveActiveRun();
            output = "Added card '" + idOrAmount + "' x" + std::to_string(count);
            return true;
        }

        if (type == "relic") {
            const RelicId relicId(idOrAmount);
            if (!content_.relics().contains(relicId)) {
                output = "Unknown relic id: " + idOrAmount;
                return true;
            }
            if (std::find(run.relicIds.begin(), run.relicIds.end(), idOrAmount) == run.relicIds.end()) {
                run.relicIds.push_back(idOrAmount);
            }
            saveActiveRun();
            output = "Added relic '" + idOrAmount + "'";
            return true;
        }

        if (type == "consumable" || type == "potion") {
            const ConsumableId consumableId(idOrAmount);
            if (!content_.consumables().contains(consumableId)) {
                output = "Unknown consumable id: " + idOrAmount;
                return true;
            }
            if (static_cast<int>(run.consumableIds.size()) >= run.maxConsumables) {
                output = "Consumable slots are full";
                return true;
            }
            run.consumableIds.push_back(idOrAmount);
            saveActiveRun();
            output = "Added consumable '" + idOrAmount + "'";
            return true;
        }
    }

    if (command == "heal" || command == "damage") {
        if (!runController_.hasActiveRun()) {
            output = "No active run";
            return true;
        }
        const int amount = parseIntOr(tokens, 1u, 0);
        if (amount <= 0) {
            output = "Usage: " + command + " <amount>";
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
        output = command == "heal" ? "Run actors healed" : "Run actors damaged";
        return true;
    }

    if (command == "fullheal") {
        if (!runController_.hasActiveRun()) {
            output = "No active run";
            return true;
        }
        for (RunActorState& actor : runController_.run().actorStates) {
            actor.currentHp = std::max(1, actor.maxHp);
        }
        saveActiveRun();
        output = "Run actors fully healed";
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
            [this](const std::size_t slotIndex) { startNewRunInSlot(slotIndex); },
            [this](const std::size_t slotIndex) { continueRunInSlot(slotIndex); },
            [this](const std::size_t slotIndex) { deleteRunInSlot(slotIndex); },
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
            localization_,
            runController_.run(),
            [this](const int nodeId) { startMapNode(nodeId); },
            [this](const int nodeId) { restHeal(nodeId); },
            [this](const int nodeId) { restUpgrade(nodeId); },
            [this](const int nodeId) { restSkip(nodeId); },
            [this]() { queueTransition([this]() { setProfileHubScene(); }); }
        )
    );
}

bool GameFlowController::setPendingRoomSceneIfNeeded() {
    if (!runController_.hasPendingRoom()) {
        return false;
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
                    pending.reward,
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

        case RunPendingRoomType::Event:
            if (!content_.events().contains(pending.eventId)) {
                throw std::runtime_error("Cannot restore pending event room: unknown event id '" + pending.eventId + "'");
            }
            setEventScene(pending.nodeId, content_.events().get(pending.eventId));
            return true;

        case RunPendingRoomType::None:
            return false;
    }

    return false;
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
                RewardState reward = runController_.completeCombatAndCreateReward(
                    nodeId,
                    result,
                    content_.cards(),
                    content_.relics(),
                    content_.rewardTuning(),
                    random_
                );
                runController_.setPendingCombatReward(nodeId, reward);
                saveActiveRun();
                return reward;
            },
            [this](const RewardState& reward, RewardSelection selection) {
                finishReward(reward, std::move(selection));
            },
            [this](const CombatResult&) {
                queueTransition([this]() {
                    deleteSelectedRunSave();
                    runController_.clearActiveRun();
                    setProfileHubScene();
                });
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
            std::move(reward),
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
            runController_.run(),
            std::move(shopState),
            [this](const ShopPurchase& purchase) { return purchaseShopItem(purchase); },
            [this, nodeId](const ShopState& changedShopState) { updatePendingShopState(nodeId, changedShopState); },
            [this, nodeId]() { finishShop(nodeId); }
        )
    );
}

void GameFlowController::setEventScene(const int nodeId, const RunEventDefinition& event) {
    sceneManager_.setScene(
        std::make_unique<EventScene>(
            uiFont_,
            localization_,
            event,
            [this, nodeId](const RunEventChoiceDefinition& choice) { finishEvent(nodeId, choice); }
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
        profileManager_.selectSlot(slotIndex);
        runSaveSystem_.deleteRun(slotIndex);
        runController_.clearActiveRun();
        selectedArchetypeId_.reset();
        selectedDifficultyId_.reset();
        setProfileHubScene();
    });
}

void GameFlowController::continueRunInSlot(const std::size_t slotIndex) {
    queueTransition([this, slotIndex]() {
        profileManager_.selectSlot(slotIndex);
        runController_.restoreRun(runSaveSystem_.loadRun(slotIndex));
        selectedArchetypeId_.reset();
        selectedDifficultyId_.reset();
        if (!setPendingRoomSceneIfNeeded()) {
            setRunMapScene();
        }
    });
}

void GameFlowController::deleteRunInSlot(const std::size_t slotIndex) {
    queueTransition([this, slotIndex]() {
        runSaveSystem_.deleteRun(slotIndex);
        if (profileManager_.selectedSlotIndex() == slotIndex && runController_.hasActiveRun()) {
            runController_.clearActiveRun();
        }
        setSaveSlotScene();
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

        const std::uint32_t runSeed =
            (static_cast<std::uint32_t>(random_.rangeInclusive(0, 65535)) << 16) |
            static_cast<std::uint32_t>(random_.rangeInclusive(0, 65535));

        runController_.startNewRun(
            archetype,
            difficulty,
            content_.actors(),
            content_.actOneMapGeneration(),
            runSeed
        );
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::startMapNode(const int nodeId) {
    queueTransition([this, nodeId]() {
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

                const std::optional<RelicId> relic = runController_.chooseChestRelic(content_.relics(), random_);
                if (relic.has_value()) {
                    reward.options.push_back(RewardOption::relic(relic->value));
                }

                runController_.setPendingChestReward(nodeId, reward);
                saveActiveRun();
                setChestRewardScene(nodeId, std::move(reward));
                return;
            }

            case RunMapNodeType::Event: {
                const RunEventDefinition& event = chooseRunEvent(content_.events(), random_);
                runController_.setPendingEvent(nodeId, event.id);
                saveActiveRun();
                setEventScene(nodeId, event);
                return;
            }

            case RunMapNodeType::Shop: {
                ShopState shopState = createShopState(
                    runController_.run(),
                    content_.cards(),
                    content_.relics(),
                    content_.consumables(),
                    content_.shopTuning(),
                    random_
                );
                runController_.setPendingShop(nodeId, shopState);
                saveActiveRun();
                setShopScene(nodeId, std::move(shopState));
                return;
            }

            case RunMapNodeType::Rest:
                // Rest is normally handled inside RunMapScene through its modal.
                setRunMapScene();
                return;
        }
    });
}

void GameFlowController::restHeal(const int nodeId) {
    queueTransition([this, nodeId]() {
        runController_.startNode(nodeId);
        runController_.completeRestHeal(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::restUpgrade(const int nodeId) {
    queueTransition([this, nodeId]() {
        runController_.startNode(nodeId);
        runController_.completeRestUpgrade(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::restSkip(const int nodeId) {
    queueTransition([this, nodeId]() {
        runController_.startNode(nodeId);
        runController_.completeRestSkip(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

bool GameFlowController::purchaseShopItem(const ShopPurchase& purchase) {
    const bool purchased = runController_.purchaseShopItem(purchase);
    if (purchased) {
        saveActiveRun();
    }
    return purchased;
}

void GameFlowController::updatePendingShopState(const int nodeId, const ShopState& shopState) {
    if (runController_.hasPendingRoom() &&
        runController_.pendingRoom().type == RunPendingRoomType::Shop &&
        runController_.pendingRoom().nodeId == nodeId) {
        runController_.setPendingShop(nodeId, shopState);
        saveActiveRun();
    }
}

void GameFlowController::finishShop(const int nodeId) {
    queueTransition([this, nodeId]() {
        runController_.completeShopNode(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::finishEvent(const int nodeId, const RunEventChoiceDefinition& choice) {
    queueTransition([this, nodeId, choice]() {
        runController_.completeEventChoice(
            nodeId,
            choice,
            content_.cards(),
            content_.relics(),
            content_.consumables(),
            random_
        );
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::finishReward(const RewardState& reward, RewardSelection selection) {
    queueTransition([this, reward, selection = std::move(selection)]() mutable {
        runController_.applyReward(reward, selection);
        runController_.clearPendingRoom();
        saveActiveRun();
        setRunMapScene();
    });
}

void GameFlowController::finishChestReward(
    const int nodeId,
    RewardSelection selection
) {
    queueTransition([this, nodeId, selection = std::move(selection)]() mutable {
        runController_.applyReward(RewardState{}, selection);
        runController_.completeChestNode(nodeId);
        saveActiveRun();
        setRunMapScene();
    });
}

bool GameFlowController::hasRunSave(const std::size_t slotIndex) const {
    return runSaveSystem_.hasRunSave(slotIndex);
}

void GameFlowController::saveActiveRun() {
    if (!runController_.hasActiveRun()) {
        return;
    }

    runSaveSystem_.saveRun(profileManager_.selectedSlotIndex(), runController_.run());
}

void GameFlowController::deleteSelectedRunSave() {
    runSaveSystem_.deleteRun(profileManager_.selectedSlotIndex());
}

void GameFlowController::requestExit() {
    queueTransition([this]() { exitRequested_ = true; });
}
