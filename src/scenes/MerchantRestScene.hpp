#pragma once

#include "cards/CardId.hpp"
#include "data/CardDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "shop/ShopState.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <raylib.h>

class MerchantRestScene final : public Scene {
public:
    MerchantRestScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const CardDatabase& cards,
        const RunState& runState,
        ShopState shopState,
        std::function<void(ShopState)> onBuyCards,
        std::function<void()> onHeal,
        std::function<void(std::size_t)> onUpgrade,
        std::function<void()> onSkip
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    enum class Mode {
        Actions,
        UpgradeList
    };

    Rectangle panelBounds() const;
    Rectangle actionButtonBounds(std::size_t index) const;
    Rectangle skipButtonBounds() const;
    Rectangle upgradeListBounds() const;
    Rectangle upgradeBackButtonBounds(Rectangle panel) const;
    Rectangle upgradeRowBounds(Rectangle list, std::size_t rowIndex) const;
    Rectangle upgradePreviewBounds(Rectangle panel) const;
    Rectangle upgradeConfirmButtonBounds(Rectangle panel) const;
    Rectangle upgradeCancelButtonBounds(Rectangle panel) const;

    void updateActions(Vector2 mousePosition);
    void updateUpgradeList(Vector2 mousePosition);

    void renderActions() const;
    void renderUpgradeList() const;
    void renderUpgradePreview(Rectangle panel) const;

    bool isDeckIndexUpgraded(std::size_t deckIndex) const;
    bool canUpgradeDeckIndex(std::size_t deckIndex) const;
    std::vector<std::size_t> upgradableDeckIndices() const;
    std::string cardName(const CardId& cardId) const;
    std::string cardUpgradeSummary(const CardId& cardId) const;
    std::string healPreviewText() const;
    std::string stressPreviewText() const;
    std::string buyCardsSummary() const;
    bool canOpenCardShop() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RunState& runState_;
    ShopState shopState_;
    std::function<void(ShopState)> onBuyCards_;
    std::function<void()> onHeal_;
    std::function<void(std::size_t)> onUpgrade_;
    std::function<void()> onSkip_;

    Mode mode_ = Mode::Actions;
    float upgradeScrollOffset_ = 0.f;
    std::optional<std::size_t> selectedUpgradeDeckIndex_;
};
