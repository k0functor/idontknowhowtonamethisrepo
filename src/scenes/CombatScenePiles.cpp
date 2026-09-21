#include "CombatScene.hpp"
#include "CombatSceneLayout.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardUpgrade.hpp"
#include "ui/BasicUi.hpp"
#include "ui/CardVisualInstance.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

const std::vector<CardInstance>& CombatScene::activePileCards() const {
    switch (pileOverlayMode_) {
        case PileOverlayMode::DrawPile:
            return state_.deck.drawPile.cards();
        case PileOverlayMode::DiscardPile:
            return state_.deck.discardPile.cards();
        case PileOverlayMode::ExhaustPile:
            return state_.deck.exhaustPile.cards();
        case PileOverlayMode::None:
            break;
    }

    return state_.deck.drawPile.cards();
}

std::string CombatScene::activePileTitle() const {
    switch (pileOverlayMode_) {
        case PileOverlayMode::DrawPile:
            return localizedOrFallback(TextId("ui.draw_pile_view_title"), "Draw pile");
        case PileOverlayMode::DiscardPile:
            return localizedOrFallback(TextId("ui.discard_pile_view_title"), "Discard pile");
        case PileOverlayMode::ExhaustPile:
            return localizedOrFallback(TextId("ui.exhaust_pile_view_title"), "Burned cards");
        case PileOverlayMode::None:
            break;
    }

    return {};
}

void CombatScene::openPileOverlay(const PileOverlayMode mode) {
    pileOverlayMode_ = mode;
    pileOverlayScrollOffset_ = 0.f;
    inspectedPileCardIndex_.reset();
    clearCardSelection();
}

void CombatScene::closePileOverlay() {
    pileOverlayMode_ = PileOverlayMode::None;
    pileOverlayScrollOffset_ = 0.f;
    inspectedPileCardIndex_.reset();
}

void CombatScene::updatePileOverlay(const Vector2 mousePosition) {
    const Rectangle modal = CombatSceneLayout::pileOverlayBounds();
    const Rectangle grid = CombatSceneLayout::pileOverlayGridBounds(modal);
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.f) {
        pileOverlayScrollOffset_ = std::clamp(
            pileOverlayScrollOffset_ - wheel * 76.f,
            0.f,
            CombatSceneLayout::pileOverlayMaxScroll(grid, activePileCards().size())
        );
    }

    if (inspectedPileCardIndex_.has_value() && IsKeyPressed(KEY_ESCAPE)) {
        inspectedPileCardIndex_.reset();
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(CombatSceneLayout::pileOverlayCloseButtonBounds(modal), mousePosition))) {
        closePileOverlay();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_I)) {
        inspectedPileCardIndex_ = hoveredPileCardIndex(mousePosition);
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const std::optional<std::size_t> hovered = hoveredPileCardIndex(mousePosition);
        if (hovered.has_value()) {
            inspectedPileCardIndex_ = *hovered;
            return;
        }

        if (inspectedPileCardIndex_.has_value()) {
            inspectedPileCardIndex_.reset();
        }
    }
}

void CombatScene::renderEnergyBubble() const {
    if (state_.resources.hasAnyActorEnergyPool()) {
        return;
    }

    const Rectangle bounds = CombatSceneLayout::energyBubbleBounds(view_.playerBounds(primaryPlayerId()));
    const int centerX = static_cast<int>(bounds.x + bounds.width * 0.5f);
    const int centerY = static_cast<int>(bounds.y + bounds.height * 0.5f);
    const float radius = bounds.width * 0.5f;

    DrawCircle(centerX, centerY, radius, Color{54, 48, 78, 245});
    DrawCircleLines(centerX, centerY, radius, Color{190, 170, 245, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        std::to_string(state_.resources.energy()) + " / " + std::to_string(state_.resources.maxEnergy()),
        bounds,
        19.f,
        Color{246, 240, 255, 255}
    );
}

void CombatScene::renderPileButtons() const {
    const Vector2 mouse = GetMousePosition();
    renderEnergyBubble();
    BasicUi::drawButton(
        uiFont_,
        CombatSceneLayout::drawPileButtonBounds(),
        localizedOrFallback(TextId("ui.draw_pile"), "Draw") + ": " + std::to_string(state_.deck.drawPile.size()),
        mouse
    );
    BasicUi::drawButton(
        uiFont_,
        CombatSceneLayout::discardPileButtonBounds(),
        localizedOrFallback(TextId("ui.discard_pile"), "Discard") + ": " + std::to_string(state_.deck.discardPile.size()),
        mouse
    );
    BasicUi::drawButton(
        uiFont_,
        CombatSceneLayout::exhaustPileButtonBounds(),
        localizedOrFallback(TextId("ui.exhaust_pile"), "Burned") + ": " + std::to_string(state_.deck.exhaustPile.size()),
        mouse
    );
}

void CombatScene::renderPileOverlay() const {
    const Rectangle modal = CombatSceneLayout::pileOverlayBounds();
    const Rectangle grid = CombatSceneLayout::pileOverlayGridBounds(modal);
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 165});
    DrawRectangleRounded(modal, 0.04f, 16, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(modal, 0.04f, 16, 3.f, Color{238, 196, 86, 255});
    BasicUi::drawCenteredText(uiFont_, activePileTitle(), Rectangle{modal.x + 24.f, modal.y + 20.f, modal.width - 48.f, 38.f}, 30.f, Color{255, 235, 175, 255});

    DrawRectangleRounded(grid, 0.02f, 8, Color{20, 22, 30, 255});

    const std::vector<CardInstance>& cards = activePileCards();
    if (cards.empty()) {
        BasicUi::drawCenteredText(uiFont_, localizedOrFallback(TextId("ui.empty"), "Empty"), grid, 24.f, Color{205, 210, 225, 255});
    } else {
        BeginScissorMode(static_cast<int>(grid.x), static_cast<int>(grid.y), static_cast<int>(grid.width), static_cast<int>(grid.height));
        for (std::size_t i = 0; i < cards.size(); ++i) {
            const Rectangle cell = CombatSceneLayout::pileOverlayCardBounds(grid, i, pileOverlayScrollOffset_);
            if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
                continue;
            }
            renderPileCard(cards[i], cell);
        }
        EndScissorMode();
    }

    BasicUi::drawText(
        uiFont_,
        localizedOrFallback(TextId("ui.pile_inspect_hint"), "Left click, right click, or I: inspect card. Esc closes the top window."),
        Vector2{modal.x + 32.f, modal.y + modal.height - 48.f},
        15.f,
        Color{150, 160, 185, 255}
    );
    BasicUi::drawButton(uiFont_, CombatSceneLayout::pileOverlayCloseButtonBounds(modal), localizedOrFallback(TextId("ui.close"), "Close"), mouse);
    renderPileCardInspectPanel();
}

void CombatScene::renderPileCard(const CardInstance& card, const Rectangle bounds) const {
    CardViewModel model = cardViewModelForInstance(card);
    const bool hovered = BasicUi::contains(bounds, GetMousePosition());
    model.selected = hovered;
    const CardTransform transform = CardVisualInstance::transformForStandardSlot(bounds, 0);
    CardVisualInstance::renderStatic(model, uiFont_.available() ? &uiFont_.font() : nullptr, transform);
}



std::optional<std::size_t> CombatScene::hoveredPileCardIndex(const Vector2 mousePosition) const {
    if (pileOverlayMode_ == PileOverlayMode::None) {
        return std::nullopt;
    }

    const Rectangle grid = CombatSceneLayout::pileOverlayGridBounds(CombatSceneLayout::pileOverlayBounds());
    const std::vector<CardInstance>& cards = activePileCards();
    for (std::size_t i = 0; i < cards.size(); ++i) {
        const Rectangle cell = CombatSceneLayout::pileOverlayCardBounds(grid, i, pileOverlayScrollOffset_);
        if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
            continue;
        }

        if (BasicUi::contains(cell, mousePosition)) {
            return i;
        }
    }

    return std::nullopt;
}

void CombatScene::renderPileCardInspectPanel() const {
    if (!inspectedPileCardIndex_.has_value()) {
        return;
    }

    const std::vector<CardInstance>& cards = activePileCards();
    if (*inspectedPileCardIndex_ >= cards.size()) {
        return;
    }

    const CardInstance& instance = cards[*inspectedPileCardIndex_];
    if (!content_.cards().contains(instance.definitionId)) {
        return;
    }

    const CardViewModel cardModel = cardViewModelForInstance(instance);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);
    const InspectPanelModel panel = inspectModelBuilder_.buildCard(definition, cardModel);

    const Rectangle modal = CombatSceneLayout::pileOverlayBounds();
    const float width = std::min(540.f, modal.width - 80.f);
    const Rectangle bounds{
        modal.x + modal.width - width - 32.f,
        modal.y + 88.f,
        width,
        std::min(660.f, modal.height - 150.f)
    };

    DrawRectangleRounded(bounds, 0.055f, 10, Color{16, 18, 24, 245});
    inspectPanelView_.render(uiFont_, panel, bounds);
}
