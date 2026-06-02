#include "RunDefeatScene.hpp"
#include "ui/VirtualViewport.hpp"

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

RunDefeatScene::RunDefeatScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const RelicDatabase& relics,
    const RunState& run,
    std::function<void()> onProfileHub,
    std::function<void()> onMainMenu
)
    : font_(font),
      localization_(localization),
      relics_(relics),
      run_(run),
      onProfileHub_(std::move(onProfileHub)),
      onMainMenu_(std::move(onMainMenu)) {}

void RunDefeatScene::update(float) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
        onProfileHub_();
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        onMainMenu_();
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(profileHubButtonBounds(), mouse)) {
        onProfileHub_();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(mainMenuButtonBounds(), mouse)) {
        onMainMenu_();
    }
}

void RunDefeatScene::render() const {
    const Rectangle panel = panelBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{10, 8, 12, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("run_defeat.title")),
        Rectangle{0.f, panel.y - 82.f, static_cast<float>(VirtualViewport::width()), 58.f},
        42.f,
        Color{238, 116, 116, 255}
    );

    DrawRectangleRounded(panel, 0.035f, 14, Color{31, 27, 37, 250});
    DrawRectangleRoundedLinesEx(panel, 0.035f, 14, 3.f, Color{160, 72, 72, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("run_defeat.subtitle")),
        Rectangle{panel.x + 32.f, panel.y + 24.f, panel.width - 64.f, 40.f},
        28.f,
        Color{238, 240, 248, 255}
    );

    const std::vector<std::string> descriptionLines = BasicUi::wrapText(
        font_,
        localization_.get(TextId("run_defeat.description")),
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
        localization_.get(TextId("run_defeat.summary_title")),
        Vector2{panel.x + 48.f, y},
        24.f,
        Color{238, 116, 116, 255}
    );
    y += 38.f;

    const std::vector<std::pair<std::string, std::string>> rows = statRows();
    const std::size_t firstColumnCount = (rows.size() + 1u) / 2u;
    const float columnGap = 24.f;
    const float columnWidth = (panel.width - 96.f - columnGap) * 0.5f;
    const float rowHeight = 29.f;
    const float rowGap = 5.f;

    auto drawRow = [&](const Rectangle row, const std::string& label, const std::string& value) {
        DrawRectangleRounded(row, 0.16f, 8, Color{43, 38, 52, 230});
        BasicUi::drawTextFitted(
            font_,
            label,
            Vector2{row.x + 12.f, row.y + 7.f},
            row.width * 0.46f,
            16.f,
            13.f,
            Color{178, 168, 196, 255}
        );
        BasicUi::drawTextFitted(
            font_,
            value,
            Vector2{row.x + row.width * 0.52f, row.y + 7.f},
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
        localization_.get(TextId("run_defeat.relic_list_label")),
        Vector2{panel.x + 48.f, y},
        20.f,
        Color{238, 116, 116, 255}
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
        localization_.get(TextId("run_defeat.save_cleanup_note")),
        16.f,
        panel.width - 96.f
    );
    for (const std::string& line : nextLines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 48.f, y}, 16.f, Color{190, 166, 112, 255});
        y += 21.f;
    }

    BasicUi::drawButton(font_, profileHubButtonBounds(), localization_.get(TextId("run_defeat.profile_hub")), mouse);
    BasicUi::drawButton(font_, mainMenuButtonBounds(), localization_.get(TextId("run_defeat.main_menu")), mouse);
}

Rectangle RunDefeatScene::panelBounds() const {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 92.f);
    const float height = std::min(780.f, static_cast<float>(VirtualViewport::height()) - 160.f);
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f + 36.f,
        width,
        height
    };
}

Rectangle RunDefeatScene::profileHubButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float spacing = 24.f;
    const float width = std::min(300.f, (panel.width - 112.f - spacing) * 0.5f);
    return Rectangle{panel.x + panel.width * 0.5f - width - spacing * 0.5f, panel.y + panel.height - 72.f, width, 52.f};
}

Rectangle RunDefeatScene::mainMenuButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float spacing = 24.f;
    const float width = std::min(300.f, (panel.width - 112.f - spacing) * 0.5f);
    return Rectangle{panel.x + panel.width * 0.5f + spacing * 0.5f, panel.y + panel.height - 72.f, width, 52.f};
}

std::string RunDefeatScene::hpSummary() const {
    int current = 0;
    int maximum = 0;
    for (const RunActorState& actor : run_.actorStates) {
        current += std::max(0, actor.currentHp);
        maximum += std::max(0, actor.maxHp);
    }

    return localization_.format(
        TextId("run_defeat.hp_value"),
        {{"current", std::to_string(current)}, {"maximum", std::to_string(std::max(1, maximum))}}
    );
}

std::string RunDefeatScene::relicSummary() const {
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
        return localization_.get(TextId("run_defeat.no_relics"));
    }

    return joinNames(names);
}

std::string RunDefeatScene::consumableSummary() const {
    return localization_.format(
        TextId("run_defeat.consumables_value"),
        {{"current", std::to_string(run_.consumableIds.size())}, {"maximum", std::to_string(std::max(0, run_.maxConsumables))}}
    );
}

std::vector<std::pair<std::string, std::string>> RunDefeatScene::statRows() const {
    return {
        {localization_.get(TextId("run_defeat.act_label")), number(run_.act)},
        {localization_.get(TextId("run_defeat.seed_label")), std::to_string(run_.seed)},
        {localization_.get(TextId("run_defeat.hp_label")), hpSummary()},
        {localization_.get(TextId("run_defeat.gold_label")), number(run_.gold)},
        {localization_.get(TextId("run_defeat.deck_label")), number(static_cast<int>(run_.deckCardIds.size()))},
        {localization_.get(TextId("run_defeat.relics_label")), number(static_cast<int>(run_.relicIds.size()))},
        {localization_.get(TextId("run_defeat.consumables_label")), consumableSummary()},
        {localization_.get(TextId("run_defeat.rooms_label")), number(run_.stats.nodesCompleted)},
        {localization_.get(TextId("run_defeat.combats_won_label")), number(run_.stats.combatsWon)},
        {localization_.get(TextId("run_defeat.combats_lost_label")), number(run_.stats.combatsLost)},
        {localization_.get(TextId("run_defeat.enemies_label")), number(run_.stats.enemiesKilled)},
        {localization_.get(TextId("run_defeat.elites_label")), number(run_.stats.elitesKilled)},
        {localization_.get(TextId("run_defeat.damage_taken_label")), number(run_.stats.damageTaken)},
        {localization_.get(TextId("run_defeat.gold_gained_label")), number(run_.stats.goldGained)},
        {localization_.get(TextId("run_defeat.gold_spent_label")), number(run_.stats.goldSpent)},
        {localization_.get(TextId("run_defeat.cards_added_label")), number(run_.stats.cardsAdded)},
        {localization_.get(TextId("run_defeat.cards_removed_label")), number(run_.stats.cardsRemoved)},
        {localization_.get(TextId("run_defeat.cards_upgraded_label")), number(run_.stats.cardsUpgraded)},
        {localization_.get(TextId("run_defeat.consumables_used_label")), number(run_.stats.consumablesUsed)}
    };
}
