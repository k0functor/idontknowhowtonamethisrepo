#include "ProfileProgressScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "archetypes/PlayableArchetypeId.hpp"
#include "cards/CardId.hpp"
#include "consumables/ConsumableId.hpp"
#include "enemies/EnemyId.hpp"
#include "relics/RelicId.hpp"
#include "statuses/StatusId.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include <raylib.h>

namespace {
struct ProfileProgressSceneLayout {
    Rectangle backButton{};
    Rectangle title{};
    Rectangle journalTab{};
    Rectangle statisticsTab{};
    Rectangle listPanel{};
    Rectangle detailsPanel{};
};

ProfileProgressSceneLayout calculateProfileProgressSceneLayout() {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    ProfileProgressSceneLayout layout;
    layout.backButton = Rectangle{32.f, 32.f, 140.f, 48.f};
    layout.title = Rectangle{0.f, 50.f, screenWidth, 56.f};
    const float tabWidth = std::clamp(screenWidth * 0.18f, 210.f, 280.f);
    const float tabHeight = 42.f;
    const float tabGap = 14.f;
    const float tabTotalWidth = 2.f * tabWidth + tabGap;
    const float tabX = screenWidth * 0.5f - tabTotalWidth * 0.5f;
    const float tabY = 116.f;
    layout.journalTab = Rectangle{tabX, tabY, tabWidth, tabHeight};
    layout.statisticsTab = Rectangle{tabX + tabWidth + tabGap, tabY, tabWidth, tabHeight};

    const float margin = 64.f;
    const float top = tabY + tabHeight + 18.f;
    const float gap = 24.f;
    const float height = std::max(316.f, screenHeight - top - 32.f);
    const float listWidth = std::clamp(screenWidth * 0.42f, 440.f, 620.f);
    layout.listPanel = Rectangle{margin, top, listWidth, height};
    layout.detailsPanel = Rectangle{layout.listPanel.x + layout.listPanel.width + gap, top, screenWidth - margin - (layout.listPanel.x + layout.listPanel.width + gap), height};
    return layout;
}

constexpr float rowHeight = 72.f;
constexpr float rowGap = 10.f;

std::size_t visibleRowCount(const Rectangle listPanel) {
    return std::max<std::size_t>(1u, static_cast<std::size_t>((listPanel.height - 36.f + rowGap) / (rowHeight + rowGap)));
}

Rectangle progressRowBounds(const Rectangle listPanel, const std::size_t visibleIndex) {
    return Rectangle{
        listPanel.x + 18.f,
        listPanel.y + 18.f + static_cast<float>(visibleIndex) * (rowHeight + rowGap),
        listPanel.width - 36.f,
        rowHeight
    };
}

bool isEntryType(const ProfileProgressEntry& entry, const char* type) {
    return entry.type == type;
}

BasicUi::ButtonStyle tabButtonStyle(const bool active) {
    BasicUi::ButtonStyle style;
    style.background = active ? Color{74, 78, 102, 255} : Color{45, 48, 58, 255};
    style.hoveredBackground = active ? Color{88, 94, 124, 255} : Color{62, 66, 82, 255};
    style.border = active ? Color{214, 184, 85, 255} : Color{130, 136, 160, 255};
    style.text = active ? Color{252, 240, 198, 255} : Color{235, 235, 242, 255};
    return style;
}
}

ProfileProgressScene::ProfileProgressScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const ContentRegistry& content,
    const ProfileData* profile,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      content_(content),
      profile_(profile),
      onBack_(std::move(onBack)) {
    if (profile_ != nullptr) {
        entries_ = profile_->progressLog;
        std::reverse(entries_.begin(), entries_.end());
    }
    rebuildStatistics();
}

void ProfileProgressScene::update(float) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        onBack_();
        return;
    }

    if (IsKeyPressed(KEY_TAB)) {
        setViewMode(viewMode_ == ViewMode::Journal ? ViewMode::Statistics : ViewMode::Journal);
        return;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        moveSelection(-1);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        moveSelection(1);
    }
    if (IsKeyPressed(KEY_PAGE_UP)) {
        pageSelection(-1);
    }
    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        pageSelection(1);
    }
    if (IsKeyPressed(KEY_HOME) && visibleEntryCount() > 0u) {
        selectedIndex_ = 0;
    }
    if (IsKeyPressed(KEY_END) && visibleEntryCount() > 0u) {
        selectedIndex_ = visibleEntryCount() - 1u;
    }

    const ProfileProgressSceneLayout layout = calculateProfileProgressSceneLayout();
    const Vector2 mouse = GetMousePosition();

    if (BasicUi::contains(layout.backButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
        return;
    }

    if (BasicUi::contains(layout.journalTab, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setViewMode(ViewMode::Journal);
        return;
    }

    if (BasicUi::contains(layout.statisticsTab, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setViewMode(ViewMode::Statistics);
        return;
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.f && BasicUi::contains(layout.listPanel, mouse)) {
        moveSelection(wheel > 0.f ? -1 : 1);
    }

    const std::size_t rows = visibleRowCount(layout.listPanel);
    const std::size_t first = firstVisibleIndex(rows);
    const std::size_t count = visibleEntryCount();
    for (std::size_t visible = 0; visible < rows; ++visible) {
        const std::size_t index = first + visible;
        if (index >= count) {
            break;
        }
        if (BasicUi::contains(progressRowBounds(layout.listPanel, visible), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedIndex_ = index;
            return;
        }
    }
}

void ProfileProgressScene::render() const {
    const ProfileProgressSceneLayout layout = calculateProfileProgressSceneLayout();
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, layout.backButton, localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(font_, localization_.get(TextId("progress_log.title")), layout.title, 38.f, Color{240, 240, 250, 255});
    BasicUi::drawButton(font_, layout.journalTab, localization_.get(TextId("progress_log.tab.journal")), mouse, true, tabButtonStyle(viewMode_ == ViewMode::Journal));
    BasicUi::drawButton(font_, layout.statisticsTab, localization_.get(TextId("progress_log.tab.statistics")), mouse, true, tabButtonStyle(viewMode_ == ViewMode::Statistics));

    DrawRectangleRounded(layout.listPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(layout.listPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    const std::size_t count = visibleEntryCount();
    if (count == 0u) {
        const TextId emptyTextId = viewMode_ == ViewMode::Journal
            ? TextId("progress_log.empty")
            : TextId("progress_log.stats.empty");
        BasicUi::drawCenteredText(font_, localization_.get(emptyTextId), layout.listPanel, 22.f, Color{190, 196, 216, 255});
    }

    const std::size_t rows = visibleRowCount(layout.listPanel);
    const std::size_t first = firstVisibleIndex(rows);
    for (std::size_t visible = 0; visible < rows; ++visible) {
        const std::size_t index = first + visible;
        if (index >= count) {
            break;
        }

        const Rectangle row = progressRowBounds(layout.listPanel, visible);
        const bool selected = index == selectedIndex_;
        const bool hovered = BasicUi::contains(row, mouse);
        const Color fill = selected
            ? Color{60, 66, 88, 255}
            : hovered ? Color{42, 46, 60, 255} : Color{34, 37, 49, 255};
        const Color border = selected ? Color{176, 188, 235, 255} : Color{86, 94, 122, 255};

        DrawRectangleRounded(row, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(row, 0.08f, 10, selected ? 3.f : 2.f, border);

        if (viewMode_ == ViewMode::Journal) {
            const ProfileProgressEntry& entry = entries_[index];
            BasicUi::drawTextFitted(font_, typeLabel(entry), Vector2{row.x + 18.f, row.y + 12.f}, row.width - 36.f, 16.f, 12.f, Color{185, 194, 224, 255});
            BasicUi::drawTextFitted(font_, entryName(entry), Vector2{row.x + 18.f, row.y + 38.f}, row.width - 36.f, 21.f, 14.f, Color{236, 239, 250, 255});
        } else {
            const StatisticEntry& statistic = statistics_[index];
            BasicUi::drawTextFitted(font_, localization_.get(statistic.labelTextId), Vector2{row.x + 18.f, row.y + 13.f}, row.width - 36.f, 21.f, 14.f, Color{236, 239, 250, 255});
            BasicUi::drawTextFitted(font_, std::to_string(statistic.value), Vector2{row.x + 18.f, row.y + 43.f}, row.width - 36.f, 18.f, 13.f, Color{185, 194, 224, 255});
        }
    }

    DrawRectangleRounded(layout.detailsPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(layout.detailsPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    float y = layout.detailsPanel.y + 34.f;
    const float x = layout.detailsPanel.x + 32.f;
    const float width = layout.detailsPanel.width - 64.f;

    if (viewMode_ == ViewMode::Journal) {
        const ProfileProgressEntry* entry = selectedEntry();
        if (entry == nullptr) {
            BasicUi::drawCenteredText(font_, localization_.get(TextId("progress_log.select_hint")), layout.detailsPanel, 22.f, Color{190, 196, 216, 255});
        } else {
            BasicUi::drawTextFitted(font_, entryName(*entry), Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
            y += 48.f;
            BasicUi::drawTextFitted(font_, typeLabel(*entry), Vector2{x, y}, width, 18.f, 14.f, Color{185, 194, 224, 255});
            y += 38.f;

            for (const std::string& line : BasicUi::wrapText(font_, entryDescription(*entry), 18.f, width)) {
                BasicUi::drawText(font_, line, Vector2{x, y}, 18.f, Color{206, 212, 232, 255});
                y += 24.f;
            }
        }
    } else {
        const StatisticEntry* statistic = selectedStatistic();
        if (statistic == nullptr) {
            BasicUi::drawCenteredText(font_, localization_.get(TextId("progress_log.stats.empty")), layout.detailsPanel, 22.f, Color{190, 196, 216, 255});
        } else {
            BasicUi::drawTextFitted(font_, localization_.get(statistic->labelTextId), Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
            y += 54.f;

            BasicUi::drawTextFitted(font_, std::to_string(statistic->value), Vector2{x, y}, width, 30.f, 22.f, Color{206, 212, 232, 255});
            y += 52.f;

            for (const std::string& line : BasicUi::wrapText(font_, localization_.get(statistic->descriptionTextId), 18.f, width)) {
                BasicUi::drawText(font_, line, Vector2{x, y}, 18.f, Color{206, 212, 232, 255});
                y += 24.f;
            }
        }
    }

}

void ProfileProgressScene::setViewMode(const ViewMode mode) {
    if (viewMode_ == mode) {
        return;
    }

    viewMode_ = mode;
    selectedIndex_ = 0;
}

void ProfileProgressScene::rebuildStatistics() {
    statistics_.clear();
    if (profile_ == nullptr) {
        return;
    }

    const ProfileRunStatistics& stats = profile_->lifetimeRunStats;
    statistics_ = {
        {TextId("progress_log.stat.victories"), TextId("progress_log.stat.victories.desc"), profile_->victories},
        {TextId("progress_log.stat.defeats"), TextId("progress_log.stat.defeats.desc"), profile_->defeats},
        {TextId("progress_log.stat.combats_won"), TextId("progress_log.stat.combats_won.desc"), stats.combatsWon},
        {TextId("progress_log.stat.combats_lost"), TextId("progress_log.stat.combats_lost.desc"), stats.combatsLost},
        {TextId("progress_log.stat.nodes_completed"), TextId("progress_log.stat.nodes_completed.desc"), stats.nodesCompleted},
        {TextId("progress_log.stat.enemies_killed"), TextId("progress_log.stat.enemies_killed.desc"), stats.enemiesKilled},
        {TextId("progress_log.stat.elites_killed"), TextId("progress_log.stat.elites_killed.desc"), stats.elitesKilled},
        {TextId("progress_log.stat.bosses_killed"), TextId("progress_log.stat.bosses_killed.desc"), stats.bossesKilled},
        {TextId("progress_log.stat.events_completed"), TextId("progress_log.stat.events_completed.desc"), stats.eventsCompleted},
        {TextId("progress_log.stat.shops_visited"), TextId("progress_log.stat.shops_visited.desc"), stats.shopsVisited},
        {TextId("progress_log.stat.chests_opened"), TextId("progress_log.stat.chests_opened.desc"), stats.chestsOpened},
        {TextId("progress_log.stat.rests_used"), TextId("progress_log.stat.rests_used.desc"), stats.restsUsed},
        {TextId("progress_log.stat.damage_taken"), TextId("progress_log.stat.damage_taken.desc"), stats.damageTaken},
        {TextId("progress_log.stat.damage_dealt"), TextId("progress_log.stat.damage_dealt.desc"), stats.damageDealt},
        {TextId("progress_log.stat.damage_blocked"), TextId("progress_log.stat.damage_blocked.desc"), stats.damageBlocked},
        {TextId("progress_log.stat.cards_played_in_combat"), TextId("progress_log.stat.cards_played_in_combat.desc"), stats.cardsPlayedInCombat},
        {TextId("progress_log.stat.combat_turns"), TextId("progress_log.stat.combat_turns.desc"), stats.combatTurns},
        {TextId("progress_log.stat.maximum_single_hit"), TextId("progress_log.stat.maximum_single_hit.desc"), stats.maximumSingleHit},
        {TextId("progress_log.stat.gold_gained"), TextId("progress_log.stat.gold_gained.desc"), stats.goldGained},
        {TextId("progress_log.stat.gold_spent"), TextId("progress_log.stat.gold_spent.desc"), stats.goldSpent},
        {TextId("progress_log.stat.cards_added"), TextId("progress_log.stat.cards_added.desc"), stats.cardsAdded},
        {TextId("progress_log.stat.cards_removed"), TextId("progress_log.stat.cards_removed.desc"), stats.cardsRemoved},
        {TextId("progress_log.stat.cards_upgraded"), TextId("progress_log.stat.cards_upgraded.desc"), stats.cardsUpgraded},
        {TextId("progress_log.stat.cards_skipped"), TextId("progress_log.stat.cards_skipped.desc"), stats.cardsSkipped},
        {TextId("progress_log.stat.rewards_skipped"), TextId("progress_log.stat.rewards_skipped.desc"), stats.rewardsSkipped},
        {TextId("progress_log.stat.relics_gained"), TextId("progress_log.stat.relics_gained.desc"), stats.relicsGained},
        {TextId("progress_log.stat.consumables_gained"), TextId("progress_log.stat.consumables_gained.desc"), stats.consumablesGained},
        {TextId("progress_log.stat.consumables_used"), TextId("progress_log.stat.consumables_used.desc"), stats.consumablesUsed},
        {TextId("progress_log.stat.unique_cards_played"), TextId("progress_log.stat.unique_cards_played.desc"), static_cast<int>(profile_->cardPlayCounts.size())},
        {TextId("progress_log.stat.unique_relics_taken"), TextId("progress_log.stat.unique_relics_taken.desc"), static_cast<int>(profile_->relicPickCounts.size())}
    };
}

void ProfileProgressScene::moveSelection(const int direction) {
    const std::size_t count = visibleEntryCount();
    if (count == 0u) {
        return;
    }

    int next = static_cast<int>(selectedIndex_) + direction;
    if (next < 0) {
        next = 0;
    }
    if (next >= static_cast<int>(count)) {
        next = static_cast<int>(count) - 1;
    }
    selectedIndex_ = static_cast<std::size_t>(next);
}

void ProfileProgressScene::pageSelection(const int direction) {
    if (visibleEntryCount() == 0u) {
        return;
    }

    const ProfileProgressSceneLayout layout = calculateProfileProgressSceneLayout();
    const int page = static_cast<int>(visibleRowCount(layout.listPanel));
    moveSelection(direction * page);
}

const ProfileProgressEntry* ProfileProgressScene::selectedEntry() const {
    if (viewMode_ != ViewMode::Journal || entries_.empty()) {
        return nullptr;
    }
    return &entries_.at(selectedIndex_);
}

const ProfileProgressScene::StatisticEntry* ProfileProgressScene::selectedStatistic() const {
    if (viewMode_ != ViewMode::Statistics || statistics_.empty()) {
        return nullptr;
    }
    return &statistics_.at(selectedIndex_);
}

std::size_t ProfileProgressScene::visibleEntryCount() const {
    return viewMode_ == ViewMode::Journal ? entries_.size() : statistics_.size();
}

std::size_t ProfileProgressScene::firstVisibleIndex(const std::size_t visibleRows) const {
    const std::size_t count = visibleEntryCount();
    if (count == 0u || visibleRows == 0u || count <= visibleRows) {
        return 0u;
    }

    if (selectedIndex_ < visibleRows) {
        return 0u;
    }
    return std::min(selectedIndex_ - visibleRows + 1u, count - visibleRows);
}

std::string ProfileProgressScene::typeLabel(const ProfileProgressEntry& entry) const {
    const TextId textId("progress_log.type." + entry.type);
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }
    return localization_.get(TextId("progress_log.type.unknown"));
}

std::string ProfileProgressScene::entryName(const ProfileProgressEntry& entry) const {
    if (isEntryType(entry, "archetype_unlocked") && content_.archetypes().contains(PlayableArchetypeId(entry.contentId))) {
        return localization_.get(content_.archetypes().get(PlayableArchetypeId(entry.contentId)).nameTextId);
    }
    if (isEntryType(entry, "card_unlocked") && content_.cards().contains(CardId(entry.contentId))) {
        return localization_.get(content_.cards().get(CardId(entry.contentId)).nameTextId);
    }
    if (isEntryType(entry, "relic_unlocked") && content_.relics().contains(RelicId(entry.contentId))) {
        return localization_.get(content_.relics().get(RelicId(entry.contentId)).nameTextId);
    }
    if (isEntryType(entry, "enemy_discovered") && content_.enemies().contains(EnemyId(entry.contentId))) {
        return localization_.get(content_.enemies().get(EnemyId(entry.contentId)).nameTextId);
    }
    if (isEntryType(entry, "status_discovered") && content_.statuses().contains(StatusId(entry.contentId))) {
        return localization_.get(content_.statuses().get(StatusId(entry.contentId)).nameTextId);
    }
    if (isEntryType(entry, "consumable_discovered") && content_.consumables().contains(ConsumableId(entry.contentId))) {
        return localization_.get(content_.consumables().get(ConsumableId(entry.contentId)).nameTextId);
    }
    if (isEntryType(entry, "challenge_completed") && content_.challenges().contains(entry.contentId)) {
        return localization_.get(content_.challenges().get(entry.contentId).nameTextId);
    }
    if (isEntryType(entry, "achievement_completed") && content_.achievements().contains(entry.contentId)) {
        return localization_.get(content_.achievements().get(entry.contentId).nameTextId);
    }
    return localization_.format(TextId("progress_log.unknown_entry"), {{"id", entry.contentId}});
}

std::string ProfileProgressScene::entryDescription(const ProfileProgressEntry& entry) const {
    if (isEntryType(entry, "archetype_unlocked") && content_.archetypes().contains(PlayableArchetypeId(entry.contentId))) {
        return localization_.get(content_.archetypes().get(PlayableArchetypeId(entry.contentId)).detailsDescriptionTextId);
    }
    if (isEntryType(entry, "card_unlocked") && content_.cards().contains(CardId(entry.contentId))) {
        return localization_.get(content_.cards().get(CardId(entry.contentId)).descriptionTextId);
    }
    if (isEntryType(entry, "relic_unlocked") && content_.relics().contains(RelicId(entry.contentId))) {
        return localization_.get(content_.relics().get(RelicId(entry.contentId)).descriptionTextId);
    }
    if (isEntryType(entry, "enemy_discovered") && content_.enemies().contains(EnemyId(entry.contentId))) {
        const auto& enemy = content_.enemies().get(EnemyId(entry.contentId));
        return localization_.format(
            TextId("progress_log.enemy_summary"),
            {{"hp", std::to_string(enemy.maxHp)}, {"actions", std::to_string(enemy.actions.size())}}
        );
    }
    if (isEntryType(entry, "status_discovered") && content_.statuses().contains(StatusId(entry.contentId))) {
        return localization_.get(content_.statuses().get(StatusId(entry.contentId)).descriptionTextId);
    }
    if (isEntryType(entry, "consumable_discovered") && content_.consumables().contains(ConsumableId(entry.contentId))) {
        return localization_.get(content_.consumables().get(ConsumableId(entry.contentId)).descriptionTextId);
    }
    if (isEntryType(entry, "challenge_completed") && content_.challenges().contains(entry.contentId)) {
        return localization_.get(content_.challenges().get(entry.contentId).goalTextId);
    }
    if (isEntryType(entry, "achievement_completed") && content_.achievements().contains(entry.contentId)) {
        return localization_.get(content_.achievements().get(entry.contentId).descriptionTextId);
    }
    return localization_.get(TextId("progress_log.missing_content"));
}
