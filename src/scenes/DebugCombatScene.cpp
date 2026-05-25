#include "DebugCombatScene.hpp"

#include "cards/DrawSystem.hpp"
#include "combat/CombatPhase.hpp"
#include "enemies/EnemyInstance.hpp"

#include <iostream>
#include <stdexcept>

namespace {
CombatEntity makeDebugPlayer(const EntityId id) {
    CombatEntity player;
    player.id = id;
    player.type = EntityType::Player;
    player.definitionId = "debug_player";
    player.nameTextId = TextId("debug.player.name");
    player.health = Health(70);
    player.block = 0;
    player.statuses.add("strength", 3);
    return player;
}
}

DebugCombatScene::DebugCombatScene(
    const ContentRegistry& content,
    const LocalizationManager& localization,
    Random& random,
    const std::filesystem::path& assetsPath
)
    : content_(content),
      localization_(localization),
      random_(random),
      damageSystem_(modifierSystem_),
      blockSystem_(modifierSystem_),
      effectSystem_(
          effectResolver_,
          targeting_,
          damageSystem_,
          blockSystem_,
          energySystem_,
          drawSystem_
      ),
      cardPlaySystem_(
          content_.cards(),
          validator_,
          energySystem_,
          effectSystem_
      ),
      previewSystem_(
          content_.cards(),
          validator_,
          effectResolver_,
          damageSystem_,
          blockSystem_
      ),
      cardViewModelBuilder_(
          content_.cards(),
          localization_,
          previewSystem_
      ),
      combatViewModelBuilder_(
          localization_,
          cardViewModelBuilder_
      ) {
    if (!uiFont_.loadFromAssetsDirectory(assetsPath)) {
        std::cout << "UI font was not found. Put a Unicode font at assets/fonts/main.ttf.\n";
    } else {
        std::cout << "Loaded UI font: " << uiFont_.loadedPath().string() << '\n';
    }

    initializeCombat();
}

void DebugCombatScene::update(const float deltaSeconds) {
    const Vector2 mousePosition = GetMousePosition();

    view_.setSelectedCard(selectedCardId_);
    view_.update(deltaSeconds, mousePosition);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        handleClick(mousePosition);
    }

    const std::optional<EntityId> previewTarget = selectedCardId_.has_value()
        ? view_.hoveredEnemyId()
        : std::nullopt;

    if (previewTarget != lastPreviewTarget_) {
        lastPreviewTarget_ = previewTarget;
        viewModelDirty_ = true;
    }

    if (viewModelDirty_) {
        rebuildViewModel(previewTarget);
        view_.setSelectedCard(selectedCardId_);
        viewModelDirty_ = false;
    }
}

void DebugCombatScene::render() const {
    view_.render(uiFont_.available() ? &uiFont_.font() : nullptr);
}

void DebugCombatScene::initializeCombat() {
    if (!content_.cards().contains(CardId("strike")) ||
        !content_.cards().contains(CardId("defend")) ||
        !content_.cards().contains(CardId("poisoned_guard")) ||
        !content_.enemies().contains(EnemyId("training_dummy"))) {
        throw std::runtime_error("DebugCombatScene requires strike, defend, poisoned_guard and training_dummy content");
    }

    state_ = CombatState{};
    entityIds_.reset();
    selectedCardId_.reset();
    lastPreviewTarget_.reset();

    state_.phase = CombatPhase::PlayerTurn;
    state_.turn = 1;
    state_.resources.setMaxEnergy(3);
    state_.resources.resetEnergy();

    playerId_ = entityIds_.create();
    state_.players.push_back(makeDebugPlayer(playerId_));

    const EntityId enemyId = entityIds_.create();
    const EnemyDefinition& enemyDefinition = content_.enemies().get(EnemyId("training_dummy"));
    state_.enemies.push_back(makeEnemyEntity(enemyDefinition, enemyId));
    state_.entity(enemyId).statuses.add("vulnerable", 2);

    state_.deck.drawPile.addTop(cardFactory_.create(CardId("defend")));
    state_.deck.drawPile.addTop(cardFactory_.create(CardId("poisoned_guard")));
    state_.deck.drawPile.addTop(cardFactory_.create(CardId("strike")));
    state_.deck.drawPile.addTop(cardFactory_.create(CardId("strike")));
    state_.deck.drawPile.addTop(cardFactory_.create(CardId("defend")));

    drawSystem_.drawCards(state_.deck, state_.hand, 5, random_);
    state_.log.add("Debug combat started");

    viewModelDirty_ = true;
    rebuildViewModel(std::nullopt);
    viewModelDirty_ = false;
}

void DebugCombatScene::rebuildViewModel(const std::optional<EntityId> previewTarget) {
    view_.setModel(
        combatViewModelBuilder_.build(
            state_,
            playerId_,
            previewTarget
        )
    );
}

void DebugCombatScene::handleClick(const Vector2) {
    if (selectedCardId_.has_value() && view_.hoveredEnemyId().has_value()) {
        playSelectedCardOn(*view_.hoveredEnemyId());
        return;
    }

    if (view_.hoveredCardId().has_value()) {
        selectedCardId_ = *view_.hoveredCardId();
        viewModelDirty_ = true;
        return;
    }

    selectedCardId_.reset();
    viewModelDirty_ = true;
}

void DebugCombatScene::playSelectedCardOn(const EntityId target) {
    if (!selectedCardId_.has_value()) {
        return;
    }

    const PlayCardResult result = cardPlaySystem_.playCard(
        state_,
        PlayCardRequest{*selectedCardId_, playerId_, target},
        random_
    );

    if (!result.played) {
        state_.log.add("Cannot play card: " + result.reason);
    }

    selectedCardId_.reset();
    lastPreviewTarget_.reset();
    drawIfHandIsLow();
    viewModelDirty_ = true;
}

void DebugCombatScene::drawIfHandIsLow() {
    if (state_.hand.size() >= 3) {
        return;
    }

    drawSystem_.drawCards(state_.deck, state_.hand, 1, random_);
}
