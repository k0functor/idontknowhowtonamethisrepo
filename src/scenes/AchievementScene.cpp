#include "AchievementScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "achievements/AchievementRules.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include <raylib.h>

namespace {
struct AchievementSceneLayout {
    Rectangle backButton{};
    Rectangle title{};
    Rectangle filterAll{};
    Rectangle filterAvailable{};
    Rectangle filterCompleted{};
    Rectangle filterLocked{};
    Rectangle listPanel{};
    Rectangle detailsPanel{};
};

AchievementSceneLayout calculateAchievementSceneLayout() {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    AchievementSceneLayout layout;
    layout.backButton = Rectangle{32.f, 32.f, 140.f, 48.f};
    layout.title = Rectangle{0.f, 52.f, screenWidth, 54.f};
    const float margin = 64.f;
    const float filterTop = 116.f;
    const float filterHeight = 42.f;
    const float filterGap = 12.f;
    const float filterWidth = std::clamp((screenWidth - 2.f * margin - 3.f * filterGap) / 4.f, 145.f, 240.f);
    const float filterTotalWidth = 4.f * filterWidth + 3.f * filterGap;
    const float filterStartX = std::max(margin, screenWidth * 0.5f - filterTotalWidth * 0.5f);

    layout.filterAll = Rectangle{filterStartX, filterTop, filterWidth, filterHeight};
    layout.filterAvailable = Rectangle{layout.filterAll.x + filterWidth + filterGap, filterTop, filterWidth, filterHeight};
    layout.filterCompleted = Rectangle{layout.filterAvailable.x + filterWidth + filterGap, filterTop, filterWidth, filterHeight};
    layout.filterLocked = Rectangle{layout.filterCompleted.x + filterWidth + filterGap, filterTop, filterWidth, filterHeight};

    const float top = filterTop + filterHeight + 20.f;
    const float gap = 24.f;
    const float height = std::max(330.f, screenHeight - top - 36.f);
    const float listWidth = std::clamp(screenWidth * 0.38f, 420.f, 560.f);
    layout.listPanel = Rectangle{margin, top, listWidth, height};
    layout.detailsPanel = Rectangle{
        layout.listPanel.x + layout.listPanel.width + gap,
        top,
        screenWidth - margin - (layout.listPanel.x + layout.listPanel.width + gap),
        height
    };
    return layout;
}

Rectangle achievementRowBounds(const Rectangle listPanel, const std::size_t visibleIndex) {
    constexpr float rowHeight = 86.f;
    constexpr float rowGap = 12.f;
    return Rectangle{
        listPanel.x + 18.f,
        listPanel.y + 18.f + static_cast<float>(visibleIndex) * (rowHeight + rowGap),
        listPanel.width - 36.f,
        rowHeight
    };
}

std::size_t visibleRowCount(const Rectangle listPanel) {
    constexpr float rowHeight = 86.f;
    constexpr float rowGap = 12.f;
    const float availableHeight = std::max(0.f, listPanel.height - 36.f + rowGap);
    return static_cast<std::size_t>(std::max(1.f, std::floor(availableHeight / (rowHeight + rowGap))));
}

bool containsId(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

BasicUi::ButtonStyle filterButtonStyle(const bool active) {
    BasicUi::ButtonStyle style;
    style.background = active ? Color{74, 78, 102, 255} : Color{45, 48, 58, 255};
    style.hoveredBackground = active ? Color{88, 94, 124, 255} : Color{62, 66, 82, 255};
    style.border = active ? Color{214, 184, 85, 255} : Color{130, 136, 160, 255};
    style.text = active ? Color{252, 240, 198, 255} : Color{235, 235, 242, 255};
    return style;
}
}

AchievementScene::AchievementScene(
    const UiFont& font,
    const LocalizationManager& localization,
    std::vector<const AchievementDefinition*> achievements,
    const ProfileData* profile,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      achievements_(std::move(achievements)),
      profile_(profile),
      onBack_(std::move(onBack)) {
    rebuildFilteredAchievements();
}

void AchievementScene::update(float) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        onBack_();
        return;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        moveSelection(-1);
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        moveSelection(1);
    }

    const AchievementSceneLayout layout = calculateAchievementSceneLayout();
    const Vector2 mouse = GetMousePosition();

    if (BasicUi::contains(layout.listPanel, mouse)) {
        const float wheel = GetMouseWheelMove();
        if (wheel > 0.f) {
            moveSelection(-1);
        } else if (wheel < 0.f) {
            moveSelection(1);
        }
    }

    if (BasicUi::contains(layout.backButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
        return;
    }

    if (BasicUi::contains(layout.filterAll, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(AchievementFilter::All);
        return;
    }

    if (BasicUi::contains(layout.filterAvailable, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(AchievementFilter::Available);
        return;
    }

    if (BasicUi::contains(layout.filterCompleted, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(AchievementFilter::Completed);
        return;
    }

    if (BasicUi::contains(layout.filterLocked, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(AchievementFilter::Locked);
        return;
    }

    const std::size_t rows = visibleRowCount(layout.listPanel);
    const std::size_t first = firstVisibleIndex(rows);
    for (std::size_t visible = 0; visible < rows; ++visible) {
        const std::size_t index = first + visible;
        if (index >= filteredAchievementIndices_.size()) {
            break;
        }

        if (BasicUi::contains(achievementRowBounds(layout.listPanel, visible), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedIndex_ = index;
            return;
        }
    }
}

void AchievementScene::render() const {
    const AchievementSceneLayout layout = calculateAchievementSceneLayout();
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, layout.backButton, localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(font_, localization_.get(TextId("achievement.title")), layout.title, 38.f, Color{240, 240, 250, 255});
    BasicUi::drawButton(font_, layout.filterAll, filterText(AchievementFilter::All), mouse, true, filterButtonStyle(filter_ == AchievementFilter::All));
    BasicUi::drawButton(font_, layout.filterAvailable, filterText(AchievementFilter::Available), mouse, true, filterButtonStyle(filter_ == AchievementFilter::Available));
    BasicUi::drawButton(font_, layout.filterCompleted, filterText(AchievementFilter::Completed), mouse, true, filterButtonStyle(filter_ == AchievementFilter::Completed));
    BasicUi::drawButton(font_, layout.filterLocked, filterText(AchievementFilter::Locked), mouse, true, filterButtonStyle(filter_ == AchievementFilter::Locked));

    DrawRectangleRounded(layout.listPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(layout.listPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    if (achievements_.empty()) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("achievement.empty")), layout.listPanel, 22.f, Color{190, 196, 216, 255});
    } else if (filteredAchievementIndices_.empty()) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("achievement.filter.empty")), layout.listPanel, 20.f, Color{190, 196, 216, 255});
    }

    BeginScissorMode(
        static_cast<int>(layout.listPanel.x),
        static_cast<int>(layout.listPanel.y),
        static_cast<int>(layout.listPanel.width),
        static_cast<int>(layout.listPanel.height)
    );

    const std::size_t rows = visibleRowCount(layout.listPanel);
    const std::size_t first = firstVisibleIndex(rows);
    for (std::size_t visible = 0; visible < rows; ++visible) {
        const std::size_t index = first + visible;
        if (index >= filteredAchievementIndices_.size()) {
            break;
        }

        const AchievementDefinition& achievement = *achievements_[filteredAchievementIndices_[index]];
        const bool selected = index == selectedIndex_;
        const bool unlocked = isUnlocked(achievement);
        const bool completed = isCompleted(achievement);
        const Rectangle row = achievementRowBounds(layout.listPanel, visible);
        const bool hovered = BasicUi::contains(row, mouse);

        const Color fill = selected
            ? Color{60, 66, 88, 255}
            : hovered ? Color{42, 46, 60, 255} : Color{34, 37, 49, 255};
        const Color border = selected ? Color{176, 188, 235, 255} : Color{86, 94, 122, 255};
        const Color text = unlocked ? Color{232, 236, 248, 255} : Color{145, 150, 168, 255};

        DrawRectangleRounded(row, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(row, 0.08f, 10, selected ? 3.f : 2.f, border);

        BasicUi::drawTextFitted(font_, localization_.get(achievement.nameTextId), Vector2{row.x + 18.f, row.y + 14.f}, row.width - 36.f, 22.f, 15.f, text);
        BasicUi::drawTextFitted(font_, statusText(achievement), Vector2{row.x + 18.f, row.y + 48.f}, row.width - 36.f, 16.f, 13.f, completed ? Color{185, 235, 196, 255} : Color{180, 186, 210, 255});
    }

    EndScissorMode();

    DrawRectangleRounded(layout.detailsPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(layout.detailsPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    const AchievementDefinition* selectedAchievementPtr = selectedAchievement();
    if (selectedAchievementPtr == nullptr) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("achievement.select_hint")), layout.detailsPanel, 22.f, Color{190, 196, 216, 255});
        return;
    }

    const AchievementDefinition& achievement = *selectedAchievementPtr;
    const bool unlocked = isUnlocked(achievement);
    const bool completed = isCompleted(achievement);
    const float x = layout.detailsPanel.x + 32.f;
    const float width = layout.detailsPanel.width - 64.f;
    const Rectangle contentBounds{x, layout.detailsPanel.y + 26.f, width, layout.detailsPanel.height - 52.f};
    float y = contentBounds.y;

    BeginScissorMode(
        static_cast<int>(contentBounds.x),
        static_cast<int>(contentBounds.y),
        static_cast<int>(contentBounds.width),
        static_cast<int>(contentBounds.height)
    );

    BasicUi::drawTextFitted(font_, localization_.get(achievement.nameTextId), Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
    y += 48.f;

    BasicUi::drawText(font_, statusText(achievement), Vector2{x, y}, 18.f, completed ? Color{185, 235, 196, 255} : Color{204, 210, 232, 255});
    y += 38.f;

    if (!unlocked) {
        for (const std::string& line : BasicUi::wrapText(font_, lockText(achievement), 18.f, width)) {
            BasicUi::drawText(font_, line, Vector2{x, y}, 18.f, Color{226, 214, 240, 255});
            y += 24.f;
        }
        y += 16.f;
    }

    BasicUi::drawText(font_, localization_.get(TextId("achievement.goal_label")), Vector2{x, y}, 20.f, Color{236, 229, 198, 255});
    y += 30.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(achievement.goalTextId), 18.f, width)) {
        BasicUi::drawText(font_, line, Vector2{x + 18.f, y}, 18.f, Color{220, 224, 238, 255});
        y += 24.f;
    }

    y += 18.f;
    BasicUi::drawText(font_, localization_.get(TextId("achievement.description_label")), Vector2{x, y}, 20.f, Color{236, 229, 198, 255});
    y += 30.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(achievement.descriptionTextId), 18.f, width)) {
        BasicUi::drawText(font_, line, Vector2{x + 18.f, y}, 18.f, Color{206, 212, 232, 255});
        y += 24.f;
    }

    y += 18.f;
    BasicUi::drawText(font_, localization_.get(TextId("achievement.reward_label")), Vector2{x, y}, 20.f, Color{236, 229, 198, 255});
    y += 30.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(achievement.rewardTextId), 18.f, width)) {
        BasicUi::drawText(font_, line, Vector2{x + 18.f, y}, 18.f, Color{206, 212, 232, 255});
        y += 24.f;
    }

    EndScissorMode();

}

void AchievementScene::moveSelection(const int direction) {
    if (filteredAchievementIndices_.empty()) {
        return;
    }

    const int count = static_cast<int>(filteredAchievementIndices_.size());
    int next = static_cast<int>(selectedIndex_) + direction;
    if (next < 0) {
        next = 0;
    }
    if (next >= count) {
        next = count - 1;
    }
    selectedIndex_ = static_cast<std::size_t>(next);
}

void AchievementScene::pageSelection(const int direction) {
    if (filteredAchievementIndices_.empty()) {
        return;
    }

    const AchievementSceneLayout layout = calculateAchievementSceneLayout();
    const int page = static_cast<int>(visibleRowCount(layout.listPanel));
    moveSelection(direction * page);
}

void AchievementScene::setFilter(const AchievementFilter filter) {
    if (filter_ == filter) {
        return;
    }

    filter_ = filter;
    selectedIndex_ = 0;
    rebuildFilteredAchievements();
}

void AchievementScene::rebuildFilteredAchievements() {
    filteredAchievementIndices_.clear();
    for (std::size_t index = 0; index < achievements_.size(); ++index) {
        if (achievements_[index] != nullptr && matchesFilter(*achievements_[index])) {
            filteredAchievementIndices_.push_back(index);
        }
    }

    if (filteredAchievementIndices_.empty()) {
        selectedIndex_ = 0;
    } else if (selectedIndex_ >= filteredAchievementIndices_.size()) {
        selectedIndex_ = filteredAchievementIndices_.size() - 1u;
    }
}

bool AchievementScene::matchesFilter(const AchievementDefinition& achievement) const {
    switch (filter_) {
    case AchievementFilter::All:
        return true;
    case AchievementFilter::Available:
        return isUnlocked(achievement) && !isCompleted(achievement);
    case AchievementFilter::Completed:
        return isCompleted(achievement);
    case AchievementFilter::Locked:
        return !isUnlocked(achievement);
    }

    return true;
}

const AchievementDefinition* AchievementScene::selectedAchievement() const {
    if (filteredAchievementIndices_.empty()) {
        return nullptr;
    }
    return achievements_.at(filteredAchievementIndices_.at(selectedIndex_));
}

std::size_t AchievementScene::firstVisibleIndex(const std::size_t visibleRows) const {
    if (filteredAchievementIndices_.empty() || visibleRows == 0u || filteredAchievementIndices_.size() <= visibleRows) {
        return 0u;
    }

    if (selectedIndex_ < visibleRows) {
        return 0u;
    }
    return std::min(selectedIndex_ - visibleRows + 1u, filteredAchievementIndices_.size() - visibleRows);
}

bool AchievementScene::isCompleted(const AchievementDefinition& achievement) const {
    return profile_ != nullptr && containsId(profile_->completedAchievementIds, achievement.id);
}

bool AchievementScene::isUnlocked(const AchievementDefinition& achievement) const {
    return profile_ != nullptr && AchievementRules::isUnlocked(achievement, *profile_);
}


std::string AchievementScene::filterText(const AchievementFilter filter) const {
    switch (filter) {
    case AchievementFilter::All:
        return localization_.get(TextId("achievement.filter.all"));
    case AchievementFilter::Available:
        return localization_.get(TextId("achievement.filter.available"));
    case AchievementFilter::Completed:
        return localization_.get(TextId("achievement.filter.completed"));
    case AchievementFilter::Locked:
        return localization_.get(TextId("achievement.filter.locked"));
    }

    return localization_.get(TextId("achievement.filter.all"));
}

std::string AchievementScene::statusText(const AchievementDefinition& achievement) const {
    if (isCompleted(achievement)) {
        return localization_.get(TextId("achievement.status.completed"));
    }

    if (!isUnlocked(achievement)) {
        return localization_.get(TextId("achievement.status.locked"));
    }

    return localization_.get(TextId("achievement.status.available"));
}

std::string AchievementScene::lockText(const AchievementDefinition& achievement) const {
    if (!achievement.isAvailable) {
        return localization_.get(TextId("achievement.status.in_development"));
    }

    if (!achievement.unlockHintTextId.value.empty() && localization_.hasText(achievement.unlockHintTextId)) {
        return localization_.get(achievement.unlockHintTextId);
    }

    return localization_.get(TextId("achievement.unlock_hint.default"));
}
