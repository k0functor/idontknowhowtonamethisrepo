#include "CompendiumScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "consumables/ConsumableRarity.hpp"
#include "relics/RelicRarity.hpp"
#include "statuses/StatusDurationRule.hpp"
#include "statuses/StatusType.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cctype>
#include <string>
#include <utility>

#include <raylib.h>

namespace {
struct CompendiumSceneLayout {
    Rectangle backButton{};
    Rectangle title{};
    Rectangle subtitle{};
    std::array<Rectangle, 5> tabs{};
    std::array<Rectangle, 3> filters{};
    Rectangle searchBox{};
    Rectangle clearSearchButton{};
    Rectangle sortButton{};
    Rectangle progress{};
    Rectangle listPanel{};
    Rectangle detailsPanel{};
};

CompendiumSceneLayout calculateCompendiumSceneLayout() {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    CompendiumSceneLayout layout;
    layout.backButton = Rectangle{32.f, 32.f, 140.f, 48.f};
    layout.title = Rectangle{0.f, 54.f, screenWidth, 54.f};
    layout.subtitle = Rectangle{0.f, 106.f, screenWidth, 30.f};

    const float tabsY = 148.f;
    const float tabsGap = 10.f;
    const float tabsTotalWidth = std::min(screenWidth - 96.f, 980.f);
    const float tabWidth = (tabsTotalWidth - tabsGap * 4.f) / 5.f;
    const float tabsStartX = screenWidth * 0.5f - tabsTotalWidth * 0.5f;
    for (std::size_t i = 0; i < layout.tabs.size(); ++i) {
        layout.tabs[i] = Rectangle{tabsStartX + static_cast<float>(i) * (tabWidth + tabsGap), tabsY, tabWidth, 44.f};
    }

    const float filtersTotalWidth = std::min(screenWidth - 96.f, 520.f);
    const float filterWidth = (filtersTotalWidth - tabsGap * 2.f) / 3.f;
    const float filtersStartX = screenWidth * 0.5f - filtersTotalWidth * 0.5f;
    for (std::size_t i = 0; i < layout.filters.size(); ++i) {
        layout.filters[i] = Rectangle{filtersStartX + static_cast<float>(i) * (filterWidth + tabsGap), 204.f, filterWidth, 36.f};
    }

    const float controlsY = 252.f;
    const float controlsWidth = std::min(screenWidth - 128.f, 940.f);
    const float controlsStartX = screenWidth * 0.5f - controlsWidth * 0.5f;
    layout.searchBox = Rectangle{controlsStartX, controlsY, controlsWidth - 292.f, 38.f};
    layout.clearSearchButton = Rectangle{layout.searchBox.x + layout.searchBox.width + 10.f, controlsY, 92.f, 38.f};
    layout.sortButton = Rectangle{layout.clearSearchButton.x + layout.clearSearchButton.width + 10.f, controlsY, 180.f, 38.f};
    layout.progress = Rectangle{0.f, 296.f, screenWidth, 24.f};

    const float margin = 64.f;
    const float top = 332.f;
    const float gap = 24.f;
    const float height = std::max(350.f, screenHeight - top - 52.f);
    const float listWidth = std::clamp(screenWidth * 0.36f, 390.f, 540.f);
    layout.listPanel = Rectangle{margin, top, listWidth, height};
    layout.detailsPanel = Rectangle{layout.listPanel.x + layout.listPanel.width + gap, top, screenWidth - margin - (layout.listPanel.x + layout.listPanel.width + gap), height};
    return layout;
}

Rectangle itemRowBounds(const Rectangle listPanel, const int index) {
    constexpr float rowHeight = 68.f;
    constexpr float rowGap = 10.f;
    return Rectangle{
        listPanel.x + 18.f,
        listPanel.y + 18.f + static_cast<float>(index) * (rowHeight + rowGap),
        listPanel.width - 36.f,
        rowHeight
    };
}

std::size_t tabIndex(const CompendiumScene::Tab tab) {
    switch (tab) {
        case CompendiumScene::Tab::Cards: return 0u;
        case CompendiumScene::Tab::Relics: return 1u;
        case CompendiumScene::Tab::Enemies: return 2u;
        case CompendiumScene::Tab::Statuses: return 3u;
        case CompendiumScene::Tab::Consumables: return 4u;
    }

    return 0u;
}

CompendiumScene::Tab tabFromIndex(const std::size_t index) {
    switch (index) {
        case 0u: return CompendiumScene::Tab::Cards;
        case 1u: return CompendiumScene::Tab::Relics;
        case 2u: return CompendiumScene::Tab::Enemies;
        case 3u: return CompendiumScene::Tab::Statuses;
        case 4u: return CompendiumScene::Tab::Consumables;
        default: return CompendiumScene::Tab::Cards;
    }
}

std::size_t filterIndex(const CompendiumScene::Filter filter) {
    switch (filter) {
        case CompendiumScene::Filter::All: return 0u;
        case CompendiumScene::Filter::Known: return 1u;
        case CompendiumScene::Filter::Unknown: return 2u;
    }

    return 0u;
}

CompendiumScene::Filter filterFromIndex(const std::size_t index) {
    switch (index) {
        case 0u: return CompendiumScene::Filter::All;
        case 1u: return CompendiumScene::Filter::Known;
        case 2u: return CompendiumScene::Filter::Unknown;
        default: return CompendiumScene::Filter::All;
    }
}

std::size_t sortIndex(const CompendiumScene::SortMode sortMode) {
    switch (sortMode) {
        case CompendiumScene::SortMode::Name: return 0u;
        case CompendiumScene::SortMode::Known: return 1u;
        case CompendiumScene::SortMode::Category: return 2u;
    }

    return 0u;
}

CompendiumScene::SortMode sortFromIndex(const std::size_t index) {
    switch (index) {
        case 0u: return CompendiumScene::SortMode::Name;
        case 1u: return CompendiumScene::SortMode::Known;
        case 2u: return CompendiumScene::SortMode::Category;
        default: return CompendiumScene::SortMode::Name;
    }
}

void appendUtf8(std::string& text, const std::uint32_t codepoint) {
    if (codepoint <= 0x7Fu) {
        text.push_back(static_cast<char>(codepoint));
        return;
    }
    if (codepoint <= 0x7FFu) {
        text.push_back(static_cast<char>(0xC0u | (codepoint >> 6u)));
        text.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        return;
    }
    if (codepoint <= 0xFFFFu) {
        text.push_back(static_cast<char>(0xE0u | (codepoint >> 12u)));
        text.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
        text.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        return;
    }
    if (codepoint <= 0x10FFFFu) {
        text.push_back(static_cast<char>(0xF0u | (codepoint >> 18u)));
        text.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu)));
        text.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
        text.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
    }
}

void popUtf8(std::string& text) {
    if (text.empty()) {
        return;
    }

    std::size_t start = text.size() - 1u;
    while (start > 0u && (static_cast<unsigned char>(text[start]) & 0xC0u) == 0x80u) {
        --start;
    }
    text.erase(start);
}

std::string normalizeSearchText(std::string text) {
    for (char& character : text) {
        const unsigned char value = static_cast<unsigned char>(character);
        if (value < 128u) {
            character = static_cast<char>(std::tolower(value));
        }
    }

    return text;
}

void drawWrapped(
    const UiFont& font,
    const std::string& text,
    const float x,
    float& y,
    const float width,
    const float fontSize,
    const Color color
) {
    for (const std::string& line : BasicUi::wrapText(font, text, fontSize, width)) {
        BasicUi::drawText(font, line, Vector2{x, y}, fontSize, color);
        y += fontSize + 6.f;
    }
}

void drawField(
    const UiFont& font,
    const std::string& label,
    const std::string& value,
    const float x,
    float& y,
    const float width
) {
    BasicUi::drawText(font, label, Vector2{x, y}, 17.f, Color{236, 229, 198, 255});
    y += 24.f;
    drawWrapped(font, value.empty() ? "-" : value, x + 18.f, y, width - 18.f, 17.f, Color{214, 220, 238, 255});
    y += 12.f;
}
}

CompendiumScene::CompendiumScene(
    const UiFont& font,
    const LocalizationManager& localization,
    std::vector<const CardDefinition*> cards,
    std::vector<const RelicDefinition*> relics,
    std::vector<const EnemyDefinition*> enemies,
    std::vector<const StatusDefinition*> statuses,
    std::vector<const ConsumableDefinition*> consumables,
    const ProfileData* profile,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      cards_(std::move(cards)),
      relics_(std::move(relics)),
      enemies_(std::move(enemies)),
      statuses_(std::move(statuses)),
      consumables_(std::move(consumables)),
      profile_(profile),
      onBack_(std::move(onBack)) {
    const auto byName = [this](const auto* lhs, const auto* rhs) {
        return localization_.get(lhs->nameTextId) < localization_.get(rhs->nameTextId);
    };
    std::sort(cards_.begin(), cards_.end(), byName);
    std::sort(relics_.begin(), relics_.end(), byName);
    std::sort(enemies_.begin(), enemies_.end(), byName);
    std::sort(statuses_.begin(), statuses_.end(), byName);
    std::sort(consumables_.begin(), consumables_.end(), byName);
}

void CompendiumScene::update(float) {
    const CompendiumSceneLayout layout = calculateCompendiumSceneLayout();
    const Vector2 mouse = GetMousePosition();

    if (searchFocused_) {
        updateSearchInput();
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            searchFocused_ = false;
            return;
        }
    } else {
        if (IsKeyPressed(KEY_ESCAPE)) {
            onBack_();
            return;
        }

        if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            moveTab(-1);
        }

        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            moveTab(1);
        }

        if (IsKeyPressed(KEY_F)) {
            moveFilter(1);
        }

        if (IsKeyPressed(KEY_R)) {
            moveSort(1);
        }

        if (IsKeyPressed(KEY_SLASH) || ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_F))) {
            searchFocused_ = true;
        }

        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            moveSelection(-1);
        }

        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            moveSelection(1);
        }
    }

    if (BasicUi::contains(layout.backButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
        return;
    }

    for (std::size_t i = 0; i < layout.tabs.size(); ++i) {
        if (BasicUi::contains(layout.tabs[i], mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            tab_ = tabFromIndex(i);
            searchFocused_ = false;
            selectedIndex_ = 0u;
            return;
        }
    }

    for (std::size_t i = 0; i < layout.filters.size(); ++i) {
        if (BasicUi::contains(layout.filters[i], mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            filter_ = filterFromIndex(i);
            searchFocused_ = false;
            selectedIndex_ = 0u;
            return;
        }
    }

    if (BasicUi::contains(layout.searchBox, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        searchFocused_ = true;
        return;
    }

    if (BasicUi::contains(layout.clearSearchButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        searchQuery_.clear();
        searchFocused_ = false;
        selectedIndex_ = 0u;
        return;
    }

    if (BasicUi::contains(layout.sortButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        moveSort(1);
        searchFocused_ = false;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !BasicUi::contains(layout.searchBox, mouse)) {
        searchFocused_ = false;
    }

    const std::size_t itemCount = currentItemCount();
    const std::size_t visibleRows = static_cast<std::size_t>(std::max(1.f, std::floor((layout.listPanel.height - 36.f) / 78.f)));
    const std::size_t firstIndex = selectedIndex_ >= visibleRows ? selectedIndex_ - visibleRows + 1u : 0u;
    const std::size_t lastIndex = std::min(itemCount, firstIndex + visibleRows);
    for (std::size_t index = firstIndex; index < lastIndex; ++index) {
        const int visualIndex = static_cast<int>(index - firstIndex);
        if (BasicUi::contains(itemRowBounds(layout.listPanel, visualIndex), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedIndex_ = index;
            return;
        }
    }
}

void CompendiumScene::render() const {
    const CompendiumSceneLayout layout = calculateCompendiumSceneLayout();
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, layout.backButton, localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(font_, localization_.get(TextId("compendium.title")), layout.title, 38.f, Color{240, 240, 250, 255});
    BasicUi::drawCenteredText(font_, localization_.get(TextId("compendium.subtitle")), layout.subtitle, 18.f, Color{175, 182, 205, 255});

    for (std::size_t i = 0; i < layout.tabs.size(); ++i) {
        const bool selected = i == tabIndex(tab_);
        BasicUi::ButtonStyle style;
        style.background = selected ? Color{72, 78, 104, 255} : Color{45, 48, 58, 255};
        style.hoveredBackground = selected ? Color{82, 90, 120, 255} : Color{68, 72, 86, 255};
        style.border = selected ? Color{178, 190, 235, 255} : Color{112, 120, 150, 255};
        BasicUi::drawButton(font_, layout.tabs[i], tabTitle(tabFromIndex(i)), mouse, true, style);
    }

    for (std::size_t i = 0; i < layout.filters.size(); ++i) {
        const bool selected = i == filterIndex(filter_);
        BasicUi::ButtonStyle style;
        style.background = selected ? Color{66, 72, 94, 255} : Color{38, 42, 54, 255};
        style.hoveredBackground = selected ? Color{78, 84, 108, 255} : Color{58, 64, 82, 255};
        style.border = selected ? Color{170, 184, 226, 255} : Color{92, 100, 128, 255};
        BasicUi::drawButton(font_, layout.filters[i], filterTitle(filterFromIndex(i)), mouse, true, style);
    }

    const bool searchHovered = BasicUi::contains(layout.searchBox, mouse);
    const Color searchFill = searchFocused_
        ? Color{44, 50, 70, 255}
        : searchHovered ? Color{38, 43, 58, 255} : Color{30, 34, 46, 255};
    const Color searchBorder = searchFocused_ ? Color{188, 202, 242, 255} : Color{98, 108, 140, 255};
    DrawRectangleRounded(layout.searchBox, 0.12f, 10, searchFill);
    DrawRectangleRoundedLinesEx(layout.searchBox, 0.12f, 10, searchFocused_ ? 3.f : 2.f, searchBorder);
    const std::string searchText = searchQuery_.empty()
        ? localization_.get(TextId("compendium.search_placeholder"))
        : searchQuery_ + (searchFocused_ ? "_" : "");
    const Color searchTextColor = searchQuery_.empty() ? Color{142, 150, 174, 255} : Color{232, 236, 248, 255};
    BasicUi::drawTextFitted(font_, searchText, Vector2{layout.searchBox.x + 16.f, layout.searchBox.y + 9.f}, layout.searchBox.width - 32.f, 18.f, 13.f, searchTextColor);

    BasicUi::drawButton(font_, layout.clearSearchButton, localization_.get(TextId("compendium.search_clear")), mouse, !searchQuery_.empty());
    BasicUi::drawButton(
        font_,
        layout.sortButton,
        localization_.format(TextId("compendium.sort.button"), {{"sort", sortTitle(sortMode_)}}),
        mouse
    );

    BasicUi::drawCenteredText(font_, progressText(), layout.progress, 16.f, Color{170, 180, 210, 255});

    renderList(layout.listPanel, mouse);
    renderDetails(layout.detailsPanel);
}

void CompendiumScene::moveTab(const int direction) {
    const int tabCount = 5;
    int next = static_cast<int>(tabIndex(tab_)) + direction;
    if (next < 0) {
        next = tabCount - 1;
    }
    if (next >= tabCount) {
        next = 0;
    }

    tab_ = tabFromIndex(static_cast<std::size_t>(next));
    selectedIndex_ = 0u;
}

void CompendiumScene::moveSelection(const int direction) {
    const std::size_t count = currentItemCount();
    if (count == 0u) {
        selectedIndex_ = 0u;
        return;
    }

    int next = static_cast<int>(selectedIndex_) + direction;
    if (next < 0) {
        next = static_cast<int>(count) - 1;
    }
    if (next >= static_cast<int>(count)) {
        next = 0;
    }

    selectedIndex_ = static_cast<std::size_t>(next);
}

void CompendiumScene::moveFilter(const int direction) {
    const int filterCount = 3;
    int next = static_cast<int>(filterIndex(filter_)) + direction;
    if (next < 0) {
        next = filterCount - 1;
    }
    if (next >= filterCount) {
        next = 0;
    }

    filter_ = filterFromIndex(static_cast<std::size_t>(next));
    selectedIndex_ = 0u;
}

void CompendiumScene::moveSort(const int direction) {
    const int sortCount = 3;
    int next = static_cast<int>(sortIndex(sortMode_)) + direction;
    if (next < 0) {
        next = sortCount - 1;
    }
    if (next >= sortCount) {
        next = 0;
    }

    sortMode_ = sortFromIndex(static_cast<std::size_t>(next));
    selectedIndex_ = 0u;
}

void CompendiumScene::updateSearchInput() {
    bool changed = false;
    int character = GetCharPressed();
    while (character > 0) {
        if (character >= 32 && searchQuery_.size() < 80u) {
            appendUtf8(searchQuery_, static_cast<std::uint32_t>(character));
            changed = true;
        }
        character = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !searchQuery_.empty()) {
        popUtf8(searchQuery_);
        changed = true;
    }

    if (IsKeyPressed(KEY_DELETE) && !searchQuery_.empty()) {
        searchQuery_.clear();
        changed = true;
    }

    if (changed) {
        selectedIndex_ = 0u;
    }
}

std::size_t CompendiumScene::currentItemCount() const {
    return currentFilteredIndices().size();
}

std::size_t CompendiumScene::totalItemCount(const Tab tab) const {
    switch (tab) {
        case Tab::Cards: return cards_.size();
        case Tab::Relics: return relics_.size();
        case Tab::Enemies: return enemies_.size();
        case Tab::Statuses: return statuses_.size();
        case Tab::Consumables: return consumables_.size();
    }

    return 0u;
}

std::size_t CompendiumScene::knownItemCount(const Tab tab) const {
    std::size_t count = 0u;
    const std::size_t total = totalItemCount(tab);
    for (std::size_t index = 0u; index < total; ++index) {
        if (itemKnownAt(tab, index)) {
            ++count;
        }
    }

    return count;
}

std::vector<std::size_t> CompendiumScene::currentFilteredIndices() const {
    std::vector<std::size_t> indices;
    const std::size_t total = totalItemCount(tab_);
    indices.reserve(total);

    for (std::size_t index = 0u; index < total; ++index) {
        const bool known = itemKnownAt(tab_, index);
        const bool filterMatches = filter_ == Filter::All || (filter_ == Filter::Known && known) || (filter_ == Filter::Unknown && !known);
        if (filterMatches && itemMatchesSearch(tab_, index)) {
            indices.push_back(index);
        }
    }

    const auto compareByName = [this](const std::size_t lhs, const std::size_t rhs) {
        const std::string lhsName = normalizeSearchText(itemSortName(tab_, lhs));
        const std::string rhsName = normalizeSearchText(itemSortName(tab_, rhs));
        if (lhsName != rhsName) {
            return lhsName < rhsName;
        }
        return lhs < rhs;
    };

    std::stable_sort(indices.begin(), indices.end(), [this, compareByName](const std::size_t lhs, const std::size_t rhs) {
        switch (sortMode_) {
            case SortMode::Name:
                return compareByName(lhs, rhs);
            case SortMode::Known: {
                const bool lhsKnown = itemKnownAt(tab_, lhs);
                const bool rhsKnown = itemKnownAt(tab_, rhs);
                if (lhsKnown != rhsKnown) {
                    return lhsKnown && !rhsKnown;
                }
                return compareByName(lhs, rhs);
            }
            case SortMode::Category: {
                const int lhsRank = itemSortCategoryRank(tab_, lhs);
                const int rhsRank = itemSortCategoryRank(tab_, rhs);
                if (lhsRank != rhsRank) {
                    return lhsRank < rhsRank;
                }
                return compareByName(lhs, rhs);
            }
        }

        return compareByName(lhs, rhs);
    });

    return indices;
}

std::size_t CompendiumScene::resolvedSelectedIndex() const {
    const std::vector<std::size_t> indices = currentFilteredIndices();
    if (indices.empty()) {
        return 0u;
    }

    return indices.at(std::min(selectedIndex_, indices.size() - 1u));
}

bool CompendiumScene::itemKnownAt(const Tab tab, const std::size_t rawIndex) const {
    switch (tab) {
        case Tab::Cards: return rawIndex < cards_.size() && isKnownCard(*cards_.at(rawIndex));
        case Tab::Relics: return rawIndex < relics_.size() && isKnownRelic(*relics_.at(rawIndex));
        case Tab::Enemies: return rawIndex < enemies_.size() && isKnownEnemy(*enemies_.at(rawIndex));
        case Tab::Statuses: return rawIndex < statuses_.size() && isKnownStatus(*statuses_.at(rawIndex));
        case Tab::Consumables: return rawIndex < consumables_.size() && isKnownConsumable(*consumables_.at(rawIndex));
    }

    return true;
}

std::string CompendiumScene::tabTitle(const Tab tab) const {
    switch (tab) {
        case Tab::Cards: return localization_.get(TextId("compendium.tab.cards"));
        case Tab::Relics: return localization_.get(TextId("compendium.tab.relics"));
        case Tab::Enemies: return localization_.get(TextId("compendium.tab.enemies"));
        case Tab::Statuses: return localization_.get(TextId("compendium.tab.statuses"));
        case Tab::Consumables: return localization_.get(TextId("compendium.tab.consumables"));
    }

    return {};
}

std::string CompendiumScene::filterTitle(const Filter filter) const {
    switch (filter) {
        case Filter::All: return localization_.get(TextId("compendium.filter.all"));
        case Filter::Known: return localization_.get(TextId("compendium.filter.known"));
        case Filter::Unknown: return localization_.get(TextId("compendium.filter.unknown"));
    }

    return {};
}

std::string CompendiumScene::sortTitle(const SortMode sortMode) const {
    switch (sortMode) {
        case SortMode::Name: return localization_.get(TextId("compendium.sort.name"));
        case SortMode::Known: return localization_.get(TextId("compendium.sort.known"));
        case SortMode::Category: return localization_.get(TextId("compendium.sort.category"));
    }

    return {};
}

std::string CompendiumScene::progressText() const {
    const std::size_t known = knownItemCount(tab_);
    const std::size_t total = totalItemCount(tab_);
    const std::size_t visible = currentItemCount();
    return localization_.format(
        TextId("compendium.progress"),
        {
            {"known", std::to_string(known)},
            {"total", std::to_string(total)},
            {"visible", std::to_string(visible)},
            {"filter", filterTitle(filter_)},
            {"sort", sortTitle(sortMode_)},
            {"search", searchQuery_.empty() ? localization_.get(TextId("compendium.search.none")) : searchQuery_}
        }
    );
}

std::string CompendiumScene::selectedItemName() const {
    if (currentItemCount() == 0u) {
        return {};
    }

    const std::size_t rawIndex = resolvedSelectedIndex();
    switch (tab_) {
        case Tab::Cards: return localization_.get(cards_.at(rawIndex)->nameTextId);
        case Tab::Relics: return localization_.get(relics_.at(rawIndex)->nameTextId);
        case Tab::Enemies: return localization_.get(enemies_.at(rawIndex)->nameTextId);
        case Tab::Statuses: return localization_.get(statuses_.at(rawIndex)->nameTextId);
        case Tab::Consumables: return localization_.get(consumables_.at(rawIndex)->nameTextId);
    }

    return {};
}

std::string CompendiumScene::selectedItemSubtitle() const {
    if (currentItemCount() == 0u) {
        return {};
    }

    const std::size_t rawIndex = resolvedSelectedIndex();
    switch (tab_) {
        case Tab::Cards: {
            const CardDefinition& card = *cards_.at(rawIndex);
            return localizedEnum("card_type", toString(card.type)) + " / " + localizedEnum("rarity", toString(card.rarity));
        }
        case Tab::Relics: {
            const RelicDefinition& relic = *relics_.at(rawIndex);
            return localizedEnum("rarity", toString(relic.rarity));
        }
        case Tab::Enemies: {
            const EnemyDefinition& enemy = *enemies_.at(rawIndex);
            return localization_.format(TextId("compendium.enemy.hp_value"), {{"hp", std::to_string(enemy.maxHp)}});
        }
        case Tab::Statuses: {
            const StatusDefinition& status = *statuses_.at(rawIndex);
            return localizedEnum("status_type", toString(status.type));
        }
        case Tab::Consumables: {
            const ConsumableDefinition& consumable = *consumables_.at(rawIndex);
            return localizedEnum("rarity", toString(consumable.rarity));
        }
    }

    return {};
}

bool CompendiumScene::selectedItemKnown() const {
    if (currentItemCount() == 0u) {
        return false;
    }

    return itemKnownAt(tab_, resolvedSelectedIndex());
}

bool CompendiumScene::isKnownCard(const CardDefinition& card) const {
    if (profile_ == nullptr || profile_->unlockedCardIds.empty()) {
        return true;
    }

    return containsId(profile_->unlockedCardIds, card.id.value);
}

bool CompendiumScene::isKnownRelic(const RelicDefinition& relic) const {
    if (profile_ == nullptr || profile_->unlockedRelicIds.empty()) {
        return true;
    }

    return containsId(profile_->unlockedRelicIds, relic.id.value);
}

bool CompendiumScene::isKnownEnemy(const EnemyDefinition& enemy) const {
    if (profile_ == nullptr) {
        return true;
    }

    return containsId(profile_->discoveredEnemyIds, enemy.id.value);
}

bool CompendiumScene::isKnownStatus(const StatusDefinition& status) const {
    if (profile_ == nullptr) {
        return true;
    }

    return containsId(profile_->discoveredStatusIds, status.id.value);
}

bool CompendiumScene::isKnownConsumable(const ConsumableDefinition& consumable) const {
    if (profile_ == nullptr) {
        return true;
    }

    return containsId(profile_->discoveredConsumableIds, consumable.id.value);
}

bool CompendiumScene::itemMatchesSearch(const Tab tab, const std::size_t rawIndex) const {
    if (searchQuery_.empty()) {
        return true;
    }

    const std::string haystack = normalizeSearchText(itemSearchText(tab, rawIndex));
    const std::string needle = normalizeSearchText(searchQuery_);
    return haystack.find(needle) != std::string::npos;
}

std::string CompendiumScene::itemSearchText(const Tab tab, const std::size_t rawIndex) const {
    const bool known = itemKnownAt(tab, rawIndex);
    const std::string unknown = localization_.get(TextId("compendium.unknown_entry"));
    switch (tab) {
        case Tab::Cards: {
            if (rawIndex >= cards_.size()) {
                return {};
            }
            const CardDefinition& card = *cards_.at(rawIndex);
            if (!known) {
                return unknown + " " + localizedEnum("card_type", toString(card.type)) + " " + localizedEnum("rarity", toString(card.rarity));
            }
            return card.id.value + " " + localization_.get(card.nameTextId) + " " + localization_.get(card.descriptionTextId) + " " +
                localizedEnum("card_type", toString(card.type)) + " " + localizedEnum("rarity", toString(card.rarity)) + " " + card.ownerActorId;
        }
        case Tab::Relics: {
            if (rawIndex >= relics_.size()) {
                return {};
            }
            const RelicDefinition& relic = *relics_.at(rawIndex);
            if (!known) {
                return unknown + " " + localizedEnum("rarity", toString(relic.rarity));
            }
            return relic.id.value + " " + localization_.get(relic.nameTextId) + " " + localization_.get(relic.descriptionTextId) + " " +
                localizedEnum("rarity", toString(relic.rarity)) + " " + relic.mechanicId;
        }
        case Tab::Enemies: {
            if (rawIndex >= enemies_.size()) {
                return {};
            }
            const EnemyDefinition& enemy = *enemies_.at(rawIndex);
            if (!known) {
                return unknown + " " + localization_.get(TextId("compendium.locked_enemy_subtitle"));
            }
            std::string text = enemy.id.value + " " + localization_.get(enemy.nameTextId) + " " + std::to_string(enemy.maxHp);
            for (const EnemyActionDefinition& action : enemy.actions) {
                const TextId actionNameTextId("enemy.action." + action.id + ".name");
                text += " " + action.id;
                text += " " + (localization_.hasText(actionNameTextId) ? localization_.get(actionNameTextId) : std::string{});
            }
            return text;
        }
        case Tab::Statuses: {
            if (rawIndex >= statuses_.size()) {
                return {};
            }
            const StatusDefinition& status = *statuses_.at(rawIndex);
            if (!known) {
                return unknown + " " + localization_.get(TextId("compendium.locked_status_subtitle"));
            }
            std::string text = status.id.value + " " + localization_.get(status.nameTextId) + " " + localization_.get(status.descriptionTextId) + " " +
                localizedEnum("status_type", toString(status.type)) + " " + localizedEnum("status_duration", toString(status.durationRule));
            for (const StatusModifierDefinition& modifier : status.modifiers) {
                text += " " + localization_.get(modifier.descriptionTextId);
            }
            if (!status.exclusiveGroup.empty()) {
                text += " " + status.exclusiveGroup;
            }
            return text;
        }
        case Tab::Consumables: {
            if (rawIndex >= consumables_.size()) {
                return {};
            }
            const ConsumableDefinition& consumable = *consumables_.at(rawIndex);
            if (!known) {
                return unknown + " " + localization_.get(TextId("compendium.locked_consumable_subtitle"));
            }
            return consumable.id.value + " " + localization_.get(consumable.nameTextId) + " " + localization_.get(consumable.descriptionTextId) + " " +
                localizedEnum("rarity", toString(consumable.rarity)) + " " + std::to_string(consumable.goldCost);
        }
    }

    return {};
}

std::string CompendiumScene::itemSortName(const Tab tab, const std::size_t rawIndex) const {
    const bool known = itemKnownAt(tab, rawIndex);
    if (!known) {
        return localization_.get(TextId("compendium.unknown_entry")) + " " + std::to_string(rawIndex);
    }

    switch (tab) {
        case Tab::Cards: return rawIndex < cards_.size() ? localization_.get(cards_.at(rawIndex)->nameTextId) : std::string{};
        case Tab::Relics: return rawIndex < relics_.size() ? localization_.get(relics_.at(rawIndex)->nameTextId) : std::string{};
        case Tab::Enemies: return rawIndex < enemies_.size() ? localization_.get(enemies_.at(rawIndex)->nameTextId) : std::string{};
        case Tab::Statuses: return rawIndex < statuses_.size() ? localization_.get(statuses_.at(rawIndex)->nameTextId) : std::string{};
        case Tab::Consumables: return rawIndex < consumables_.size() ? localization_.get(consumables_.at(rawIndex)->nameTextId) : std::string{};
    }

    return {};
}

int CompendiumScene::itemSortCategoryRank(const Tab tab, const std::size_t rawIndex) const {
    switch (tab) {
        case Tab::Cards:
            return rawIndex < cards_.size()
                ? static_cast<int>(cards_.at(rawIndex)->type) * 100 + static_cast<int>(cards_.at(rawIndex)->rarity)
                : 0;
        case Tab::Relics:
            return rawIndex < relics_.size() ? static_cast<int>(relics_.at(rawIndex)->rarity) : 0;
        case Tab::Enemies:
            return rawIndex < enemies_.size() ? enemies_.at(rawIndex)->maxHp : 0;
        case Tab::Statuses:
            return rawIndex < statuses_.size()
                ? static_cast<int>(statuses_.at(rawIndex)->type) * 100 + static_cast<int>(statuses_.at(rawIndex)->durationRule)
                : 0;
        case Tab::Consumables:
            return rawIndex < consumables_.size() ? static_cast<int>(consumables_.at(rawIndex)->rarity) : 0;
    }

    return 0;
}

void CompendiumScene::renderList(const Rectangle listPanel, const Vector2 mouse) const {
    DrawRectangleRounded(listPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(listPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    const std::vector<std::size_t> indices = currentFilteredIndices();
    const std::size_t itemCount = indices.size();
    if (itemCount == 0u) {
        const TextId emptyTextId = totalItemCount(tab_) == 0u
            ? TextId("compendium.empty")
            : !searchQuery_.empty() ? TextId("compendium.search_empty") : TextId("compendium.filter_empty");
        BasicUi::drawCenteredText(font_, localization_.get(emptyTextId), listPanel, 22.f, Color{190, 196, 216, 255});
        return;
    }

    const std::size_t visibleRows = static_cast<std::size_t>(std::max(1.f, std::floor((listPanel.height - 36.f) / 78.f)));
    const std::size_t firstIndex = selectedIndex_ >= visibleRows ? selectedIndex_ - visibleRows + 1u : 0u;
    const std::size_t lastIndex = std::min(itemCount, firstIndex + visibleRows);

    for (std::size_t index = firstIndex; index < lastIndex; ++index) {
        const std::size_t rawIndex = indices.at(index);
        const int visualIndex = static_cast<int>(index - firstIndex);
        const Rectangle row = itemRowBounds(listPanel, visualIndex);
        const bool selected = index == selectedIndex_;
        const bool hovered = BasicUi::contains(row, mouse);
        const bool known = itemKnownAt(tab_, rawIndex);

        const Color fill = selected
            ? Color{60, 66, 88, 255}
            : hovered ? Color{42, 46, 60, 255} : Color{34, 37, 49, 255};
        const Color border = selected ? Color{176, 188, 235, 255} : Color{86, 94, 122, 255};
        const Color text = known ? Color{232, 236, 248, 255} : Color{145, 150, 168, 255};

        DrawRectangleRounded(row, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(row, 0.08f, 10, selected ? 3.f : 2.f, border);

        std::string name;
        std::string subtitle;
        switch (tab_) {
            case Tab::Cards: {
                const CardDefinition& card = *cards_.at(rawIndex);
                name = known ? localization_.get(card.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
                subtitle = localizedEnum("card_type", toString(card.type)) + " / " + localizedEnum("rarity", toString(card.rarity));
                break;
            }
            case Tab::Relics: {
                const RelicDefinition& relic = *relics_.at(rawIndex);
                name = known ? localization_.get(relic.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
                subtitle = localizedEnum("rarity", toString(relic.rarity));
                break;
            }
            case Tab::Enemies: {
                const EnemyDefinition& enemy = *enemies_.at(rawIndex);
                name = known ? localization_.get(enemy.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
                subtitle = known
                    ? localization_.format(TextId("compendium.enemy.hp_value"), {{"hp", std::to_string(enemy.maxHp)}})
                    : localization_.get(TextId("compendium.locked_enemy_subtitle"));
                break;
            }
            case Tab::Statuses: {
                const StatusDefinition& status = *statuses_.at(rawIndex);
                name = known ? localization_.get(status.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
                subtitle = known ? localizedEnum("status_type", toString(status.type)) : localization_.get(TextId("compendium.locked_status_subtitle"));
                break;
            }
            case Tab::Consumables: {
                const ConsumableDefinition& consumable = *consumables_.at(rawIndex);
                name = known ? localization_.get(consumable.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
                subtitle = known ? localizedEnum("rarity", toString(consumable.rarity)) : localization_.get(TextId("compendium.locked_consumable_subtitle"));
                break;
            }
        }

        BasicUi::drawTextFitted(font_, name, Vector2{row.x + 16.f, row.y + 12.f}, row.width - 32.f, 21.f, 14.f, text);
        BasicUi::drawTextFitted(font_, subtitle, Vector2{row.x + 16.f, row.y + 42.f}, row.width - 32.f, 15.f, 12.f, Color{172, 180, 204, 255});
    }

    const std::string counter = localization_.format(
        TextId("compendium.counter"),
        {{"current", std::to_string(std::min(selectedIndex_ + 1u, itemCount))}, {"total", std::to_string(itemCount)}}
    );
    BasicUi::drawCenteredText(font_, counter, Rectangle{listPanel.x, listPanel.y + listPanel.height - 28.f, listPanel.width, 20.f}, 15.f, Color{155, 164, 190, 255});
}

void CompendiumScene::renderDetails(const Rectangle detailsPanel) const {
    DrawRectangleRounded(detailsPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(detailsPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    if (currentItemCount() == 0u) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("compendium.select_hint")), detailsPanel, 22.f, Color{190, 196, 216, 255});
        return;
    }

    const std::size_t rawIndex = resolvedSelectedIndex();
    switch (tab_) {
        case Tab::Cards:
            renderCardDetails(detailsPanel, *cards_.at(rawIndex), selectedItemKnown());
            return;
        case Tab::Relics:
            renderRelicDetails(detailsPanel, *relics_.at(rawIndex), selectedItemKnown());
            return;
        case Tab::Enemies:
            renderEnemyDetails(detailsPanel, *enemies_.at(rawIndex), selectedItemKnown());
            return;
        case Tab::Statuses:
            renderStatusDetails(detailsPanel, *statuses_.at(rawIndex), selectedItemKnown());
            return;
        case Tab::Consumables:
            renderConsumableDetails(detailsPanel, *consumables_.at(rawIndex), selectedItemKnown());
            return;
    }
}

void CompendiumScene::renderCardDetails(const Rectangle panel, const CardDefinition& card, const bool known) const {
    const float x = panel.x + 32.f;
    float y = panel.y + 30.f;
    const float width = panel.width - 64.f;
    const std::string name = known ? localization_.get(card.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
    BasicUi::drawTextFitted(font_, name, Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
    y += 48.f;

    if (!known) {
        drawWrapped(font_, localization_.get(TextId("compendium.locked_card_hint")), x, y, width, 18.f, Color{226, 214, 240, 255});
        return;
    }

    CardDescriptionFormatter formatter(localization_);
    drawField(font_, localization_.get(TextId("compendium.field.description")), formatter.formatStaticDescription(card), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.type")), localizedEnum("card_type", toString(card.type)), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.rarity")), localizedEnum("rarity", toString(card.rarity)), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.energy")), std::to_string(card.energyCost), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.gold_cost")), std::to_string(card.goldCost), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.owner")), card.ownerActorId.empty() ? localization_.get(TextId("compendium.owner.any_actor")) : card.ownerActorId, x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.effects")), std::to_string(card.effects.size()), x, y, width);
}

void CompendiumScene::renderRelicDetails(const Rectangle panel, const RelicDefinition& relic, const bool known) const {
    const float x = panel.x + 32.f;
    float y = panel.y + 30.f;
    const float width = panel.width - 64.f;
    const std::string name = known ? localization_.get(relic.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
    BasicUi::drawTextFitted(font_, name, Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
    y += 48.f;

    if (!known) {
        drawWrapped(font_, localization_.get(TextId("compendium.locked_relic_hint")), x, y, width, 18.f, Color{226, 214, 240, 255});
        return;
    }

    drawField(font_, localization_.get(TextId("compendium.field.description")), localization_.get(relic.descriptionTextId), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.rarity")), localizedEnum("rarity", toString(relic.rarity)), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.mechanic")), relic.mechanicId, x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.modifiers")), std::to_string(relic.modifiers.size()), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.triggers")), std::to_string(relic.triggers.size()), x, y, width);
}

void CompendiumScene::renderEnemyDetails(const Rectangle panel, const EnemyDefinition& enemy, const bool known) const {
    const float x = panel.x + 32.f;
    float y = panel.y + 30.f;
    const float width = panel.width - 64.f;
    const std::string name = known ? localization_.get(enemy.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
    BasicUi::drawTextFitted(font_, name, Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
    y += 48.f;

    if (!known) {
        drawWrapped(font_, localization_.get(TextId("compendium.locked_enemy_hint")), x, y, width, 18.f, Color{226, 214, 240, 255});
        return;
    }

    drawField(font_, localization_.get(TextId("compendium.field.hp")), std::to_string(enemy.maxHp), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.starting_block")), std::to_string(enemy.startingBlock), x, y, width);

    BasicUi::drawText(font_, localization_.get(TextId("compendium.field.enemy_actions")), Vector2{x, y}, 17.f, Color{236, 229, 198, 255});
    y += 28.f;
    if (enemy.actions.empty()) {
        drawWrapped(font_, localization_.get(TextId("compendium.none")), x + 18.f, y, width - 18.f, 17.f, Color{214, 220, 238, 255});
        return;
    }

    for (const EnemyActionDefinition& action : enemy.actions) {
        const TextId actionNameTextId("enemy.action." + action.id + ".name");
        const std::string actionName = localization_.hasText(actionNameTextId) ? localization_.get(actionNameTextId) : action.id;
        drawWrapped(font_, "* " + actionName, x + 18.f, y, width - 18.f, 17.f, Color{214, 220, 238, 255});
    }
}

void CompendiumScene::renderStatusDetails(const Rectangle panel, const StatusDefinition& status, const bool known) const {
    const float x = panel.x + 32.f;
    float y = panel.y + 30.f;
    const float width = panel.width - 64.f;
    const std::string name = known ? localization_.get(status.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
    BasicUi::drawTextFitted(font_, name, Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
    y += 48.f;

    if (!known) {
        drawWrapped(font_, localization_.get(TextId("compendium.locked_status_hint")), x, y, width, 18.f, Color{226, 214, 240, 255});
        return;
    }

    drawField(font_, localization_.get(TextId("compendium.field.description")), localization_.get(status.descriptionTextId), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.type")), localizedEnum("status_type", toString(status.type)), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.duration")), localizedEnum("status_duration", toString(status.durationRule)), x, y, width);
    std::string mechanics;
    for (const StatusModifierDefinition& modifier : status.modifiers) {
        if (!mechanics.empty()) {
            mechanics += "\n";
        }
        mechanics += localization_.get(modifier.descriptionTextId);
    }
    for (const StatusTriggerDefinition& trigger : status.triggers) {
        if (!mechanics.empty()) {
            mechanics += "\n";
        }
        switch (trigger.logType) {
            case StatusTriggerLogType::None:
                mechanics += localization_.get(TextId("compendium.status.trigger"));
                break;
            case StatusTriggerLogType::PoisonDamage:
                mechanics += localization_.get(TextId("status.end_turn_effect.poison_damage"));
                break;
            case StatusTriggerLogType::BurnDamage:
                mechanics += localization_.get(TextId("status.end_turn_effect.burn_damage"));
                break;
        }
    }
    drawField(
        font_,
        localization_.get(TextId("compendium.field.mechanics")),
        mechanics.empty() ? localization_.get(TextId("compendium.none")) : mechanics,
        x,
        y,
        width
    );
}

void CompendiumScene::renderConsumableDetails(const Rectangle panel, const ConsumableDefinition& consumable, const bool known) const {
    const float x = panel.x + 32.f;
    float y = panel.y + 30.f;
    const float width = panel.width - 64.f;
    const std::string name = known ? localization_.get(consumable.nameTextId) : localization_.get(TextId("compendium.unknown_entry"));
    BasicUi::drawTextFitted(font_, name, Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
    y += 48.f;

    if (!known) {
        drawWrapped(font_, localization_.get(TextId("compendium.locked_consumable_hint")), x, y, width, 18.f, Color{226, 214, 240, 255});
        return;
    }

    drawField(font_, localization_.get(TextId("compendium.field.description")), localization_.get(consumable.descriptionTextId), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.rarity")), localizedEnum("rarity", toString(consumable.rarity)), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.gold_cost")), std::to_string(consumable.goldCost), x, y, width);
    drawField(font_, localization_.get(TextId("compendium.field.effects")), std::to_string(consumable.effects.size()), x, y, width);
}

std::string CompendiumScene::localizedEnum(const std::string& category, const std::string& value) const {
    const TextId textId("compendium." + category + "." + value);
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return value;
}

bool CompendiumScene::containsId(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}
