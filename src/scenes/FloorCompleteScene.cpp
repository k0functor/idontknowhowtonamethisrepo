#include "FloorCompleteScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "enemies/EnemyDefinition.hpp"
#include "enemies/EnemyId.hpp"
#include "relics/RelicDefinition.hpp"
#include "relics/RelicId.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

namespace {
std::string joinNames(const std::vector<std::string>& names) {
    std::string result;
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (index > 0) {
            result += ", ";
        }
        result += names[index];
    }
    return result;
}

std::string number(const int value) {
    return std::to_string(value);
}
}

FloorCompleteScene::FloorCompleteScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const EnemyDatabase& enemies,
    const RelicDatabase& relics,
    const RunState& run,
    std::function<void()> onContinue,
    std::function<void()> onMainMenu
)
    : font_(font),
      localization_(localization),
      enemies_(enemies),
      relics_(relics),
      run_(run),
      onContinue_(std::move(onContinue)),
      onMainMenu_(std::move(onMainMenu)) {}

void FloorCompleteScene::update(float) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
        onContinue_();
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        onMainMenu_();
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(continueButtonBounds(), mouse)) {
        onContinue_();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(mainMenuButtonBounds(), mouse)) {
        onMainMenu_();
    }
}

void FloorCompleteScene::render() const {
    const Rectangle panel = panelBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{10, 12, 18, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.format(TextId("floor_complete.title"), {{"act", std::to_string(run_.completedAct > 0 ? run_.completedAct : run_.act)}}),
        Rectangle{0.f, panel.y - 82.f, static_cast<float>(VirtualViewport::width()), 58.f},
        42.f,
        Color{255, 228, 150, 255}
    );

    DrawRectangleRounded(panel, 0.035f, 14, Color{30, 33, 45, 250});
    DrawRectangleRoundedLinesEx(panel, 0.035f, 14, 3.f, Color{210, 172, 82, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("floor_complete.subtitle")),
        Rectangle{panel.x + 32.f, panel.y + 24.f, panel.width - 64.f, 40.f},
        28.f,
        Color{238, 240, 248, 255}
    );

    const std::vector<std::string> descriptionLines = BasicUi::wrapText(
        font_,
        localization_.get(TextId("floor_complete.description")),
        18.f,
        panel.width - 96.f
    );

    float y = panel.y + 78.f;
    for (const std::string& line : descriptionLines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 48.f, y}, 18.f, Color{190, 198, 220, 255});
        y += 24.f;
    }

    y += 16.f;
    BasicUi::drawText(
        font_,
        localization_.get(TextId("floor_complete.summary_title")),
        Vector2{panel.x + 48.f, y},
        24.f,
        Color{255, 228, 150, 255}
    );
    y += 38.f;

    const std::vector<std::pair<std::string, std::string>> rows = statRows();
    const std::size_t firstColumnCount = (rows.size() + 1u) / 2u;
    const float columnGap = 24.f;
    const float columnWidth = (panel.width - 96.f - columnGap) * 0.5f;
    const float rowHeight = 34.f;
    const float rowGap = 7.f;

    auto drawRow = [&](const Rectangle row, const std::string& label, const std::string& value) {
        DrawRectangleRounded(row, 0.16f, 8, Color{40, 44, 58, 230});
        BasicUi::drawTextFitted(
            font_,
            label,
            Vector2{row.x + 12.f, row.y + 8.f},
            row.width * 0.46f,
            16.f,
            13.f,
            Color{170, 178, 204, 255}
        );
        BasicUi::drawTextFitted(
            font_,
            value,
            Vector2{row.x + row.width * 0.52f, row.y + 8.f},
            row.width * 0.45f,
            16.f,
            13.f,
            Color{238, 240, 248, 255}
        );
    };

    for (std::size_t index = 0; index < rows.size(); ++index) {
        const std::size_t column = index < firstColumnCount ? 0u : 1u;
        const std::size_t rowIndex = column == 0u ? index : index - firstColumnCount;
        const Rectangle row{
            panel.x + 48.f + static_cast<float>(column) * (columnWidth + columnGap),
            y + static_cast<float>(rowIndex) * (rowHeight + rowGap),
            columnWidth,
            rowHeight
        };
        drawRow(row, rows[index].first, rows[index].second);
    }

    y += static_cast<float>(firstColumnCount) * (rowHeight + rowGap) + 16.f;

    BasicUi::drawText(
        font_,
        localization_.get(TextId("floor_complete.relic_list_label")),
        Vector2{panel.x + 48.f, y},
        20.f,
        Color{255, 228, 150, 255}
    );
    y += 30.f;

    const std::vector<std::string> relicLines = BasicUi::wrapText(font_, relicSummary(), 17.f, panel.width - 96.f);
    for (const std::string& line : relicLines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 48.f, y}, 17.f, Color{205, 211, 232, 255});
        y += 22.f;
    }

    y += 14.f;
    const std::vector<std::string> nextLines = BasicUi::wrapText(
        font_,
        localization_.get(TextId("floor_complete.save_cleanup_note")),
        16.f,
        panel.width - 96.f
    );
    for (const std::string& line : nextLines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 48.f, y}, 16.f, Color{190, 166, 112, 255});
        y += 21.f;
    }

    BasicUi::drawButton(font_, continueButtonBounds(), localization_.get(TextId("floor_complete.continue")), mouse);
    BasicUi::drawButton(font_, mainMenuButtonBounds(), localization_.get(TextId("floor_complete.main_menu")), mouse);
}

Rectangle FloorCompleteScene::panelBounds() const {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 92.f);
    const float height = std::min(780.f, static_cast<float>(VirtualViewport::height()) - 160.f);
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f + 36.f,
        width,
        height
    };
}

Rectangle FloorCompleteScene::continueButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float spacing = 24.f;
    const float width = std::min(300.f, (panel.width - 112.f - spacing) * 0.5f);
    return Rectangle{panel.x + panel.width * 0.5f - width - spacing * 0.5f, panel.y + panel.height - 72.f, width, 52.f};
}

Rectangle FloorCompleteScene::mainMenuButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float spacing = 24.f;
    const float width = std::min(300.f, (panel.width - 112.f - spacing) * 0.5f);
    return Rectangle{panel.x + panel.width * 0.5f + spacing * 0.5f, panel.y + panel.height - 72.f, width, 52.f};
}

std::string FloorCompleteScene::bossSummary() const {
    std::vector<std::string> names;
    for (const std::string& bossId : run_.defeatedBossEnemyIds) {
        const EnemyId id(bossId);
        if (enemies_.contains(id)) {
            names.push_back(localization_.get(enemies_.get(id).nameTextId));
        } else if (!bossId.empty()) {
            names.push_back(bossId);
        }
    }

    if (names.empty()) {
        return localization_.get(TextId("floor_complete.boss_unknown"));
    }

    return joinNames(names);
}

std::string FloorCompleteScene::hpSummary() const {
    int current = 0;
    int maximum = 0;
    for (const RunActorState& actor : run_.actorStates) {
        current += std::max(0, actor.currentHp);
        maximum += std::max(0, actor.maxHp);
    }

    return localization_.format(
        TextId("floor_complete.hp_value"),
        {{"current", std::to_string(current)}, {"maximum", std::to_string(std::max(1, maximum))}}
    );
}

std::string FloorCompleteScene::relicSummary() const {
    std::vector<std::string> names;
    names.reserve(run_.relicIds.size());

    for (const std::string& relicId : run_.relicIds) {
        const RelicId id(relicId);
        if (relics_.contains(id)) {
            names.push_back(localization_.get(relics_.get(id).nameTextId));
        } else if (!relicId.empty()) {
            names.push_back(relicId);
        }
    }

    if (names.empty()) {
        return localization_.get(TextId("floor_complete.no_relics"));
    }

    return joinNames(names);
}

std::string FloorCompleteScene::consumableSummary() const {
    return localization_.format(
        TextId("floor_complete.consumables_value"),
        {{"current", std::to_string(run_.consumableIds.size())}, {"maximum", std::to_string(std::max(0, run_.maxConsumables))}}
    );
}

std::vector<std::pair<std::string, std::string>> FloorCompleteScene::statRows() const {
    return {
        {localization_.get(TextId("floor_complete.boss_label")), bossSummary()},
        {localization_.get(TextId("floor_complete.hp_label")), hpSummary()},
        {localization_.get(TextId("floor_complete.gold_label")), number(run_.gold)},
        {localization_.get(TextId("floor_complete.deck_label")), number(static_cast<int>(run_.deckCardIds.size()))},
        {localization_.get(TextId("floor_complete.relics_label")), number(static_cast<int>(run_.relicIds.size()))},
        {localization_.get(TextId("floor_complete.consumables_label")), consumableSummary()},
        {localization_.get(TextId("floor_complete.combats_label")), number(run_.stats.combatsWon)},
        {localization_.get(TextId("floor_complete.elites_label")), number(run_.stats.elitesKilled)},
        {localization_.get(TextId("floor_complete.events_label")), number(run_.stats.eventsCompleted)},
        {localization_.get(TextId("floor_complete.shops_label")), number(run_.stats.shopsVisited)},
        {localization_.get(TextId("floor_complete.chests_label")), number(run_.stats.chestsOpened)},
        {localization_.get(TextId("floor_complete.rests_label")), number(run_.stats.restsUsed)},
        {localization_.get(TextId("floor_complete.gold_gained_label")), number(run_.stats.goldGained)},
        {localization_.get(TextId("floor_complete.gold_spent_label")), number(run_.stats.goldSpent)},
        {localization_.get(TextId("floor_complete.cards_added_label")), number(run_.stats.cardsAdded)},
        {localization_.get(TextId("floor_complete.cards_removed_label")), number(run_.stats.cardsRemoved)},
        {localization_.get(TextId("floor_complete.cards_upgraded_label")), number(run_.stats.cardsUpgraded)},
        {localization_.get(TextId("floor_complete.nodes_label")), number(run_.stats.nodesCompleted)}
    };
}
