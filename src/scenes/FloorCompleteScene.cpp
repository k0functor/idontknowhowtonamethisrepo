#include "FloorCompleteScene.hpp"

#include "enemies/EnemyDefinition.hpp"
#include "enemies/EnemyId.hpp"
#include "ui/BasicUi.hpp"

#include <raylib.h>

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

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
}

FloorCompleteScene::FloorCompleteScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const EnemyDatabase& enemies,
    const RunState& run,
    std::function<void()> onContinue
)
    : font_(font),
      localization_(localization),
      enemies_(enemies),
      run_(run),
      onContinue_(std::move(onContinue)) {}

void FloorCompleteScene::update(float) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
        onContinue_();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(continueButtonBounds(), GetMousePosition())) {
        onContinue_();
    }
}

void FloorCompleteScene::render() const {
    const Rectangle panel = panelBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{10, 12, 18, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.format(TextId("floor_complete.title"), {{"act", std::to_string(run_.completedAct > 0 ? run_.completedAct : run_.act)}}),
        Rectangle{0.f, panel.y - 92.f, static_cast<float>(GetScreenWidth()), 58.f},
        42.f,
        Color{255, 228, 150, 255}
    );

    DrawRectangleRounded(panel, 0.045f, 14, Color{30, 33, 45, 250});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{210, 172, 82, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("floor_complete.subtitle")),
        Rectangle{panel.x + 28.f, panel.y + 26.f, panel.width - 56.f, 42.f},
        27.f,
        Color{238, 240, 248, 255}
    );

    const std::vector<std::string> descriptionLines = BasicUi::wrapText(
        font_,
        localization_.get(TextId("floor_complete.description")),
        18.f,
        panel.width - 80.f
    );

    float y = panel.y + 84.f;
    for (const std::string& line : descriptionLines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 40.f, y}, 18.f, Color{190, 198, 220, 255});
        y += 24.f;
    }

    y += 18.f;
    const std::vector<std::pair<std::string, std::string>> rows = statRows();
    for (const auto& [label, value] : rows) {
        const Rectangle row{panel.x + 42.f, y, panel.width - 84.f, 38.f};
        DrawRectangleRounded(row, 0.16f, 8, Color{40, 44, 58, 230});
        BasicUi::drawText(font_, label, Vector2{row.x + 16.f, row.y + 9.f}, 18.f, Color{170, 178, 204, 255});
        BasicUi::drawText(font_, value, Vector2{row.x + row.width * 0.54f, row.y + 9.f}, 18.f, Color{238, 240, 248, 255});
        y += 45.f;
    }

    const std::vector<std::string> nextLines = BasicUi::wrapText(
        font_,
        localization_.get(TextId("floor_complete.next_floor_placeholder")),
        17.f,
        panel.width - 80.f
    );

    y += 12.f;
    for (const std::string& line : nextLines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 40.f, y}, 17.f, Color{190, 166, 112, 255});
        y += 23.f;
    }

    BasicUi::drawButton(font_, continueButtonBounds(), localization_.get(TextId("floor_complete.return_to_hub")), mouse);
}

Rectangle FloorCompleteScene::panelBounds() const {
    const float width = std::min(760.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = std::min(620.f, static_cast<float>(GetScreenHeight()) - 150.f);
    return Rectangle{
        static_cast<float>(GetScreenWidth()) * 0.5f - width * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f - height * 0.5f + 30.f,
        width,
        height
    };
}

Rectangle FloorCompleteScene::continueButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float width = std::min(360.f, panel.width - 90.f);
    return Rectangle{panel.x + panel.width * 0.5f - width * 0.5f, panel.y + panel.height - 72.f, width, 52.f};
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

std::vector<std::pair<std::string, std::string>> FloorCompleteScene::statRows() const {
    return {
        {localization_.get(TextId("floor_complete.boss_label")), bossSummary()},
        {localization_.get(TextId("floor_complete.hp_label")), hpSummary()},
        {localization_.get(TextId("floor_complete.gold_label")), std::to_string(run_.gold)},
        {localization_.get(TextId("floor_complete.deck_label")), std::to_string(run_.deckCardIds.size())},
        {localization_.get(TextId("floor_complete.relics_label")), std::to_string(run_.relicIds.size())},
        {localization_.get(TextId("floor_complete.combats_label")), std::to_string(run_.stats.combatsWon)},
        {localization_.get(TextId("floor_complete.elites_label")), std::to_string(run_.stats.elitesKilled)},
        {localization_.get(TextId("floor_complete.nodes_label")), std::to_string(run_.stats.nodesCompleted)}
    };
}
