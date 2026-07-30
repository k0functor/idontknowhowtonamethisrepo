#include "ChallengeScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "challenges/ChallengeRules.hpp"
#include "archetypes/PlayableArchetypeId.hpp"
#include "cards/CardId.hpp"
#include "consumables/ConsumableId.hpp"
#include "relics/RelicId.hpp"
#include "run/DifficultyId.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

#include <raylib.h>

namespace {
struct ChallengeSceneLayout {
    Rectangle backButton{};
    Rectangle title{};
    Rectangle filterAll{};
    Rectangle filterAvailable{};
    Rectangle filterCompleted{};
    Rectangle filterLocked{};
    Rectangle listPanel{};
    Rectangle detailsPanel{};
    Rectangle startButton{};
    Rectangle detailsContentClip{};
};

ChallengeSceneLayout calculateChallengeSceneLayout() {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    ChallengeSceneLayout layout;
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
    const float height = std::max(330.f, screenHeight - top - 32.f);
    const float listWidth = std::clamp(screenWidth * 0.38f, 420.f, 560.f);
    layout.listPanel = Rectangle{margin, top, listWidth, height};
    layout.detailsPanel = Rectangle{layout.listPanel.x + layout.listPanel.width + gap, top, screenWidth - margin - (layout.listPanel.x + layout.listPanel.width + gap), height};
    layout.startButton = Rectangle{
        layout.detailsPanel.x + layout.detailsPanel.width - 268.f,
        layout.detailsPanel.y + layout.detailsPanel.height - 72.f,
        236.f,
        48.f
    };
    layout.detailsContentClip = Rectangle{
        layout.detailsPanel.x + 24.f,
        layout.detailsPanel.y + 24.f,
        layout.detailsPanel.width - 48.f,
        layout.detailsPanel.height - 116.f
    };
    return layout;
}

Rectangle challengeRowBounds(const Rectangle listPanel, const std::size_t visibleIndex) {
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

Rectangle overwriteConfirmPanelBounds() {
    const float width = std::min(720.f, static_cast<float>(VirtualViewport::width()) - 96.f);
    const float height = 276.f;
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle overwriteConfirmCancelButtonBounds(const Rectangle panel) {
    constexpr float buttonWidth = 220.f;
    constexpr float buttonHeight = 48.f;
    constexpr float gap = 24.f;
    return Rectangle{
        panel.x + panel.width * 0.5f - buttonWidth - gap * 0.5f,
        panel.y + panel.height - 72.f,
        buttonWidth,
        buttonHeight
    };
}

Rectangle overwriteConfirmStartButtonBounds(const Rectangle panel) {
    constexpr float buttonWidth = 220.f;
    constexpr float buttonHeight = 48.f;
    constexpr float gap = 24.f;
    return Rectangle{
        panel.x + panel.width * 0.5f + gap * 0.5f,
        panel.y + panel.height - 72.f,
        buttonWidth,
        buttonHeight
    };
}

bool containsId(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

constexpr float maxDetailsScroll = 720.f;

std::string countLabel(const std::string& label, const int count) {
    if (count <= 1) {
        return label;
    }
    return label + " x" + std::to_string(count);
}

BasicUi::ButtonStyle filterButtonStyle(const bool active) {
    BasicUi::ButtonStyle style;
    style.background = active ? Color{74, 78, 102, 255} : Color{45, 48, 58, 255};
    style.hoveredBackground = active ? Color{88, 94, 124, 255} : Color{62, 66, 82, 255};
    style.border = active ? Color{214, 184, 85, 255} : Color{130, 136, 160, 255};
    style.text = active ? Color{252, 240, 198, 255} : Color{235, 235, 242, 255};
    return style;
}

std::string joinStrings(const std::vector<std::string>& values) {
    std::string result;
    for (const std::string& value : values) {
        if (value.empty()) {
            continue;
        }
        if (!result.empty()) {
            result += ", ";
        }
        result += value;
    }
    return result;
}
}

ChallengeScene::ChallengeScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const ContentRegistry& content,
    std::vector<const ChallengeDefinition*> challenges,
    const ProfileData* profile,
    const bool hasExistingRunSave,
    std::function<void(std::string)> onStartChallenge,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      content_(content),
      challenges_(std::move(challenges)),
      profile_(profile),
      hasExistingRunSave_(hasExistingRunSave),
      onStartChallenge_(std::move(onStartChallenge)),
      onBack_(std::move(onBack)) {
    rebuildFilteredChallenges();
}

void ChallengeScene::update(float) {
    const ChallengeSceneLayout layout = calculateChallengeSceneLayout();
    const Vector2 mouse = GetMousePosition();

    if (!pendingStartChallengeId_.empty()) {
        const Rectangle confirmPanel = overwriteConfirmPanelBounds();
        const Rectangle cancelButton = overwriteConfirmCancelButtonBounds(confirmPanel);
        const Rectangle startButton = overwriteConfirmStartButtonBounds(confirmPanel);

        if (IsKeyPressed(KEY_ESCAPE)) {
            cancelPendingStart();
            return;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            confirmPendingStart();
            return;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (BasicUi::contains(cancelButton, mouse)) {
                cancelPendingStart();
                return;
            }

            if (BasicUi::contains(startButton, mouse)) {
                confirmPendingStart();
                return;
            }
        }

        return;
    }

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

    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) && canStartSelected()) {
        requestStartSelected();
        return;
    }

    if (BasicUi::contains(layout.backButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
        return;
    }

    if (BasicUi::contains(layout.filterAll, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(ChallengeFilter::All);
        return;
    }

    if (BasicUi::contains(layout.filterAvailable, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(ChallengeFilter::Available);
        return;
    }

    if (BasicUi::contains(layout.filterCompleted, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(ChallengeFilter::Completed);
        return;
    }

    if (BasicUi::contains(layout.filterLocked, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        setFilter(ChallengeFilter::Locked);
        return;
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.f) {
        if (BasicUi::contains(layout.detailsPanel, mouse)) {
            detailsScroll_ = std::clamp(detailsScroll_ - wheel * 54.f, 0.f, maxDetailsScroll);
        } else if (BasicUi::contains(layout.listPanel, mouse)) {
            moveSelection(wheel > 0.f ? -1 : 1);
        }
    }

    const std::size_t rows = visibleRowCount(layout.listPanel);
    const std::size_t first = firstVisibleIndex(rows);
    for (std::size_t visible = 0; visible < rows; ++visible) {
        const std::size_t index = first + visible;
        if (index >= filteredChallengeIndices_.size()) {
            break;
        }

        if (BasicUi::contains(challengeRowBounds(layout.listPanel, visible), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectIndex(index);
            return;
        }
    }

    if (BasicUi::contains(layout.startButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canStartSelected()) {
        requestStartSelected();
    }
}

void ChallengeScene::render() const {
    const ChallengeSceneLayout layout = calculateChallengeSceneLayout();
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, layout.backButton, localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(font_, localization_.get(TextId("challenge.title")), layout.title, 38.f, Color{240, 240, 250, 255});
    BasicUi::drawButton(font_, layout.filterAll, filterText(ChallengeFilter::All), mouse, true, filterButtonStyle(filter_ == ChallengeFilter::All));
    BasicUi::drawButton(font_, layout.filterAvailable, filterText(ChallengeFilter::Available), mouse, true, filterButtonStyle(filter_ == ChallengeFilter::Available));
    BasicUi::drawButton(font_, layout.filterCompleted, filterText(ChallengeFilter::Completed), mouse, true, filterButtonStyle(filter_ == ChallengeFilter::Completed));
    BasicUi::drawButton(font_, layout.filterLocked, filterText(ChallengeFilter::Locked), mouse, true, filterButtonStyle(filter_ == ChallengeFilter::Locked));

    DrawRectangleRounded(layout.listPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(layout.listPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    if (challenges_.empty()) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("challenge.empty")), layout.listPanel, 22.f, Color{190, 196, 216, 255});
    } else if (filteredChallengeIndices_.empty()) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("challenge.filter.empty")), layout.listPanel, 20.f, Color{190, 196, 216, 255});
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
        if (index >= filteredChallengeIndices_.size()) {
            break;
        }

        const ChallengeDefinition& challenge = *challenges_[filteredChallengeIndices_[index]];
        const bool selected = index == selectedIndex_;
        const bool unlocked = isUnlocked(challenge);
        const bool completed = isCompleted(challenge);
        const Rectangle row = challengeRowBounds(layout.listPanel, visible);
        const bool hovered = BasicUi::contains(row, mouse);

        const Color fill = selected
            ? Color{60, 66, 88, 255}
            : hovered ? Color{42, 46, 60, 255} : Color{34, 37, 49, 255};
        const Color border = selected ? Color{176, 188, 235, 255} : Color{86, 94, 122, 255};
        const Color text = unlocked ? Color{232, 236, 248, 255} : Color{145, 150, 168, 255};

        DrawRectangleRounded(row, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(row, 0.08f, 10, selected ? 3.f : 2.f, border);

        BasicUi::drawTextFitted(font_, localization_.get(challenge.nameTextId), Vector2{row.x + 18.f, row.y + 14.f}, row.width - 36.f, 22.f, 15.f, text);
        BasicUi::drawTextFitted(font_, statusText(challenge), Vector2{row.x + 18.f, row.y + 48.f}, row.width - 36.f, 16.f, 13.f, completed ? Color{185, 235, 196, 255} : Color{180, 186, 210, 255});
    }

    EndScissorMode();

    DrawRectangleRounded(layout.detailsPanel, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(layout.detailsPanel, 0.035f, 12, 2.f, Color{95, 104, 135, 255});

    const ChallengeDefinition* selectedChallengePtr = selectedChallenge();
    if (selectedChallengePtr == nullptr) {
        BasicUi::drawCenteredText(font_, localization_.get(TextId("challenge.select_hint")), layout.detailsPanel, 22.f, Color{190, 196, 216, 255});
        return;
    }

    const ChallengeDefinition& challenge = *selectedChallengePtr;
    const bool unlocked = isUnlocked(challenge);
    const bool completed = isCompleted(challenge);
    const float x = layout.detailsContentClip.x + 8.f;
    const float width = layout.detailsContentClip.width - 16.f;
    float y = layout.detailsContentClip.y + 6.f - detailsScroll_;

    BeginScissorMode(
        static_cast<int>(layout.detailsContentClip.x),
        static_cast<int>(layout.detailsContentClip.y),
        static_cast<int>(layout.detailsContentClip.width),
        static_cast<int>(layout.detailsContentClip.height)
    );

    BasicUi::drawTextFitted(font_, localization_.get(challenge.nameTextId), Vector2{x, y}, width, 30.f, 20.f, Color{244, 244, 250, 255});
    y += 48.f;

    BasicUi::drawText(font_, statusText(challenge), Vector2{x, y}, 18.f, completed ? Color{185, 235, 196, 255} : Color{204, 210, 232, 255});
    y += 38.f;

    if (!unlocked) {
        for (const std::string& line : BasicUi::wrapText(font_, lockText(challenge), 18.f, width)) {
            BasicUi::drawText(font_, line, Vector2{x, y}, 18.f, Color{226, 214, 240, 255});
            y += 24.f;
        }
        y += 16.f;
    }

    BasicUi::drawText(font_, localization_.get(TextId("challenge.goal_label")), Vector2{x, y}, 20.f, Color{236, 229, 198, 255});
    y += 30.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(challenge.goalTextId), 18.f, width)) {
        BasicUi::drawText(font_, line, Vector2{x + 18.f, y}, 18.f, Color{220, 224, 238, 255});
        y += 24.f;
    }

    y += 18.f;
    BasicUi::drawText(font_, localization_.get(TextId("challenge.loadout_label")), Vector2{x, y}, 20.f, Color{236, 229, 198, 255});
    y += 30.f;
    for (const std::string& loadoutLine : loadoutLines(challenge)) {
        for (const std::string& line : BasicUi::wrapText(font_, loadoutLine, 17.f, width - 18.f)) {
            BasicUi::drawText(font_, line, Vector2{x + 18.f, y}, 17.f, Color{206, 212, 232, 255});
            y += 23.f;
        }
    }

    y += 18.f;
    BasicUi::drawText(font_, localization_.get(TextId("challenge.description_label")), Vector2{x, y}, 20.f, Color{236, 229, 198, 255});
    y += 30.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(challenge.descriptionTextId), 18.f, width)) {
        BasicUi::drawText(font_, line, Vector2{x + 18.f, y}, 18.f, Color{206, 212, 232, 255});
        y += 24.f;
    }

    y += 18.f;
    BasicUi::drawText(font_, localization_.get(TextId("challenge.reward_label")), Vector2{x, y}, 20.f, Color{236, 229, 198, 255});
    y += 30.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(challenge.rewardTextId), 18.f, width)) {
        BasicUi::drawText(font_, line, Vector2{x + 18.f, y}, 18.f, Color{206, 212, 232, 255});
        y += 24.f;
    }

    EndScissorMode();

    const bool canStart = unlocked;
    (void)BasicUi::drawButton(
        font_,
        layout.startButton,
        localization_.get(TextId("challenge.start")),
        mouse,
        canStart
    );

    if (!pendingStartChallengeId_.empty()) {
        const auto selected = std::find_if(
            challenges_.begin(),
            challenges_.end(),
            [this](const ChallengeDefinition* candidate) {
                return candidate != nullptr && candidate->id == pendingStartChallengeId_;
            }
        );
        const std::string challengeName = selected != challenges_.end()
            ? localization_.get((*selected)->nameTextId)
            : pendingStartChallengeId_;

        DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 170});

        const Rectangle confirmPanel = overwriteConfirmPanelBounds();
        DrawRectangleRounded(confirmPanel, 0.045f, 14, Color{26, 29, 40, 255});
        DrawRectangleRoundedLinesEx(confirmPanel, 0.045f, 14, 2.f, Color{170, 132, 78, 255});

        BasicUi::drawCenteredTextFitted(
            font_,
            localization_.get(TextId("challenge.confirm_overwrite.title")),
            Rectangle{confirmPanel.x + 28.f, confirmPanel.y + 24.f, confirmPanel.width - 56.f, 34.f},
            25.f,
            17.f,
            Color{248, 230, 185, 255}
        );

        const std::string body = localization_.format(
            TextId("challenge.confirm_overwrite.description"),
            {{"name", challengeName}}
        );
        float textY = confirmPanel.y + 78.f;
        for (const std::string& line : BasicUi::wrapText(font_, body, 18.f, confirmPanel.width - 64.f)) {
            BasicUi::drawText(font_, line, Vector2{confirmPanel.x + 32.f, textY}, 18.f, Color{214, 218, 236, 255});
            textY += 25.f;
        }

        const Rectangle cancelButton = overwriteConfirmCancelButtonBounds(confirmPanel);
        const Rectangle startButton = overwriteConfirmStartButtonBounds(confirmPanel);
        (void)BasicUi::drawButton(font_, cancelButton, localization_.get(TextId("ui.cancel")), mouse);
        (void)BasicUi::drawButton(font_, startButton, localization_.get(TextId("challenge.confirm_overwrite.start")), mouse);
    }
}

void ChallengeScene::moveSelection(const int direction) {
    if (filteredChallengeIndices_.empty()) {
        return;
    }

    const int count = static_cast<int>(filteredChallengeIndices_.size());
    int next = static_cast<int>(selectedIndex_) + direction;
    if (next < 0) {
        next = count - 1;
    }
    if (next >= count) {
        next = 0;
    }
    selectIndex(static_cast<std::size_t>(next));
}

void ChallengeScene::selectIndex(const std::size_t index) {
    if (index >= filteredChallengeIndices_.size()) {
        return;
    }
    if (selectedIndex_ != index) {
        detailsScroll_ = 0.f;
    }
    selectedIndex_ = index;
}

void ChallengeScene::setFilter(const ChallengeFilter filter) {
    if (filter_ == filter) {
        return;
    }

    filter_ = filter;
    selectedIndex_ = 0u;
    detailsScroll_ = 0.f;
    rebuildFilteredChallenges();
}

void ChallengeScene::rebuildFilteredChallenges() {
    filteredChallengeIndices_.clear();
    for (std::size_t index = 0; index < challenges_.size(); ++index) {
        const ChallengeDefinition* challenge = challenges_[index];
        if (challenge != nullptr && matchesFilter(*challenge)) {
            filteredChallengeIndices_.push_back(index);
        }
    }

    if (selectedIndex_ >= filteredChallengeIndices_.size()) {
        selectedIndex_ = filteredChallengeIndices_.empty() ? 0u : filteredChallengeIndices_.size() - 1u;
    }
}

bool ChallengeScene::matchesFilter(const ChallengeDefinition& challenge) const {
    switch (filter_) {
    case ChallengeFilter::All:
        return true;
    case ChallengeFilter::Available:
        return isUnlocked(challenge) && !isCompleted(challenge);
    case ChallengeFilter::Completed:
        return isCompleted(challenge);
    case ChallengeFilter::Locked:
        return !isUnlocked(challenge);
    }

    return true;
}

std::size_t ChallengeScene::firstVisibleIndex(const std::size_t visibleRows) const {
    if (filteredChallengeIndices_.empty() || visibleRows == 0u || filteredChallengeIndices_.size() <= visibleRows) {
        return 0u;
    }

    const std::size_t half = visibleRows / 2u;
    if (selectedIndex_ <= half) {
        return 0u;
    }

    const std::size_t maxFirst = filteredChallengeIndices_.size() - visibleRows;
    return std::min(maxFirst, selectedIndex_ - half);
}

void ChallengeScene::requestStartSelected() {
    const ChallengeDefinition* challenge = selectedChallenge();
    if (challenge == nullptr || !canStartSelected()) {
        return;
    }

    if (hasExistingRunSave_) {
        pendingStartChallengeId_ = challenge->id;
        return;
    }

    onStartChallenge_(challenge->id);
}

void ChallengeScene::confirmPendingStart() {
    if (pendingStartChallengeId_.empty()) {
        return;
    }

    std::string challengeId = std::move(pendingStartChallengeId_);
    pendingStartChallengeId_.clear();
    onStartChallenge_(std::move(challengeId));
}

void ChallengeScene::cancelPendingStart() {
    pendingStartChallengeId_.clear();
}

const ChallengeDefinition* ChallengeScene::selectedChallenge() const {
    if (filteredChallengeIndices_.empty() || selectedIndex_ >= filteredChallengeIndices_.size()) {
        return nullptr;
    }

    return challenges_.at(filteredChallengeIndices_[selectedIndex_]);
}

bool ChallengeScene::isSelectedCompleted() const {
    const ChallengeDefinition* challenge = selectedChallenge();
    return challenge != nullptr && isCompleted(*challenge);
}

bool ChallengeScene::canStartSelected() const {
    const ChallengeDefinition* challenge = selectedChallenge();
    return challenge != nullptr && isUnlocked(*challenge);
}

bool ChallengeScene::isCompleted(const ChallengeDefinition& challenge) const {
    return profile_ != nullptr && containsId(profile_->completedChallengeIds, challenge.id);
}

bool ChallengeScene::isUnlocked(const ChallengeDefinition& challenge) const {
    return profile_ != nullptr && ChallengeRules::isUnlocked(challenge, *profile_);
}


std::string ChallengeScene::filterText(const ChallengeFilter filter) const {
    switch (filter) {
    case ChallengeFilter::All:
        return localization_.get(TextId("challenge.filter.all"));
    case ChallengeFilter::Available:
        return localization_.get(TextId("challenge.filter.available"));
    case ChallengeFilter::Completed:
        return localization_.get(TextId("challenge.filter.completed"));
    case ChallengeFilter::Locked:
        return localization_.get(TextId("challenge.filter.locked"));
    }

    return localization_.get(TextId("challenge.filter.all"));
}


std::string ChallengeScene::statusText(const ChallengeDefinition& challenge) const {
    if (isCompleted(challenge)) {
        return localization_.get(TextId("challenge.status.completed"));
    }

    if (!isUnlocked(challenge)) {
        return localization_.get(TextId("challenge.status.locked"));
    }

    return localization_.get(TextId("challenge.status.available"));
}

std::string ChallengeScene::lockText(const ChallengeDefinition& challenge) const {
    if (!challenge.isAvailable) {
        return localization_.get(TextId("challenge.status.in_development"));
    }

    if (!challenge.unlockHintTextId.value.empty() && localization_.hasText(challenge.unlockHintTextId)) {
        return localization_.get(challenge.unlockHintTextId);
    }

    return localization_.get(TextId("challenge.unlock_hint.default"));
}


std::vector<std::string> ChallengeScene::loadoutLines(const ChallengeDefinition& challenge) const {
    std::vector<std::string> lines;

    const std::string archetypeName = contentNameOrId(challenge.startingArchetypeId, "archetype");
    const std::string difficultyName = contentNameOrId(challenge.startingDifficultyId, "difficulty");
    const std::string floorId = challenge.startingFloorId.empty()
        ? content_.floors().startingFloor().id
        : challenge.startingFloorId;
    const std::string floorName = contentNameOrId(floorId, "floor");

    lines.push_back(localization_.format(TextId("challenge.loadout.archetype"), {{"value", archetypeName}}));
    lines.push_back(localization_.format(TextId("challenge.loadout.difficulty"), {{"value", difficultyName}}));
    lines.push_back(localization_.format(TextId("challenge.loadout.floor"), {{"value", floorName}}));

    if (challenge.startingGoldOverride >= 0) {
        lines.push_back(localization_.format(
            TextId("challenge.loadout.gold"),
            {{"value", std::to_string(challenge.startingGoldOverride)}}
        ));
    }

    if (!challenge.fixedStartingDeckCardIds.empty()) {
        lines.push_back(localization_.format(
            TextId("challenge.loadout.deck"),
            {
                {"count", std::to_string(challenge.fixedStartingDeckCardIds.size())},
                {"value", summarizedContentList(challenge.fixedStartingDeckCardIds, "card")}
            }
        ));
    }

    if (!challenge.fixedStartingRelicIds.empty()) {
        lines.push_back(localization_.format(
            TextId("challenge.loadout.relics"),
            {{"value", summarizedContentList(challenge.fixedStartingRelicIds, "relic")}}
        ));
    }

    if (!challenge.fixedStartingConsumableIds.empty()) {
        lines.push_back(localization_.format(
            TextId("challenge.loadout.consumables"),
            {{"value", summarizedContentList(challenge.fixedStartingConsumableIds, "consumable")}}
        ));
    }

    return lines;
}

std::string ChallengeScene::contentNameOrId(const std::string& contentId, const std::string& type) const {
    if (contentId.empty()) {
        return localization_.get(TextId("challenge.loadout.default"));
    }

    if (type == "archetype" && content_.archetypes().contains(PlayableArchetypeId(contentId))) {
        return localization_.get(content_.archetypes().get(PlayableArchetypeId(contentId)).nameTextId);
    }
    if (type == "difficulty" && content_.difficulties().contains(DifficultyId(contentId))) {
        return localization_.get(content_.difficulties().get(DifficultyId(contentId)).nameTextId);
    }
    if (type == "floor" && content_.floors().contains(contentId)) {
        return localization_.get(TextId(content_.floors().get(contentId).nameTextId));
    }
    if (type == "card" && content_.cards().contains(CardId(contentId))) {
        return localization_.get(content_.cards().get(CardId(contentId)).nameTextId);
    }
    if (type == "relic" && content_.relics().contains(RelicId(contentId))) {
        return localization_.get(content_.relics().get(RelicId(contentId)).nameTextId);
    }
    if (type == "consumable" && content_.consumables().contains(ConsumableId(contentId))) {
        return localization_.get(content_.consumables().get(ConsumableId(contentId)).nameTextId);
    }

    return contentId;
}

std::string ChallengeScene::summarizedContentList(
    const std::vector<std::string>& contentIds,
    const std::string& type
) const {
    if (contentIds.empty()) {
        return localization_.get(TextId("challenge.loadout.none"));
    }

    std::vector<std::pair<std::string, int>> counts;
    for (const std::string& contentId : contentIds) {
        const auto existing = std::find_if(
            counts.begin(),
            counts.end(),
            [&contentId](const auto& entry) { return entry.first == contentId; }
        );
        if (existing == counts.end()) {
            counts.push_back({contentId, 1});
        } else {
            ++existing->second;
        }
    }

    std::vector<std::string> names;
    names.reserve(counts.size());
    for (const auto& [contentId, count] : counts) {
        names.push_back(countLabel(contentNameOrId(contentId, type), count));
    }

    return joinStrings(names);
}
