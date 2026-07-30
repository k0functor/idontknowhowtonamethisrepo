#pragma once

#include "cards/CardDefinition.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "enemies/EnemyDefinition.hpp"
#include "localization/LocalizationManager.hpp"
#include "profile/ProfileData.hpp"
#include "relics/RelicDefinition.hpp"
#include "scenes/Scene.hpp"
#include "statuses/StatusDefinition.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <string>
#include <vector>

class CompendiumScene final : public Scene {
public:
    CompendiumScene(
        const UiFont& font,
        const LocalizationManager& localization,
        std::vector<const CardDefinition*> cards,
        std::vector<const RelicDefinition*> relics,
        std::vector<const EnemyDefinition*> enemies,
        std::vector<const StatusDefinition*> statuses,
        std::vector<const ConsumableDefinition*> consumables,
        const ProfileData* profile,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

    enum class Tab {
        Cards,
        Relics,
        Enemies,
        Statuses,
        Consumables
    };

    enum class Filter {
        All,
        Known,
        Unknown
    };

    enum class SortMode {
        Name,
        Known,
        Category
    };

private:
    void moveTab(int direction);
    void moveSelection(int direction);
    void moveFilter(int direction);
    void moveSort(int direction);
    void updateSearchInput();
    std::size_t currentItemCount() const;
    std::size_t totalItemCount(Tab tab) const;
    std::size_t knownItemCount(Tab tab) const;
    std::vector<std::size_t> currentFilteredIndices() const;
    std::size_t resolvedSelectedIndex() const;
    bool itemKnownAt(Tab tab, std::size_t rawIndex) const;
    std::string tabTitle(Tab tab) const;
    std::string filterTitle(Filter filter) const;
    std::string sortTitle(SortMode sortMode) const;
    std::string progressText() const;
    std::string selectedItemName() const;
    std::string selectedItemSubtitle() const;
    bool selectedItemKnown() const;
    bool isKnownCard(const CardDefinition& card) const;
    bool isKnownRelic(const RelicDefinition& relic) const;
    bool isKnownEnemy(const EnemyDefinition& enemy) const;
    bool isKnownStatus(const StatusDefinition& status) const;
    bool isKnownConsumable(const ConsumableDefinition& consumable) const;
    bool itemMatchesSearch(Tab tab, std::size_t rawIndex) const;
    std::string itemSearchText(Tab tab, std::size_t rawIndex) const;
    std::string itemSortName(Tab tab, std::size_t rawIndex) const;
    int itemSortCategoryRank(Tab tab, std::size_t rawIndex) const;

    void renderList(Rectangle listPanel, Vector2 mouse) const;
    void renderDetails(Rectangle detailsPanel) const;
    void renderCardDetails(Rectangle panel, const CardDefinition& card, bool known) const;
    void renderRelicDetails(Rectangle panel, const RelicDefinition& relic, bool known) const;
    void renderEnemyDetails(Rectangle panel, const EnemyDefinition& enemy, bool known) const;
    void renderStatusDetails(Rectangle panel, const StatusDefinition& status, bool known) const;
    void renderConsumableDetails(Rectangle panel, const ConsumableDefinition& consumable, bool known) const;

    std::string localizedEnum(const std::string& category, const std::string& value) const;
    static bool containsId(const std::vector<std::string>& ids, const std::string& id);

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    std::vector<const CardDefinition*> cards_;
    std::vector<const RelicDefinition*> relics_;
    std::vector<const EnemyDefinition*> enemies_;
    std::vector<const StatusDefinition*> statuses_;
    std::vector<const ConsumableDefinition*> consumables_;
    const ProfileData* profile_ = nullptr;
    std::function<void()> onBack_;

    Tab tab_ = Tab::Cards;
    Filter filter_ = Filter::All;
    SortMode sortMode_ = SortMode::Name;
    std::string searchQuery_;
    bool searchFocused_ = false;
    std::size_t selectedIndex_ = 0;
};
