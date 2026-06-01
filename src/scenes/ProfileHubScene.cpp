#include "ProfileHubScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardId.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <raylib.h>

namespace {
Color archetypeAccentColor(const PlayableArchetypeDefinition& archetype, const std::uint8_t alpha = 255) {
    return Color{
        archetype.palette.accentR,
        archetype.palette.accentG,
        archetype.palette.accentB,
        alpha
    };
}

struct ProfileHubLayout {
    Rectangle backButton{};
    Rectangle title{};
    Rectangle leftArrow{};
    Rectangle rightArrow{};
    Rectangle characterPanel{};
    Rectangle continueButton{};
    Rectangle startButton{};
    Rectangle challengesButton{};
    Rectangle achievementsButton{};
    Rectangle compendiumButton{};
    Rectangle notification{};
};

struct DetailsModalLayout {
    Rectangle modal{};
    Rectangle title{};
    Rectangle content{};
    Rectangle closeButton{};
    Rectangle scrollTrack{};
    Rectangle scrollHint{};
};

ProfileHubLayout calculateProfileHubLayout(const bool hasSavedRun) {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    constexpr float sideButtonWidth = 230.f;
    constexpr float sideButtonHeight = 54.f;
    constexpr float sideButtonGap = 16.f;
    constexpr float arrowWidth = 56.f;
    constexpr float arrowHeight = 64.f;
    constexpr float arrowGap = 12.f;
    constexpr float panelToButtonsGap = 18.f;

    ProfileHubLayout layout;
    layout.backButton = Rectangle{32.f, 32.f, 140.f, 48.f};
    layout.title = Rectangle{0.f, 70.f, screenWidth, 60.f};

    const float panelX = std::max(88.f, screenWidth * 0.11f);
    const float panelY = std::max(135.f, screenHeight * 0.21f);

    const float maxPanelWidthByScreen = std::max(360.f, screenWidth - panelX - sideButtonWidth - arrowWidth - arrowGap - panelToButtonsGap - 40.f);
    const float preferredPanelWidth = std::clamp(screenWidth * 0.50f, 540.f, 760.f);
    const float panelWidth = std::min(preferredPanelWidth, maxPanelWidthByScreen);
    const float panelHeight = std::clamp(screenHeight * 0.64f, 420.f, 540.f);

    layout.characterPanel = Rectangle{panelX, panelY, panelWidth, panelHeight};

    const float arrowY = layout.characterPanel.y + layout.characterPanel.height * 0.5f - arrowHeight * 0.5f;
    layout.leftArrow = Rectangle{
        layout.characterPanel.x - arrowGap - arrowWidth,
        arrowY,
        arrowWidth,
        arrowHeight
    };

    layout.rightArrow = Rectangle{
        layout.characterPanel.x + layout.characterPanel.width + arrowGap,
        arrowY,
        arrowWidth,
        arrowHeight
    };

    const float sideX = layout.rightArrow.x + layout.rightArrow.width + panelToButtonsGap;
    const float sideY = layout.characterPanel.y + 20.f;
    float nextButtonY = sideY;

    if (hasSavedRun) {
        layout.continueButton = Rectangle{sideX, nextButtonY, sideButtonWidth, sideButtonHeight};
        nextButtonY += sideButtonHeight + sideButtonGap;
    }

    layout.startButton = Rectangle{sideX, nextButtonY, sideButtonWidth, sideButtonHeight};
    layout.challengesButton = Rectangle{sideX, nextButtonY + (sideButtonHeight + sideButtonGap), sideButtonWidth, sideButtonHeight};
    layout.achievementsButton = Rectangle{sideX, nextButtonY + 2.f * (sideButtonHeight + sideButtonGap), sideButtonWidth, sideButtonHeight};
    layout.compendiumButton = Rectangle{sideX, nextButtonY + 3.f * (sideButtonHeight + sideButtonGap), sideButtonWidth, sideButtonHeight};

    layout.notification = Rectangle{0.f, screenHeight - 50.f, screenWidth, 32.f};

    return layout;
}

DetailsModalLayout calculateDetailsModalLayout() {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    const float modalWidth = std::clamp(screenWidth - 96.f, 560.f, 900.f);
    const float modalHeight = std::clamp(screenHeight - 120.f, 420.f, 680.f);
    const Rectangle modal{
        screenWidth * 0.5f - modalWidth * 0.5f,
        screenHeight * 0.5f - modalHeight * 0.5f,
        modalWidth,
        modalHeight
    };

    DetailsModalLayout layout;
    layout.modal = modal;
    layout.title = Rectangle{modal.x + 36.f, modal.y + 22.f, modal.width - 72.f, 44.f};
    layout.closeButton = Rectangle{modal.x + modal.width * 0.5f - 90.f, modal.y + modal.height - 56.f, 180.f, 42.f};
    layout.scrollHint = Rectangle{modal.x + 36.f, layout.closeButton.y - 26.f, modal.width - 72.f, 20.f};
    layout.content = Rectangle{
        modal.x + 36.f,
        modal.y + 82.f,
        modal.width - 72.f,
        layout.scrollHint.y - modal.y - 96.f
    };
    layout.scrollTrack = Rectangle{modal.x + modal.width - 24.f, layout.content.y, 8.f, layout.content.height};

    return layout;
}

Rectangle scrollThumbBounds(const Rectangle track, const float visibleHeight, const float contentHeight, const float scrollY) {
    if (contentHeight <= visibleHeight || visibleHeight <= 0.f || contentHeight <= 0.f) {
        return Rectangle{track.x, track.y, track.width, track.height};
    }

    const float thumbHeight = std::clamp((visibleHeight / contentHeight) * track.height, 34.f, track.height);
    const float maxScroll = std::max(1.f, contentHeight - visibleHeight);
    const float travel = std::max(0.f, track.height - thumbHeight);
    const float t = std::clamp(scrollY / maxScroll, 0.f, 1.f);
    return Rectangle{track.x, track.y + travel * t, track.width, thumbHeight};
}
}

ProfileHubScene::ProfileHubScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const PlayerActorDatabase& actors,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    std::vector<const PlayableArchetypeDefinition*> archetypes,
    const bool hasSavedRun,
    std::function<void(PlayableArchetypeId)> onStartRun,
    std::function<void()> onContinueRun,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      actors_(actors),
      cards_(cards),
      relics_(relics),
      archetypes_(std::move(archetypes)),
      hasSavedRun_(hasSavedRun),
      onStartRun_(std::move(onStartRun)),
      onContinueRun_(std::move(onContinueRun)),
      onBack_(std::move(onBack)) {
    if (archetypes_.empty()) {
        notification_ = localization_.get(TextId("profile_hub.no_archetypes"));
    }
}

void ProfileHubScene::update(float) {
    if (detailsOpen_) {
        updateDetailsModal();
        return;
    }

    const Vector2 mouse = GetMousePosition();

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        moveSelection(-1);
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        moveSelection(1);
    }

    if (IsKeyPressed(KEY_SPACE) && !archetypes_.empty()) {
        detailsOpen_ = true;
        detailsScrollY_ = 0.f;
        return;
    }

    const ProfileHubLayout layout = calculateProfileHubLayout(hasSavedRun_);

    if (BasicUi::contains(layout.leftArrow, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        moveSelection(-1);
    }

    if (BasicUi::contains(layout.rightArrow, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        moveSelection(1);
    }

    if (hasSavedRun_ && BasicUi::contains(layout.continueButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onContinueRun_();
        return;
    }

    if (BasicUi::contains(layout.startButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !archetypes_.empty()) {
        if (selectedArchetype().isAvailable) {
            onStartRun_(selectedArchetype().id);
        } else {
            notification_ = localization_.get(TextId("profile_hub.archetype_in_development"));
        }
    }

    if (BasicUi::contains(layout.backButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
    }

    if (BasicUi::contains(layout.challengesButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        notification_ = localization_.get(TextId("ui.challenges_later"));
    }

    if (BasicUi::contains(layout.achievementsButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        notification_ = localization_.get(TextId("ui.achievements_later"));
    }

    if (BasicUi::contains(layout.compendiumButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        notification_ = localization_.get(TextId("ui.compendium_later"));
    }
}

void ProfileHubScene::render() const {
    const Vector2 mouse = GetMousePosition();

    const ProfileHubLayout layout = calculateProfileHubLayout(hasSavedRun_);

    BasicUi::drawButton(font_, layout.backButton, localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(font_, localization_.get(TextId("profile_hub.title")), layout.title, 38.f, Color{240, 240, 250, 255});

    BasicUi::drawButton(font_, layout.leftArrow, "<", mouse, !archetypes_.empty());
    BasicUi::drawButton(font_, layout.rightArrow, ">", mouse, !archetypes_.empty());

    const Rectangle characterPanel = layout.characterPanel;
    DrawRectangleRounded(characterPanel, 0.06f, 12, Color{31, 34, 44, 255});
    DrawRectangleRoundedLinesEx(characterPanel, 0.06f, 12, 2.f, Color{100, 110, 145, 255});

    if (!archetypes_.empty()) {
        const PlayableArchetypeDefinition& archetype = selectedArchetype();
        const bool available = archetype.isAvailable;
        const Color titleColor = available ? Color{245, 245, 250, 255} : Color{165, 168, 184, 255};
        const Color subtitleColor = available ? Color{190, 198, 220, 255} : Color{135, 140, 158, 255};
        const Color accent = available ? archetypeAccentColor(archetype) : Color{105, 108, 124, 255};
        const Color accentFill = available ? archetypeAccentColor(archetype, 70) : Color{65, 67, 78, 190};

        BasicUi::drawCenteredText(font_, localization_.get(archetype.nameTextId), Rectangle{characterPanel.x, characterPanel.y + 40.f, characterPanel.width, 50.f}, 34.f, titleColor);
        BasicUi::drawCenteredText(font_, localization_.get(archetype.shortDescriptionTextId), Rectangle{characterPanel.x + 50.f, characterPanel.y + 100.f, characterPanel.width - 100.f, 60.f}, 20.f, subtitleColor);

        DrawCircle(static_cast<int>(characterPanel.x + characterPanel.width * 0.5f), static_cast<int>(characterPanel.y + 265.f), 96.f, accentFill);
        DrawCircleLines(static_cast<int>(characterPanel.x + characterPanel.width * 0.5f), static_cast<int>(characterPanel.y + 265.f), 96.f, accent);
        DrawCircle(static_cast<int>(characterPanel.x + characterPanel.width * 0.5f), static_cast<int>(characterPanel.y + 265.f), 82.f, available ? Color{75, 79, 96, 255} : Color{48, 50, 60, 255});
        BasicUi::drawCenteredText(font_, localization_.get(TextId("ui.portrait_placeholder")), Rectangle{characterPanel.x, characterPanel.y + 240.f, characterPanel.width, 40.f}, 18.f, available ? Color{160, 166, 190, 255} : Color{105, 110, 130, 255});
        BasicUi::drawCenteredText(font_, localization_.get(archetype.palette.nameTextId), Rectangle{characterPanel.x + 45.f, characterPanel.y + 355.f, characterPanel.width - 90.f, 28.f}, 16.f, accent);

        if (!available) {
            const Rectangle badge{characterPanel.x + characterPanel.width * 0.5f - 120.f, characterPanel.y + 170.f, 240.f, 38.f};
            DrawRectangleRounded(badge, 0.22f, 8, Color{55, 49, 58, 240});
            DrawRectangleRoundedLinesEx(badge, 0.22f, 8, 2.f, Color{150, 124, 170, 255});
            BasicUi::drawCenteredText(font_, localization_.get(TextId("profile_hub.in_development_badge")), badge, 18.f, Color{226, 214, 240, 255});
        }

        BasicUi::drawCenteredText(font_, localization_.get(TextId("profile_hub.details_hint")), Rectangle{characterPanel.x, characterPanel.y + 405.f, characterPanel.width, 40.f}, 20.f, available ? Color{220, 220, 235, 255} : Color{150, 154, 174, 255});
    }

    if (hasSavedRun_) {
        BasicUi::drawButton(font_, layout.continueButton, localization_.get(TextId("save_slot.continue_run")), mouse);
    }

    const bool selectedAvailable = !archetypes_.empty() && selectedArchetype().isAvailable;
    BasicUi::drawButton(font_, layout.startButton, localization_.get(TextId("ui.to_the_road")), mouse, selectedAvailable);
    BasicUi::drawButton(font_, layout.challengesButton, localization_.get(TextId("ui.challenges")), mouse);
    BasicUi::drawButton(font_, layout.achievementsButton, localization_.get(TextId("ui.achievements")), mouse);
    BasicUi::drawButton(font_, layout.compendiumButton, localization_.get(TextId("ui.compendium")), mouse);

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("profile_hub.navigation_hint")),
        Rectangle{characterPanel.x, characterPanel.y + characterPanel.height + 12.f, characterPanel.width, 28.f},
        18.f,
        Color{165, 172, 195, 255}
    );

    if (!notification_.empty()) {
        BasicUi::drawCenteredText(font_, notification_, layout.notification, 18.f, Color{185, 190, 210, 255});
    }

    if (detailsOpen_) {
        renderDetailsModal();
    }
}

void ProfileHubScene::moveSelection(const int direction) {
    if (archetypes_.empty()) {
        return;
    }

    const int count = static_cast<int>(archetypes_.size());
    int next = static_cast<int>(selectedIndex_) + direction;

    if (next < 0) {
        next = count - 1;
    }

    if (next >= count) {
        next = 0;
    }

    selectedIndex_ = static_cast<std::size_t>(next);
}

const PlayableArchetypeDefinition& ProfileHubScene::selectedArchetype() const {
    return *archetypes_.at(selectedIndex_);
}

void ProfileHubScene::updateDetailsModal() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
        detailsOpen_ = false;
        return;
    }

    const DetailsModalLayout layout = calculateDetailsModalLayout();
    const Vector2 mouse = GetMousePosition();

    if (BasicUi::contains(layout.closeButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        detailsOpen_ = false;
        return;
    }

    const float contentHeight = detailsContentHeight(layout.content);
    const float maxScroll = std::max(0.f, contentHeight - layout.content.height);

    if (BasicUi::contains(layout.modal, mouse)) {
        const float mouseWheel = GetMouseWheelMove();
        if (mouseWheel != 0.f) {
            detailsScrollY_ -= mouseWheel * 56.f;
        }
    }

    if (IsKeyPressed(KEY_DOWN)) {
        detailsScrollY_ += 48.f;
    }

    if (IsKeyPressed(KEY_UP)) {
        detailsScrollY_ -= 48.f;
    }

    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        detailsScrollY_ += layout.content.height * 0.82f;
    }

    if (IsKeyPressed(KEY_PAGE_UP)) {
        detailsScrollY_ -= layout.content.height * 0.82f;
    }

    if (IsKeyPressed(KEY_HOME)) {
        detailsScrollY_ = 0.f;
    }

    if (IsKeyPressed(KEY_END)) {
        detailsScrollY_ = maxScroll;
    }

    detailsScrollY_ = std::clamp(detailsScrollY_, 0.f, maxScroll);
}

void ProfileHubScene::renderDetailsModal() const {
    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 145});

    const PlayableArchetypeDefinition& archetype = selectedArchetype();
    const DetailsModalLayout layout = calculateDetailsModalLayout();
    DrawRectangleRounded(layout.modal, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(layout.modal, 0.035f, 12, 2.f, Color{140, 150, 185, 255});

    BasicUi::drawCenteredText(font_, localization_.get(archetype.nameTextId), layout.title, 32.f, Color{245, 245, 250, 255});

    const float contentHeight = detailsContentHeight(layout.content);
    const float maxScroll = std::max(0.f, contentHeight - layout.content.height);
    const float scrollY = std::clamp(detailsScrollY_, 0.f, maxScroll);

    BeginScissorMode(
        static_cast<int>(std::floor(layout.content.x)),
        static_cast<int>(std::floor(layout.content.y)),
        static_cast<int>(std::ceil(layout.content.width)),
        static_cast<int>(std::ceil(layout.content.height))
    );
    renderDetailsContent(layout.content, scrollY, true);
    EndScissorMode();

    if (contentHeight > layout.content.height + 1.f) {
        DrawRectangleRounded(layout.scrollTrack, 0.6f, 6, Color{42, 46, 61, 255});
        DrawRectangleRounded(
            scrollThumbBounds(layout.scrollTrack, layout.content.height, contentHeight, scrollY),
            0.6f,
            6,
            Color{150, 162, 205, 255}
        );
        BasicUi::drawCenteredText(font_, localization_.get(TextId("profile_hub.scroll_hint")), layout.scrollHint, 15.f, Color{160, 168, 195, 255});
    }

    BasicUi::drawButton(font_, layout.closeButton, localization_.get(TextId("ui.close")), GetMousePosition());
}

float ProfileHubScene::detailsContentHeight(const Rectangle contentBounds) const {
    return renderDetailsContent(contentBounds, 0.f, false);
}

float ProfileHubScene::renderDetailsContent(const Rectangle contentBounds, const float scrollY, const bool draw) const {
    const PlayableArchetypeDefinition& archetype = selectedArchetype();

    const float baseX = contentBounds.x + 6.f;
    const float indentedX = contentBounds.x + 28.f;
    const float wrappedWidth = contentBounds.width - 52.f;
    float y = contentBounds.y - scrollY;

    const auto drawTextIfNeeded = [&](const std::string& text, const Vector2 position, const float fontSize, const Color color) {
        if (draw) {
            BasicUi::drawText(font_, text, position, fontSize, color);
        }
    };

    const auto addWrappedText = [&](const std::string& text, const float fontSize, const float lineHeight, const float x, const float maxWidth, const Color color) {
        for (const std::string& line : BasicUi::wrapText(font_, text, fontSize, maxWidth)) {
            drawTextIfNeeded(line, Vector2{x, y}, fontSize, color);
            y += lineHeight;
        }
    };

    const auto addSectionTitle = [&](const std::string& title, const Color color) {
        y += 18.f;
        drawTextIfNeeded(title, Vector2{baseX, y}, 23.f, color);
        y += 32.f;
    };

    if (!archetype.isAvailable) {
        addWrappedText(localization_.get(TextId("profile_hub.archetype_in_development")), 18.f, 24.f, baseX, wrappedWidth + 16.f, Color{226, 214, 240, 255});
        y += 8.f;
    }

    addWrappedText(localization_.get(archetype.detailsDescriptionTextId), 18.f, 24.f, baseX, wrappedWidth + 16.f, Color{200, 206, 226, 255});

    if (!archetype.visualIdentityTextId.value.empty()) {
        addSectionTitle(localization_.get(TextId("profile_hub.visual_identity")), archetypeAccentColor(archetype));
        addWrappedText(localization_.get(archetype.visualIdentityTextId), 17.f, 23.f, indentedX, wrappedWidth, Color{220, 224, 238, 255});
    }

    addSectionTitle(localization_.get(TextId("profile_hub.palette")), archetypeAccentColor(archetype));
    addWrappedText(localization_.get(archetype.palette.nameTextId), 17.f, 23.f, indentedX, wrappedWidth, Color{220, 224, 238, 255});

    addSectionTitle(localization_.get(TextId("profile_hub.starting_resources")), Color{240, 235, 210, 255});
    drawTextIfNeeded(localization_.format(TextId("profile_hub.gold_value"), {{"gold", std::to_string(archetype.startingGold)}}), Vector2{indentedX, y}, 19.f, Color{220, 224, 238, 255});
    y += 28.f;

    addSectionTitle(localization_.get(TextId("profile_hub.party")), Color{240, 235, 210, 255});
    for (const std::string& actorId : archetype.actorDefinitionIds) {
        drawTextIfNeeded("* " + actorName(actorId), Vector2{indentedX, y}, 18.f, Color{220, 224, 238, 255});
        y += 24.f;
    }

    addSectionTitle(localization_.get(TextId("profile_hub.starting_relics")), Color{240, 235, 210, 255});
    if (archetype.startingRelicIds.empty()) {
        drawTextIfNeeded(localization_.get(TextId("profile_hub.none_bullet")), Vector2{indentedX, y}, 18.f, Color{190, 196, 216, 255});
        y += 24.f;
    } else {
        for (const std::string& relicId : archetype.startingRelicIds) {
            drawTextIfNeeded("* " + relicName(relicId), Vector2{indentedX, y}, 18.f, Color{220, 224, 238, 255});
            y += 24.f;
        }
    }

    addSectionTitle(localization_.get(TextId("profile_hub.starting_deck")), Color{240, 235, 210, 255});
    const int columns = std::max(1, static_cast<int>((contentBounds.width - 48.f) / 250.f));
    const float columnWidth = std::max(230.f, (contentBounds.width - 48.f) / static_cast<float>(columns));
    int shownCards = 0;
    for (const std::string& cardId : archetype.startingDeckCardIds) {
        const int column = shownCards % columns;
        const int row = shownCards / columns;
        drawTextIfNeeded(
            "* " + cardName(cardId),
            Vector2{indentedX + static_cast<float>(column) * columnWidth, y + static_cast<float>(row) * 24.f},
            17.f,
            Color{220, 224, 238, 255}
        );
        ++shownCards;
    }
    const int rows = std::max(1, (shownCards + columns - 1) / columns);
    y += 24.f * static_cast<float>(rows);

    addSectionTitle(localization_.get(TextId("profile_hub.strengths")), Color{180, 235, 190, 255});
    if (archetype.strengthTextIds.empty()) {
        drawTextIfNeeded(localization_.get(TextId("profile_hub.none_bullet")), Vector2{indentedX, y}, 17.f, Color{190, 196, 216, 255});
        y += 23.f;
    } else {
        for (const TextId& textId : archetype.strengthTextIds) {
            addWrappedText("+ " + localization_.get(textId), 17.f, 23.f, indentedX, wrappedWidth, Color{210, 235, 216, 255});
        }
    }

    addSectionTitle(localization_.get(TextId("profile_hub.weaknesses")), Color{235, 190, 185, 255});
    if (archetype.weaknessTextIds.empty()) {
        drawTextIfNeeded(localization_.get(TextId("profile_hub.none_bullet")), Vector2{indentedX, y}, 17.f, Color{190, 196, 216, 255});
        y += 23.f;
    } else {
        for (const TextId& textId : archetype.weaknessTextIds) {
            addWrappedText("- " + localization_.get(textId), 17.f, 23.f, indentedX, wrappedWidth, Color{235, 210, 208, 255});
        }
    }

    addSectionTitle(localization_.get(TextId("profile_hub.unique_mechanic")), Color{220, 210, 255, 255});
    addWrappedText(localization_.get(archetype.uniqueMechanicTextId), 17.f, 23.f, indentedX, wrappedWidth, Color{220, 224, 238, 255});

    y += 18.f;
    return y + scrollY - contentBounds.y;
}

std::string ProfileHubScene::cardName(const std::string& cardId) const {
    if (!cards_.contains(CardId(cardId))) {
        return cardId;
    }

    return localization_.get(cards_.get(CardId(cardId)).nameTextId);
}

std::string ProfileHubScene::actorName(const std::string& actorId) const {
    if (!actors_.contains(PlayerActorId(actorId))) {
        return actorId;
    }

    return localization_.get(actors_.get(PlayerActorId(actorId)).nameTextId);
}

std::string ProfileHubScene::relicName(const std::string& relicId) const {
    if (!relics_.contains(RelicId(relicId))) {
        return relicId;
    }

    return localization_.get(relics_.get(RelicId(relicId)).nameTextId);
}
