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
    std::string nextFloorName,
    std::string runModeLabel,
    const RunCompletionType completionType,
    const bool canContinueToNextFloor,
    std::function<void()> onContinue,
    std::function<void()> onMainMenu
)
    : font_(font),
      localization_(localization),
      enemies_(enemies),
      relics_(relics),
      run_(run),
      nextFloorName_(std::move(nextFloorName)),
      runModeLabel_(std::move(runModeLabel)),
      completionType_(completionType),
      canContinueToNextFloor_(canContinueToNextFloor),
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
        titleText(),
        Rectangle{0.f, panel.y - 70.f, static_cast<float>(VirtualViewport::width()), 52.f},
        40.f,
        Color{255, 228, 150, 255}
    );

    DrawRectangleRounded(panel, 0.035f, 14, Color{30, 33, 45, 250});
    DrawRectangleRoundedLinesEx(panel, 0.035f, 14, 3.f, Color{210, 172, 82, 255});
    renderRunModeBanner(panel);

    const std::vector<std::pair<std::string, std::string>> rows = statRows();
    constexpr std::size_t columnCount = 3u;
    const std::size_t rowsPerColumn = std::max<std::size_t>(1u, (rows.size() + columnCount - 1u) / columnCount);
    const float top = panel.y + (runModeLabel_.empty() ? 30.f : 68.f);
    const float columnGap = 16.f;
    const float columnWidth = (panel.width - 96.f - columnGap * static_cast<float>(columnCount - 1u)) /
        static_cast<float>(columnCount);
    const float rowHeight = 32.f;
    const float rowGap = 7.f;

    for (std::size_t index = 0; index < rows.size(); ++index) {
        const std::size_t column = index / rowsPerColumn;
        const std::size_t rowIndex = index % rowsPerColumn;
        const Rectangle row{
            panel.x + 48.f + static_cast<float>(column) * (columnWidth + columnGap),
            top + static_cast<float>(rowIndex) * (rowHeight + rowGap),
            columnWidth,
            rowHeight
        };

        DrawRectangleRounded(row, 0.14f, 8, Color{40, 44, 58, 230});
        BasicUi::drawTextFitted(
            font_,
            rows[index].first,
            Vector2{row.x + 9.f, row.y + 8.f},
            row.width * 0.50f,
            14.f,
            11.f,
            Color{170, 178, 204, 255}
        );
        BasicUi::drawTextFitted(
            font_,
            rows[index].second,
            Vector2{row.x + row.width * 0.54f, row.y + 8.f},
            row.width * 0.40f,
            14.f,
            11.f,
            Color{238, 240, 248, 255}
        );
    }

    const float rowsBottom = top + static_cast<float>(rowsPerColumn) * (rowHeight + rowGap);
    BasicUi::drawTextFitted(
        font_,
        relicSummary(),
        Vector2{panel.x + 48.f, rowsBottom + 12.f},
        panel.width - 96.f,
        16.f,
        12.f,
        Color{205, 211, 232, 255}
    );

    const Rectangle status = statusBounds();
    DrawRectangleRounded(status, 0.10f, 10, Color{42, 36, 30, 255});
    DrawRectangleRoundedLinesEx(status, 0.10f, 10, 2.f, Color{190, 132, 70, 230});
    BasicUi::drawCenteredTextFitted(
        font_,
        statusText(),
        Rectangle{status.x + 16.f, status.y + 8.f, status.width - 32.f, status.height - 16.f},
        18.f,
        13.f,
        Color{255, 226, 170, 255}
    );

    BasicUi::drawButton(
        font_,
        continueButtonBounds(),
        localization_.get(TextId(canContinueToNextFloor_ ? "floor_complete.continue_next" : "floor_complete.continue")),
        mouse
    );
    BasicUi::drawButton(
        font_,
        mainMenuButtonBounds(),
        localization_.get(TextId(canContinueToNextFloor_ ? "floor_complete.save_and_exit" : "floor_complete.main_menu")),
        mouse
    );
}

Rectangle FloorCompleteScene::panelBounds() const {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 92.f);
    const float height = std::min(610.f, static_cast<float>(VirtualViewport::height()) - 178.f);
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f + 24.f,
        width,
        height
    };
}

Rectangle FloorCompleteScene::continueButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float spacing = 24.f;
    const float width = std::min(300.f, (panel.width - 112.f - spacing) * 0.5f);
    return Rectangle{panel.x + panel.width * 0.5f - width - spacing * 0.5f, panel.y + panel.height - 66.f, width, 48.f};
}

Rectangle FloorCompleteScene::mainMenuButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float spacing = 24.f;
    const float width = std::min(300.f, (panel.width - 112.f - spacing) * 0.5f);
    return Rectangle{panel.x + panel.width * 0.5f + spacing * 0.5f, panel.y + panel.height - 66.f, width, 48.f};
}

Rectangle FloorCompleteScene::statusBounds() const {
    const Rectangle panel = panelBounds();
    return Rectangle{panel.x + 48.f, panel.y + panel.height - 128.f, panel.width - 96.f, 46.f};
}

void FloorCompleteScene::renderRunModeBanner(const Rectangle panel) const {
    if (runModeLabel_.empty()) {
        return;
    }

    const float width = std::min(560.f, panel.width - 96.f);
    const Rectangle banner{panel.x + (panel.width - width) * 0.5f, panel.y + 22.f, width, 28.f};
    DrawRectangleRounded(banner, 0.28f, 12, Color{58, 42, 82, 235});
    DrawRectangleRoundedLinesEx(banner, 0.28f, 12, 1.5f, Color{194, 152, 236, 230});
    BasicUi::drawCenteredText(font_, runModeLabel_, banner, 15.f, Color{238, 226, 255, 255});
}

bool FloorCompleteScene::isVictoryCompletion() const {
    return completionType_ == RunCompletionType::Victory;
}

bool FloorCompleteScene::isContentComplete() const {
    return completionType_ == RunCompletionType::PlayableContentComplete;
}

bool FloorCompleteScene::isFinalRunCompletion() const {
    return isVictoryCompletion() || isContentComplete();
}

std::string FloorCompleteScene::titleText() const {
    if (isVictoryCompletion()) {
        return localization_.get(TextId("run_complete.title"));
    }
    if (isContentComplete()) {
        return localization_.get(TextId("playable_content_complete.title"));
    }
    return localization_.format(
        TextId("floor_complete.title"),
        {{"act", std::to_string(run_.completedAct > 0 ? run_.completedAct : run_.act)}}
    );
}

std::string FloorCompleteScene::statusText() const {
    if (canContinueToNextFloor_) {
        return localization_.format(TextId("floor_complete.next_floor_ready"), {{"floor", nextFloorName_}});
    }
    if (isVictoryCompletion()) {
        return localization_.get(TextId("run_complete.status"));
    }
    return localization_.get(TextId("playable_content_complete.status_short"));
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
    return names.empty() ? localization_.get(TextId("floor_complete.boss_unknown")) : joinNames(names);
}

std::string FloorCompleteScene::floorProgressSummary() const {
    const int cleared = run_.completedAct > 0 ? run_.completedAct : run_.currentFloorIndex;
    if (isVictoryCompletion()) {
        return localization_.format(TextId("run_complete.floors_value"), {{"cleared", std::to_string(std::max(1, cleared))}});
    }
    if (isContentComplete()) {
        return std::to_string(std::max(1, cleared));
    }
    return localization_.format(
        TextId("floor_complete.floor_progress_value_short"),
        {{"current", std::to_string(std::max(1, cleared))}, {"next", nextFloorName_.empty() ? "?" : nextFloorName_}}
    );
}

std::string FloorCompleteScene::hpSummary() const {
    int current = 0;
    int maximum = 0;
    for (const RunActorState& actor : run_.actorStates) {
        current += std::max(0, actor.currentHp);
        maximum += std::max(0, actor.maxHp);
    }
    return std::to_string(current) + "/" + std::to_string(std::max(1, maximum));
}

std::string FloorCompleteScene::relicSummary() const {
    std::vector<std::string> names;
    names.reserve(run_.relicIds.size());
    for (const std::string& relicId : run_.relicIds) {
        const RelicId id(relicId);
        if (relics_.contains(id)) {
            names.push_back(localization_.get(relics_.get(id).nameTextId));
        }
    }
    return names.empty() ? localization_.get(TextId("floor_complete.no_relics")) : joinNames(names);
}

std::string FloorCompleteScene::consumableSummary() const {
    return std::to_string(run_.consumableIds.size()) + "/" + std::to_string(std::max(0, run_.maxConsumables));
}

std::vector<std::pair<std::string, std::string>> FloorCompleteScene::statRows() const {
    return {
        {localization_.get(TextId(isFinalRunCompletion() ? "run_complete.floors_label" : "floor_complete.floor_progress_label")), floorProgressSummary()},
        {localization_.get(TextId("floor_complete.boss_label")), bossSummary()},
        {localization_.get(TextId("floor_complete.hp_label")), hpSummary()},
        {localization_.get(TextId("floor_complete.gold_label")), number(run_.gold)},
        {localization_.get(TextId("floor_complete.deck_label")), number(static_cast<int>(run_.deckCardIds.size()))},
        {localization_.get(TextId("floor_complete.relics_label")), number(static_cast<int>(run_.relicIds.size()))},
        {localization_.get(TextId("floor_complete.consumables_label")), consumableSummary()},
        {localization_.get(TextId("floor_complete.combats_label")), number(run_.stats.combatsWon)},
        {localization_.get(TextId("floor_complete.enemies_label")), number(run_.stats.enemiesKilled)},
        {localization_.get(TextId("floor_complete.elites_label")), number(run_.stats.elitesKilled)},
        {localization_.get(TextId("floor_complete.damage_taken_label")), number(run_.stats.damageTaken)},
        {localization_.get(TextId("floor_complete.cards_upgraded_label")), number(run_.stats.cardsUpgraded)},
        {localization_.get(TextId("floor_complete.nodes_label")), number(run_.stats.nodesCompleted)}
    };
}
