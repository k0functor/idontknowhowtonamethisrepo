#include "RunCompleteScene.hpp"

#include "relics/RelicDefinition.hpp"
#include "relics/RelicId.hpp"
#include "ui/BasicUi.hpp"
#include "ui/VirtualViewport.hpp"

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

RunCompleteScene::RunCompleteScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const ContentRegistry& content,
    RunState run,
    const RunEndReason reason,
    std::function<void()> onProfileHub
)
    : font_(font),
      localization_(localization),
      content_(content),
      run_(std::move(run)),
      reason_(reason),
      onProfileHub_(std::move(onProfileHub)) {}

void RunCompleteScene::update(float) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
        onProfileHub_();
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(profileHubButtonBounds(), mouse)) {
        onProfileHub_();
    }
}

void RunCompleteScene::render() const {
    const Rectangle panel = panelBounds();
    const Vector2 mouse = GetMousePosition();
    const bool victory = runEndReasonCountsAsVictory(reason_);
    const Color accent = victory ? Color{226, 188, 92, 255} : Color{210, 104, 104, 255};

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{10, 11, 17, 255});
    BasicUi::drawCenteredText(
        font_,
        titleText(),
        Rectangle{0.f, panel.y - 78.f, static_cast<float>(VirtualViewport::width()), 56.f},
        42.f,
        accent
    );

    DrawRectangleRounded(panel, 0.035f, 14, Color{29, 32, 43, 250});
    DrawRectangleRoundedLinesEx(panel, 0.035f, 14, 3.f, accent);

    float y = panel.y + 26.f;
    const std::string mode = runModeText();
    if (!mode.empty()) {
        BasicUi::drawCenteredText(
            font_,
            mode,
            Rectangle{panel.x + 40.f, y, panel.width - 80.f, 34.f},
            22.f,
            Color{211, 188, 238, 255}
        );
        y += 48.f;
    }

    const std::vector<std::pair<std::string, std::string>> rows = statRows();
    const std::size_t columnCount = 3u;
    const std::size_t rowsPerColumn = (rows.size() + columnCount - 1u) / columnCount;
    const float columnGap = 16.f;
    const float columnWidth = (panel.width - 96.f - columnGap * static_cast<float>(columnCount - 1u)) /
        static_cast<float>(columnCount);
    const float rowHeight = 28.f;
    const float rowGap = 5.f;

    for (std::size_t index = 0; index < rows.size(); ++index) {
        const std::size_t column = index / rowsPerColumn;
        const std::size_t rowIndex = index % rowsPerColumn;
        const Rectangle row{
            panel.x + 48.f + static_cast<float>(column) * (columnWidth + columnGap),
            y + static_cast<float>(rowIndex) * (rowHeight + rowGap),
            columnWidth,
            rowHeight
        };

        DrawRectangleRounded(row, 0.16f, 8, Color{40, 44, 58, 230});
        BasicUi::drawTextFitted(
            font_,
            rows[index].first,
            Vector2{row.x + 9.f, row.y + 7.f},
            row.width * 0.50f,
            14.f,
            11.f,
            Color{170, 178, 204, 255}
        );
        BasicUi::drawTextFitted(
            font_,
            rows[index].second,
            Vector2{row.x + row.width * 0.54f, row.y + 7.f},
            row.width * 0.40f,
            14.f,
            11.f,
            Color{238, 240, 248, 255}
        );
    }

    y += static_cast<float>(rowsPerColumn) * (rowHeight + rowGap) + 16.f;
    BasicUi::drawText(
        font_,
        localization_.get(TextId("run_complete.relics")),
        Vector2{panel.x + 48.f, y},
        18.f,
        accent
    );
    BasicUi::drawTextFitted(
        font_,
        relicSummary(),
        Vector2{panel.x + 48.f, y + 25.f},
        panel.width - 96.f,
        16.f,
        12.f,
        Color{205, 211, 232, 255}
    );

    BasicUi::drawButton(
        font_,
        profileHubButtonBounds(),
        localization_.get(TextId("run_complete.profile_hub")),
        mouse
    );
}

Rectangle RunCompleteScene::panelBounds() const {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 92.f);
    const float height = std::min(690.f, static_cast<float>(VirtualViewport::height()) - 190.f);
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f + 28.f,
        width,
        height
    };
}

Rectangle RunCompleteScene::profileHubButtonBounds() const {
    const Rectangle panel = panelBounds();
    const float width = std::min(320.f, panel.width - 96.f);
    return Rectangle{
        panel.x + panel.width * 0.5f - width * 0.5f,
        panel.y + panel.height - 70.f,
        width,
        50.f
    };
}

std::string RunCompleteScene::titleText() const {
    switch (reason_) {
        case RunEndReason::Victory:
            return localization_.get(TextId("run_complete.victory"));
        case RunEndReason::ChallengeCompleted:
            return localization_.get(TextId("run_complete.challenge_completed"));
        case RunEndReason::ChallengeFailed:
            return localization_.get(TextId("run_complete.challenge_failed"));
        case RunEndReason::Abandoned:
            return localization_.get(TextId("run_complete.abandoned"));
        case RunEndReason::Defeat:
            return localization_.get(TextId("run_complete.defeat"));
    }
    return localization_.get(TextId("run_complete.title"));
}

std::string RunCompleteScene::runModeText() const {
    if (run_.challengeId.empty() || !content_.challenges().contains(run_.challengeId)) {
        return {};
    }
    return localization_.get(content_.challenges().get(run_.challengeId).nameTextId);
}

std::string RunCompleteScene::archetypeName() const {
    if (content_.archetypes().contains(run_.archetypeId)) {
        return localization_.get(content_.archetypes().get(run_.archetypeId).nameTextId);
    }
    return run_.archetypeId.value;
}

std::string RunCompleteScene::difficultyName() const {
    if (content_.difficulties().contains(run_.difficultyId)) {
        return localization_.get(content_.difficulties().get(run_.difficultyId).nameTextId);
    }
    return run_.difficultyId.value;
}

std::string RunCompleteScene::floorName() const {
    if (content_.floors().contains(run_.currentFloorId)) {
        return localization_.get(TextId(content_.floors().get(run_.currentFloorId).nameTextId));
    }
    return run_.currentFloorId;
}

std::string RunCompleteScene::relicSummary() const {
    std::vector<std::string> names;
    names.reserve(run_.relicIds.size());
    for (const std::string& relicId : run_.relicIds) {
        const RelicId id(relicId);
        if (content_.relics().contains(id)) {
            names.push_back(localization_.get(content_.relics().get(id).nameTextId));
        }
    }

    return names.empty()
        ? localization_.get(TextId("run_complete.none"))
        : joinNames(names);
}

std::string RunCompleteScene::hpSummary() const {
    int current = 0;
    int maximum = 0;
    for (const RunActorState& actor : run_.actorStates) {
        current += std::max(0, actor.currentHp);
        maximum += std::max(0, actor.maxHp);
    }
    return std::to_string(current) + "/" + std::to_string(std::max(1, maximum));
}

std::vector<std::pair<std::string, std::string>> RunCompleteScene::statRows() const {
    return {
        {localization_.get(TextId("run_complete.archetype")), archetypeName()},
        {localization_.get(TextId("run_complete.difficulty")), difficultyName()},
        {localization_.get(TextId("run_complete.floor")), floorName()},
        {localization_.get(TextId("run_complete.hp")), hpSummary()},
        {localization_.get(TextId("run_complete.gold")), number(run_.gold)},
        {localization_.get(TextId("run_complete.deck")), number(static_cast<int>(run_.deckCardIds.size()))},
        {localization_.get(TextId("run_complete.rooms")), number(run_.stats.nodesCompleted)},
        {localization_.get(TextId("run_complete.enemies")), number(run_.stats.enemiesKilled)},
        {localization_.get(TextId("run_complete.elites")), number(run_.stats.elitesKilled)},
        {localization_.get(TextId("run_complete.bosses")), number(run_.stats.bossesKilled)},
        {localization_.get(TextId("run_complete.damage")), number(run_.stats.damageTaken)},
        {localization_.get(TextId("run_complete.damage_dealt")), number(run_.stats.damageDealt)},
        {localization_.get(TextId("run_complete.damage_blocked")), number(run_.stats.damageBlocked)},
        {localization_.get(TextId("run_complete.cards_played")), number(run_.stats.cardsPlayedInCombat)},
        {localization_.get(TextId("run_complete.cards_upgraded")), number(run_.stats.cardsUpgraded)}
    };
}
