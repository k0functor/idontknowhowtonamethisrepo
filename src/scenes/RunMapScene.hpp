#pragma once

#include <cstddef>

#include "cards/CardId.hpp"
#include "cards/CardKeyword.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "consumables/ConsumableRarity.hpp"
#include "data/CardDatabase.hpp"
#include "effects/EffectDefinition.hpp"
#include "effects/EffectValue.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicModifierDefinition.hpp"
#include "relics/RelicRarity.hpp"
#include "relics/RelicTriggerDefinition.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <raylib.h>

class RunMapScene final : public Scene {
public:
    RunMapScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const RunState& runState,
        std::function<void(int)> onNodeSelected,
        std::function<void(int)> onRestHeal,
        std::function<void(int, std::size_t)> onRestUpgrade,
        std::function<void(int)> onRestSkip,
        std::function<void()> onBackToHub
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    enum class OverlayMode {
        None,
        Deck,
        Relics,
        Consumables,
        Upgrade
    };

    Vector2 nodeScreenPosition(const RunMapNode& node) const;
    Rectangle nodeBounds(const RunMapNode& node) const;
    Rectangle restModalBounds() const;
    Rectangle restHealButtonBounds(Rectangle modal) const;
    Rectangle restUpgradeButtonBounds(Rectangle modal) const;
    Rectangle restSkipButtonBounds(Rectangle modal) const;
    Rectangle restCancelButtonBounds(Rectangle modal) const;
    Rectangle deckButtonBounds() const;
    Rectangle relicsButtonBounds() const;
    Rectangle consumablesButtonBounds() const;
    Rectangle overlayBounds() const;
    Rectangle overlayCloseButtonBounds(Rectangle modal) const;
    Rectangle overlayGridBounds(Rectangle modal) const;
    Rectangle upgradePreviewModalBounds() const;
    Rectangle upgradePreviewBeforeCardBounds(Rectangle modal) const;
    Rectangle upgradePreviewAfterCardBounds(Rectangle modal) const;
    Rectangle upgradePreviewCancelButtonBounds(Rectangle modal) const;
    Rectangle upgradePreviewConfirmButtonBounds(Rectangle modal) const;

    const RunMapNode* hoveredMapNode(Vector2 mousePosition) const;
    const RunMapNode* currentMapNode() const;
    bool isMapInteractionBlocked() const;
    bool isSelectableMapNode(const RunMapNode& node) const;
    bool isPastLockedAlternative(const RunMapNode& node) const;
    bool isNextAvailableConnection(const RunMapNode& from, const RunMapNode& to) const;
    bool isChosenPathConnection(const RunMapNode& from, const RunMapNode& to) const;
    Color connectionColor(const RunMapNode& from, const RunMapNode& to) const;
    float connectionThickness(const RunMapNode& from, const RunMapNode& to) const;
    Color nodeColor(const RunMapNode& node) const;
    Color nodeOutlineColor(const RunMapNode& node) const;
    Color nodeTextColor(const RunMapNode& node) const;
    float nodeOutlineThickness(const RunMapNode& node) const;
    std::string nodeLabel(const RunMapNode& node) const;
    void renderMapLegend() const;

    void updateRestModal(Vector2 mousePosition);
    void renderRestModal() const;
    std::string restHealPreviewText() const;
    std::string restStressPreviewText() const;
    std::string runHpSummaryText() const;
    std::string runStressSummaryText() const;

    void openOverlay(OverlayMode mode);
    void closeOverlay();
    void updateOverlay(Vector2 mousePosition);
    void updateUpgradePreviewModal(Vector2 mousePosition);
    void renderOverlay() const;
    std::optional<std::size_t> hoveredOverlayDeckIndex(Vector2 mousePosition) const;
    std::optional<std::string> hoveredOverlayRelicId(Vector2 mousePosition) const;
    std::optional<std::string> hoveredOverlayConsumableId(Vector2 mousePosition) const;
    void renderCardInspectModal() const;
    void renderRelicInspectModal() const;
    void renderConsumableInspectModal() const;
    Rectangle cardInspectModalBounds() const;
    Rectangle cardInspectCloseButtonBounds(Rectangle modal) const;
    Rectangle cardInspectPreviousButtonBounds(Rectangle modal) const;
    Rectangle cardInspectNextButtonBounds(Rectangle modal) const;
    std::string cardInspectEffectText(const EffectDefinition& effect) const;
    std::string cardInspectKeywordName(CardKeyword keyword) const;
    std::string cardInspectKeywordDescription(CardKeyword keyword) const;
    std::string cardInspectValueText(const EffectValue& value) const;
    std::string localizedOrFallback(const TextId& textId, const std::string& fallback) const;
    void renderDeckOverlay(Rectangle modal) const;
    void renderUpgradeOverlay(Rectangle modal) const;
    void renderUpgradePreviewModal() const;
    void renderRelicsOverlay(Rectangle modal) const;
    void renderConsumablesOverlay(Rectangle modal) const;
    void renderOverlayFooterHint(Rectangle modal, const std::string& countText) const;
    void renderInspectModalControls(Rectangle modal, std::size_t itemIndex, std::size_t itemCount) const;
    std::size_t inspectedOverlayItemIndex() const;
    std::size_t inspectedOverlayItemCount() const;
    void inspectPreviousOverlayItem();
    void inspectNextOverlayItem();
    void renderCardGrid(Rectangle grid, const std::vector<std::size_t>& deckIndices, bool selectionMode) const;
    Rectangle cardGridCellBounds(Rectangle grid, std::size_t index, float scrollOffset) const;
    float cardGridMaxScroll(Rectangle grid, std::size_t count) const;
    Rectangle listRowBounds(Rectangle area, std::size_t index, float scrollOffset) const;
    float listMaxScroll(Rectangle area, std::size_t count) const;
    bool isDeckCardUpgraded(std::size_t deckIndex) const;
    bool canUpgradeDeckIndex(std::size_t deckIndex) const;
    std::vector<std::size_t> upgradableDeckIndices() const;
    std::string cardName(const CardId& cardId, bool upgraded) const;
    std::string cardDescription(const CardId& cardId, bool upgraded) const;
    std::string relicRarityText(RelicRarity rarity) const;
    std::string consumableRarityText(ConsumableRarity rarity) const;
    std::string relicModifierText(const RelicModifierDefinition& modifier) const;
    std::string relicTriggerText(const RelicTriggerDefinition& trigger) const;
    std::string overlayTitle() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    const ConsumableDatabase& consumables_;
    const RunState& runState_;
    std::function<void(int)> onNodeSelected_;
    std::function<void(int)> onRestHeal_;
    std::function<void(int, std::size_t)> onRestUpgrade_;
    std::function<void(int)> onRestSkip_;
    std::function<void()> onBackToHub_;

    std::optional<int> restModalNodeId_;
    OverlayMode overlayMode_ = OverlayMode::None;
    float overlayScrollOffset_ = 0.f;
    std::optional<std::size_t> selectedUpgradeDeckIndex_;
    std::optional<std::size_t> inspectedCardDeckIndex_;
    std::optional<std::string> inspectedRelicId_;
    std::optional<std::string> inspectedConsumableId_;
};
