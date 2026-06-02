#include "CombatScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "effects/EffectDefinition.hpp"
#include "effects/EffectTarget.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

void CombatScene::updateInspectInput(const Vector2) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        inspectedCardId_.reset();
        closeCombatItemInspect();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (view_.hoveredRelicIndex().has_value()) {
            openRelicInspect(*view_.hoveredRelicIndex());
            return;
        }

        if (view_.hoveredConsumableIndex().has_value()) {
            openConsumableInspect(*view_.hoveredConsumableIndex());
            return;
        }

        if (view_.hoveredCardId().has_value()) {
            inspectedCardId_ = *view_.hoveredCardId();
            closeCombatItemInspect();
            return;
        }
    }

    if (IsKeyPressed(KEY_I)) {
        if (view_.hoveredRelicIndex().has_value()) {
            openRelicInspect(*view_.hoveredRelicIndex());
            return;
        }

        if (view_.hoveredConsumableIndex().has_value()) {
            openConsumableInspect(*view_.hoveredConsumableIndex());
            return;
        }

        if (selectedCardId_.has_value()) {
            inspectedCardId_ = selectedCardId_;
            closeCombatItemInspect();
            return;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!view_.hoveredCardId().has_value()) {
            inspectedCardId_.reset();
        }
    }
}

void CombatScene::renderInspectOverlay() const {
    if (relicInspectModal_.isOpen()) {
        relicInspectModal_.render(uiFont_, localization_, view_.model().relics);
        return;
    }

    if (inspectedCardId_.has_value()) {
        const std::optional<CardViewModel> cardModel = inspectedCardViewModel();
        if (cardModel.has_value() && state_.hand.contains(*inspectedCardId_)) {
            const CardInstance& instance = state_.hand.get(*inspectedCardId_);
            const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);
            const InspectPanelModel panel = inspectModelBuilder_.buildCard(definition, *cardModel);

            const float width = std::min(540.f, static_cast<float>(VirtualViewport::width()) - 60.f);
            const Rectangle bounds{
                static_cast<float>(VirtualViewport::width()) - width - 24.f,
                94.f,
                width,
                std::min(660.f, static_cast<float>(VirtualViewport::height()) - 120.f)
            };
            inspectPanelView_.render(uiFont_, panel, bounds);
        }
        return;
    }

    if (view_.hoveredRelicIndex().has_value()) {
        const std::size_t index = *view_.hoveredRelicIndex();
        if (index < view_.model().relics.size()) {
            const RelicViewModel& relicModel = view_.model().relics[index];
            InspectPanelModel panel;
            if (content_.relics().contains(RelicId(relicModel.id))) {
                panel = inspectModelBuilder_.buildRelic(content_.relics().get(RelicId(relicModel.id)));
            } else {
                panel.header = relicModel.name;
                panel.subheader = relicModel.description;
            }

            const float screenWidth = static_cast<float>(VirtualViewport::width());
            const float screenHeight = static_cast<float>(VirtualViewport::height());
            constexpr float screenMargin = 18.f;
            const float width = std::min(460.f, screenWidth - screenMargin * 2.f);
            const Rectangle bounds{
                screenMargin,
                78.f,
                width,
                std::min(500.f, screenHeight - 120.f)
            };
            inspectPanelView_.render(uiFont_, panel, bounds);
            return;
        }
    }

    if (view_.hoveredConsumableIndex().has_value()) {
        const std::size_t index = *view_.hoveredConsumableIndex();
        if (index < view_.model().consumables.size()) {
            const ConsumableViewModel& consumableModel = view_.model().consumables[index];
            InspectPanelModel panel;
            if (consumableModel.filled && content_.consumables().contains(ConsumableId(consumableModel.id))) {
                panel = inspectModelBuilder_.buildConsumable(content_.consumables().get(ConsumableId(consumableModel.id)));
            } else {
                panel = inspectModelBuilder_.buildConsumable(consumableModel);
            }
            const float screenWidth = static_cast<float>(VirtualViewport::width());
            const float screenHeight = static_cast<float>(VirtualViewport::height());
            constexpr float screenMargin = 18.f;
            const float width = std::min(420.f, screenWidth - screenMargin * 2.f);
            const Rectangle bounds{
                std::max(screenMargin, screenWidth - width - screenMargin),
                78.f,
                width,
                std::min(360.f, screenHeight - 120.f)
            };
            inspectPanelView_.render(uiFont_, panel, bounds);
            return;
        }
    }

    if (view_.hoveredDroneSlotIndex().has_value()) {
        const std::size_t index = *view_.hoveredDroneSlotIndex();
        if (index < view_.model().droneSlots.size()) {
            const InspectPanelModel panel = inspectModelBuilder_.buildDroneSlot(view_.model().droneSlots[index]);

            constexpr float gap = 12.f;
            constexpr float screenMargin = 18.f;
            constexpr float minWidth = 320.f;
            constexpr float preferredWidth = 460.f;

            const float screenWidth = static_cast<float>(VirtualViewport::width());
            const float screenHeight = static_cast<float>(VirtualViewport::height());
            Rectangle bounds{
                screenWidth * 0.5f - preferredWidth * 0.5f,
                136.f,
                preferredWidth,
                std::min(360.f, screenHeight - 150.f)
            };

            const std::optional<Rectangle> slotBounds = view_.hoveredDroneSlotBounds();
            if (slotBounds.has_value()) {
                const float rightX = slotBounds->x + slotBounds->width + gap;
                const float availableRightWidth = screenWidth - rightX - screenMargin;

                if (availableRightWidth >= minWidth) {
                    bounds.x = rightX;
                    bounds.width = std::clamp(availableRightWidth, minWidth, preferredWidth);
                } else {
                    const float availableLeftWidth = slotBounds->x - gap - screenMargin;
                    bounds.width = std::clamp(availableLeftWidth, minWidth, preferredWidth);
                    bounds.x = std::max(screenMargin, slotBounds->x - gap - bounds.width);
                }

                bounds.y = std::clamp(slotBounds->y, 82.f, screenHeight - bounds.height - screenMargin);
            }

            inspectPanelView_.render(uiFont_, panel, bounds);
            return;
        }
    }

    if (view_.hoveredStatus().has_value()) {
        const InspectPanelModel panel = inspectModelBuilder_.buildStatus(*view_.hoveredStatus());

        constexpr float gap = 12.f;
        constexpr float screenMargin = 18.f;
        constexpr float preferredWidth = 360.f;
        const float screenWidth = static_cast<float>(VirtualViewport::width());
        const float screenHeight = static_cast<float>(VirtualViewport::height());

        Rectangle bounds{
            screenWidth - preferredWidth - screenMargin,
            94.f,
            preferredWidth,
            std::min(360.f, screenHeight - 120.f)
        };

        const std::optional<Rectangle> statusBounds = view_.hoveredStatusBounds();
        if (statusBounds.has_value()) {
            const float rightX = statusBounds->x + statusBounds->width + gap;
            if (rightX + preferredWidth + screenMargin <= screenWidth) {
                bounds.x = rightX;
            } else {
                bounds.x = std::max(screenMargin, statusBounds->x - gap - preferredWidth);
            }
            bounds.y = std::clamp(statusBounds->y - 16.f, 82.f, screenHeight - bounds.height - screenMargin);
        }

        inspectPanelView_.render(uiFont_, panel, bounds);
        return;
    }

    const std::optional<PlayerViewModel> playerModel = hoveredPlayerViewModel();
    if (playerModel.has_value()) {
        const InspectPanelModel panel = inspectModelBuilder_.buildPlayer(*playerModel);

        constexpr float gap = 12.f;
        constexpr float screenMargin = 18.f;
        constexpr float minWidth = 220.f;
        constexpr float preferredWidth = 300.f;

        const float screenHeight = static_cast<float>(VirtualViewport::height());

        Rectangle bounds{
            screenMargin,
            94.f,
            preferredWidth,
            std::min(440.f, screenHeight - 120.f)
        };

        const std::optional<Rectangle> playerBounds = view_.hoveredPlayerBounds();
        if (playerBounds.has_value()) {
            const float availableLeftWidth = playerBounds->x - gap - screenMargin;
            bounds.width = std::clamp(availableLeftWidth, minWidth, preferredWidth);
            bounds.x = std::max(screenMargin, playerBounds->x - gap - bounds.width);
            bounds.y = std::clamp(playerBounds->y, 82.f, screenHeight - bounds.height - screenMargin);
        }

        inspectPanelView_.render(uiFont_, panel, bounds);
        return;
    }

    const std::optional<EnemyViewModel> enemyModel = hoveredEnemyViewModel();
    if (enemyModel.has_value()) {
        const InspectPanelModel panel = inspectModelBuilder_.buildEnemy(*enemyModel);

        constexpr float gap = 12.f;
        constexpr float screenMargin = 18.f;
        constexpr float minWidth = 220.f;
        constexpr float preferredWidth = 300.f;

        const float screenWidth = static_cast<float>(VirtualViewport::width());
        const float screenHeight = static_cast<float>(VirtualViewport::height());

        Rectangle bounds{
            screenWidth - preferredWidth - screenMargin,
            94.f,
            preferredWidth,
            std::min(440.f, screenHeight - 120.f)
        };

        const std::optional<Rectangle> enemyBounds = view_.hoveredEnemyBounds();
        if (enemyBounds.has_value()) {
            const float rightX = enemyBounds->x + enemyBounds->width + gap;
            const float availableRightWidth = screenWidth - rightX - screenMargin;
            bounds.width = std::clamp(availableRightWidth, minWidth, preferredWidth);
            bounds.x = std::min(rightX, screenWidth - bounds.width - screenMargin);
            bounds.x = std::max(screenMargin, bounds.x);
            bounds.y = std::clamp(enemyBounds->y, 82.f, screenHeight - bounds.height - screenMargin);
        }

        inspectPanelView_.render(uiFont_, panel, bounds);
    }
}

std::optional<EnemyViewModel> CombatScene::hoveredEnemyViewModel() const {
    if (!view_.hoveredEnemyId().has_value()) {
        return std::nullopt;
    }

    for (const EnemyViewModel& enemy : view_.model().enemies) {
        if (enemy.entityId == *view_.hoveredEnemyId()) {
            return enemy;
        }
    }

    return std::nullopt;
}

std::optional<PlayerViewModel> CombatScene::hoveredPlayerViewModel() const {
    if (!view_.hoveredPlayerId().has_value()) {
        return std::nullopt;
    }

    for (const PlayerViewModel& player : view_.model().players) {
        if (player.entityId == *view_.hoveredPlayerId()) {
            return player;
        }
    }

    return std::nullopt;
}

std::optional<CardViewModel> CombatScene::inspectedCardViewModel() const {
    if (!inspectedCardId_.has_value() || !state_.hand.contains(*inspectedCardId_)) {
        return std::nullopt;
    }

    const std::optional<EntityId> previewTarget = lastPreviewTarget_;
    return cardViewModelBuilder_.build(
        state_,
        *inspectedCardId_,
        sourceForCard(*inspectedCardId_),
        previewTarget
    );
}

CardViewModel CombatScene::cardViewModelForInstance(const CardInstance& card) const {
    CardViewModel model;
    model.instanceId = card.instanceId;

    if (!content_.cards().contains(card.definitionId)) {
        model.name = card.definitionId.value;
        model.description = card.definitionId.value;
        model.energyCost = 0;
        model.playable = false;
        model.upgraded = card.upgraded;
        return model;
    }

    const CardDefinition& baseDefinition = content_.cards().get(card.definitionId);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(baseDefinition, card.upgraded);
    const CardDescriptionFormatter formatter(localization_);

    model.name = localizedOrFallback(definition.nameTextId, card.definitionId.value) + (card.upgraded ? "+" : "");
    model.description = formatter.formatStaticDescription(definition);
    if (definition.ownerActorId.empty()) {
        model.ownerLabel = localizedOrFallback(TextId("ui.card_owner.common"), "Common card");
    } else {
        std::string ownerName = definition.ownerActorId;
        if (content_.actors().contains(PlayerActorId(definition.ownerActorId))) {
            const PlayerActorDefinition& actor = content_.actors().get(PlayerActorId(definition.ownerActorId));
            ownerName = localizedOrFallback(actor.nameTextId, definition.ownerActorId);
        }

        model.ownerLabel = localization_.format(TextId("ui.card_owner.actor"), {{"actor", ownerName}});
    }
    model.energyCost = definition.energyCost;
    model.type = definition.type;
    model.rarity = definition.rarity;
    model.playable = false;
    model.upgraded = card.upgraded;
    return model;
}


bool CombatScene::combatItemInspectOpen() const {
    return inspectedRelicIndex_.has_value() || inspectedConsumableIndex_.has_value();
}

void CombatScene::openRelicInspect(const std::size_t index) {
    if (index >= view_.model().relics.size()) {
        return;
    }

    inspectedRelicIndex_ = index;
    inspectedConsumableIndex_.reset();
    inspectedCardId_.reset();
    inspectedPileCardIndex_.reset();
    relicInspectModal_.close();
    clearCardSelection();
}

void CombatScene::openConsumableInspect(const std::size_t index) {
    if (index >= view_.model().consumables.size() || !view_.model().consumables[index].filled) {
        return;
    }

    inspectedConsumableIndex_ = index;
    inspectedRelicIndex_.reset();
    inspectedCardId_.reset();
    inspectedPileCardIndex_.reset();
    relicInspectModal_.close();
    clearCardSelection();
}

void CombatScene::closeCombatItemInspect() {
    inspectedRelicIndex_.reset();
    inspectedConsumableIndex_.reset();
}

void CombatScene::updateCombatItemInspectInput(const Vector2 mousePosition) {
    if (!combatItemInspectOpen()) {
        return;
    }

    if (inspectedRelicIndex_.has_value()) {
        if (view_.model().relics.empty()) {
            closeCombatItemInspect();
            return;
        }

        if (*inspectedRelicIndex_ >= view_.model().relics.size()) {
            inspectedRelicIndex_ = view_.model().relics.size() - 1;
        }
    }

    if (inspectedConsumableIndex_.has_value()) {
        const std::size_t count = view_.model().consumables.size();
        if (*inspectedConsumableIndex_ >= count || !view_.model().consumables[*inspectedConsumableIndex_].filled) {
            const std::optional<std::size_t> next = nextFilledConsumableIndex(0, 1);
            if (!next.has_value()) {
                closeCombatItemInspect();
                return;
            }
            inspectedConsumableIndex_ = *next;
        }
    }

    const Rectangle modal = combatItemInspectModalBounds();

    if (IsKeyPressed(KEY_ESCAPE)) {
        closeCombatItemInspect();
        return;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        inspectPreviousItem();
        return;
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        inspectNextItem();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (BasicUi::contains(combatItemInspectCloseButtonBounds(modal), mousePosition)) {
            closeCombatItemInspect();
            return;
        }

        if (BasicUi::contains(combatItemInspectPreviousButtonBounds(modal), mousePosition)) {
            inspectPreviousItem();
            return;
        }

        if (BasicUi::contains(combatItemInspectNextButtonBounds(modal), mousePosition)) {
            inspectNextItem();
            return;
        }

        if (!BasicUi::contains(modal, mousePosition)) {
            closeCombatItemInspect();
        }
    }
}

void CombatScene::renderCombatItemInspectModal() const {
    if (!combatItemInspectOpen()) {
        return;
    }

    InspectPanelModel panel;
    std::size_t itemIndex = 0;
    std::size_t itemCount = 0;

    if (inspectedRelicIndex_.has_value()) {
        itemIndex = *inspectedRelicIndex_;
        itemCount = view_.model().relics.size();
        if (itemIndex >= itemCount) {
            return;
        }

        const RelicViewModel& relicModel = view_.model().relics[itemIndex];
        if (content_.relics().contains(RelicId(relicModel.id))) {
            panel = inspectModelBuilder_.buildRelic(content_.relics().get(RelicId(relicModel.id)));
        } else {
            panel.header = relicModel.name;
            panel.subheader = relicModel.description;
        }
    } else if (inspectedConsumableIndex_.has_value()) {
        itemIndex = *inspectedConsumableIndex_;
        itemCount = view_.model().consumables.size();
        if (itemIndex >= itemCount || !view_.model().consumables[itemIndex].filled) {
            return;
        }

        const ConsumableViewModel& consumableModel = view_.model().consumables[itemIndex];
        if (content_.consumables().contains(ConsumableId(consumableModel.id))) {
            panel = inspectModelBuilder_.buildConsumable(content_.consumables().get(ConsumableId(consumableModel.id)));
        } else {
            panel = inspectModelBuilder_.buildConsumable(consumableModel);
        }
    }

    const Rectangle modal = combatItemInspectModalBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 145});
    DrawRectangleRounded(modal, 0.045f, 14, Color{18, 20, 28, 250});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    const Rectangle panelBounds{
        modal.x + 24.f,
        modal.y + 24.f,
        modal.width - 48.f,
        modal.height - 104.f
    };
    inspectPanelView_.render(uiFont_, panel, panelBounds);

    const Rectangle previous = combatItemInspectPreviousButtonBounds(modal);
    const Rectangle next = combatItemInspectNextButtonBounds(modal);
    BasicUi::drawButton(uiFont_, previous, "<", mouse);
    BasicUi::drawButton(uiFont_, next, ">", mouse);
    BasicUi::drawButton(uiFont_, combatItemInspectCloseButtonBounds(modal), localizedOrFallback(TextId("ui.close"), "Close"), mouse);

    const std::string counter = itemCount > 0
        ? std::to_string(itemIndex + 1) + "/" + std::to_string(itemCount)
        : "0/0";
    BasicUi::drawCenteredText(
        uiFont_,
        counter,
        Rectangle{previous.x + previous.width + 8.f, previous.y, next.x - previous.x - previous.width - 16.f, previous.height},
        16.f,
        Color{185, 190, 205, 255}
    );
}

Rectangle CombatScene::combatItemInspectModalBounds() const {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());
    const float width = std::min(620.f, screenWidth - 80.f);
    const float height = std::min(560.f, screenHeight - 80.f);
    return Rectangle{
        (screenWidth - width) * 0.5f,
        (screenHeight - height) * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::combatItemInspectCloseButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 136.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle CombatScene::combatItemInspectPreviousButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 24.f, modal.y + modal.height - 58.f, 52.f, 40.f};
}

Rectangle CombatScene::combatItemInspectNextButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 86.f, modal.y + modal.height - 58.f, 52.f, 40.f};
}

void CombatScene::inspectPreviousItem() {
    if (inspectedRelicIndex_.has_value()) {
        const std::size_t count = view_.model().relics.size();
        if (count == 0) {
            closeCombatItemInspect();
            return;
        }

        inspectedRelicIndex_ = *inspectedRelicIndex_ == 0 ? count - 1 : *inspectedRelicIndex_ - 1;
        return;
    }

    if (inspectedConsumableIndex_.has_value()) {
        const std::optional<std::size_t> previous = nextFilledConsumableIndex(*inspectedConsumableIndex_, -1);
        if (previous.has_value()) {
            inspectedConsumableIndex_ = *previous;
        }
    }
}

void CombatScene::inspectNextItem() {
    if (inspectedRelicIndex_.has_value()) {
        const std::size_t count = view_.model().relics.size();
        if (count == 0) {
            closeCombatItemInspect();
            return;
        }

        inspectedRelicIndex_ = (*inspectedRelicIndex_ + 1) % count;
        return;
    }

    if (inspectedConsumableIndex_.has_value()) {
        const std::optional<std::size_t> next = nextFilledConsumableIndex(*inspectedConsumableIndex_, 1);
        if (next.has_value()) {
            inspectedConsumableIndex_ = *next;
        }
    }
}

std::optional<std::size_t> CombatScene::nextFilledConsumableIndex(const std::size_t start, const int direction) const {
    const std::size_t count = view_.model().consumables.size();
    if (count == 0) {
        return std::nullopt;
    }

    const int step = direction < 0 ? -1 : 1;
    std::size_t index = start % count;
    for (std::size_t visited = 0; visited < count; ++visited) {
        index = static_cast<std::size_t>((static_cast<int>(index) + step + static_cast<int>(count)) % static_cast<int>(count));
        if (view_.model().consumables[index].filled) {
            return index;
        }
    }

    return std::nullopt;
}

void CombatScene::openConsumableConfirmation(const std::size_t index) {
    if (index >= combatConsumableIds_.size()) {
        pendingConsumableIndex_.reset();
        targetingConsumableIndex_.reset();
        return;
    }

    pendingConsumableIndex_ = index;
    targetingConsumableIndex_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    lastPreviewTarget_.reset();
}

void CombatScene::cancelConsumableConfirmation() {
    pendingConsumableIndex_.reset();
}

void CombatScene::confirmConsumableUse() {
    if (!pendingConsumableIndex_.has_value()) {
        return;
    }

    const std::size_t index = *pendingConsumableIndex_;
    pendingConsumableIndex_.reset();

    if (consumableRequiresTarget(index)) {
        startConsumableTargeting(index);
        return;
    }

    tryUseConsumable(index, std::nullopt);
}

void CombatScene::startConsumableTargeting(const std::size_t index) {
    if (index >= combatConsumableIds_.size()) {
        targetingConsumableIndex_.reset();
        return;
    }

    targetingConsumableIndex_ = index;
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    inspectedPileCardIndex_.reset();
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

void CombatScene::cancelConsumableTargeting() {
    targetingConsumableIndex_.reset();
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

void CombatScene::tryUseConsumable(const std::size_t index, const std::optional<EntityId> target) {
    if (index >= combatConsumableIds_.size()) {
        return;
    }

    const std::string consumableId = combatConsumableIds_[index];
    const std::optional<EntityId> explicitTarget = target.has_value() ? target : std::optional<EntityId>{primaryPlayerId()};
    if (consumableSystem_.useConsumable(
            state_,
            consumableId,
            primaryPlayerId(),
            explicitTarget,
            effectSystem_,
            random_
        )) {
        combatConsumableIds_.erase(combatConsumableIds_.begin() + static_cast<std::ptrdiff_t>(index));
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        targetingConsumableIndex_.reset();
        lastPreviewTarget_.reset();
        finalResult_ = combatController_.updateAfterAction(state_);
        if (finalResult_.outcome == CombatOutcome::Ongoing) {
            turnSystem_.refreshEnemyIntentValues(state_);
        }
        viewModelDirty_ = true;
    }
}

void CombatScene::updateConsumableConfirmationInput(const Vector2 mousePosition) {
    if (!pendingConsumableIndex_.has_value() || *pendingConsumableIndex_ >= combatConsumableIds_.size()) {
        cancelConsumableConfirmation();
        return;
    }

    const Rectangle modal = consumableConfirmationBounds();

    if (IsKeyPressed(KEY_ESCAPE)) {
        cancelConsumableConfirmation();
        return;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        confirmConsumableUse();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    if (BasicUi::contains(consumableConfirmButtonBounds(modal), mousePosition)) {
        confirmConsumableUse();
        return;
    }

    if (BasicUi::contains(consumableCancelButtonBounds(modal), mousePosition) ||
        !BasicUi::contains(modal, mousePosition)) {
        cancelConsumableConfirmation();
        return;
    }
}

void CombatScene::updateConsumableTargetingInput(const Vector2) {
    if (!targetingConsumableIndex_.has_value() || *targetingConsumableIndex_ >= combatConsumableIds_.size()) {
        cancelConsumableTargeting();
        return;
    }

    const std::optional<EntityId> previewTarget = previewTargetForConsumable(*targetingConsumableIndex_);
    if (previewTarget != lastPreviewTarget_) {
        lastPreviewTarget_ = previewTarget;
        viewModelDirty_ = true;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        cancelConsumableTargeting();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (previewTarget.has_value()) {
            tryUseConsumable(*targetingConsumableIndex_, previewTarget);
        }
        return;
    }
}

void CombatScene::renderConsumableConfirmationModal() const {
    if (!pendingConsumableIndex_.has_value() || *pendingConsumableIndex_ >= combatConsumableIds_.size()) {
        return;
    }

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 120});

    const std::size_t index = *pendingConsumableIndex_;
    const std::string consumableId = combatConsumableIds_[index];
    std::string name = consumableId;
    std::string description;
    if (content_.consumables().contains(ConsumableId(consumableId))) {
        const ConsumableDefinition& definition = content_.consumables().get(ConsumableId(consumableId));
        name = localizedOrFallback(definition.nameTextId, consumableId);
        description = localizedOrFallback(definition.descriptionTextId, {});
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = consumableConfirmationBounds();
    DrawRectangleRounded(modal, 0.045f, 14, Color{28, 30, 40, 250});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("consumable.confirm.title"), "Use consumable?"),
        Rectangle{modal.x + 28.f, modal.y + 24.f, modal.width - 56.f, 36.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    BasicUi::drawCenteredText(
        uiFont_,
        name,
        Rectangle{modal.x + 36.f, modal.y + 74.f, modal.width - 72.f, 32.f},
        24.f,
        Color{245, 245, 250, 255}
    );

    const std::string fallbackDescription = consumableRequiresTarget(index)
        ? localizedOrFallback(TextId("consumable.confirm.target_description"), "Confirm, then choose a highlighted target.")
        : localizedOrFallback(TextId("consumable.confirm.description"), "This will consume the item immediately.");

    const std::vector<std::string> lines = BasicUi::wrapText(
        uiFont_,
        description.empty() ? fallbackDescription : description + "\n" + fallbackDescription,
        18.f,
        modal.width - 72.f
    );

    float y = modal.y + 124.f;
    for (const std::string& line : lines) {
        if (y > modal.y + modal.height - 110.f) {
            break;
        }
        BasicUi::drawText(uiFont_, line, Vector2{modal.x + 36.f, y}, 18.f, Color{205, 210, 225, 255});
        y += 24.f;
    }

    BasicUi::drawButton(
        uiFont_,
        consumableCancelButtonBounds(modal),
        localizedOrFallback(TextId("ui.cancel"), "Cancel"),
        mouse
    );

    BasicUi::drawButton(
        uiFont_,
        consumableConfirmButtonBounds(modal),
        consumableRequiresTarget(index)
            ? localizedOrFallback(TextId("consumable.confirm.choose_target"), "Choose target")
            : localizedOrFallback(TextId("ui.confirm"), "Confirm"),
        mouse
    );
}

bool CombatScene::consumableRequiresTarget(const std::size_t index) const {
    return consumableCanTargetEnemy(index) || consumableCanTargetPlayer(index);
}

bool CombatScene::consumableCanTargetEnemy(const std::size_t index) const {
    if (index >= combatConsumableIds_.size()) {
        return false;
    }

    const ConsumableId id(combatConsumableIds_[index]);
    if (!content_.consumables().contains(id)) {
        return false;
    }

    const ConsumableDefinition& definition = content_.consumables().get(id);
    return std::any_of(definition.effects.begin(), definition.effects.end(), [](const EffectDefinition& effect) {
        return effect.target == EffectTarget::SingleEnemy;
    });
}

bool CombatScene::consumableCanTargetPlayer(const std::size_t index) const {
    if (index >= combatConsumableIds_.size()) {
        return false;
    }

    const ConsumableId id(combatConsumableIds_[index]);
    if (!content_.consumables().contains(id)) {
        return false;
    }

    const ConsumableDefinition& definition = content_.consumables().get(id);
    return std::any_of(definition.effects.begin(), definition.effects.end(), [](const EffectDefinition& effect) {
        return effect.target == EffectTarget::Ally;
    });
}

std::vector<EntityId> CombatScene::targetCandidatesForConsumable(const std::size_t index) const {
    std::vector<EntityId> candidates;

    if (consumableCanTargetEnemy(index)) {
        const std::vector<EntityId> enemies = state_.aliveEnemyIds();
        candidates.insert(candidates.end(), enemies.begin(), enemies.end());
    }

    if (consumableCanTargetPlayer(index)) {
        const std::vector<EntityId> players = state_.alivePlayerIds();
        candidates.insert(candidates.end(), players.begin(), players.end());
    }

    return candidates;
}

std::optional<EntityId> CombatScene::previewTargetForConsumable(const std::size_t index) const {
    const std::vector<EntityId> candidates = targetCandidatesForConsumable(index);
    if (candidates.empty()) {
        return std::nullopt;
    }

    const auto isCandidate = [&candidates](const EntityId id) {
        return std::find(candidates.begin(), candidates.end(), id) != candidates.end();
    };

    if (view_.hoveredEnemyId().has_value() && isCandidate(*view_.hoveredEnemyId())) {
        return view_.hoveredEnemyId();
    }

    if (view_.hoveredPlayerId().has_value() && isCandidate(*view_.hoveredPlayerId())) {
        return view_.hoveredPlayerId();
    }

    return std::nullopt;
}

Rectangle CombatScene::consumableConfirmationBounds() const {
    const float width = std::min(560.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = 340.f;
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::consumableConfirmButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f + 18.f, modal.y + modal.height - 70.f, 190.f, 48.f};
}

Rectangle CombatScene::consumableCancelButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f - 208.f, modal.y + modal.height - 70.f, 190.f, 48.f};
}


