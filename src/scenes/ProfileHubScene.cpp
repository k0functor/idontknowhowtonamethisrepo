#include "ProfileHubScene.hpp"

#include "cards/CardId.hpp"
#include "ui/BasicUi.hpp"

#include <raylib.h>

#include <algorithm>
#include <cstdint>

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
    Rectangle startButton{};
    Rectangle challengesButton{};
    Rectangle achievementsButton{};
    Rectangle compendiumButton{};
    Rectangle notification{};
};

ProfileHubLayout calculateProfileHubLayout() {
    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());

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

    layout.startButton = Rectangle{sideX, sideY, sideButtonWidth, sideButtonHeight};
    layout.challengesButton = Rectangle{sideX, sideY + (sideButtonHeight + sideButtonGap), sideButtonWidth, sideButtonHeight};
    layout.achievementsButton = Rectangle{sideX, sideY + 2.f * (sideButtonHeight + sideButtonGap), sideButtonWidth, sideButtonHeight};
    layout.compendiumButton = Rectangle{sideX, sideY + 3.f * (sideButtonHeight + sideButtonGap), sideButtonWidth, sideButtonHeight};

    layout.notification = Rectangle{0.f, screenHeight - 50.f, screenWidth, 32.f};

    return layout;
}
}

ProfileHubScene::ProfileHubScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const PlayerActorDatabase& actors,
    const CardDatabase& cards,
    std::vector<const PlayableArchetypeDefinition*> archetypes,
    std::function<void(PlayableArchetypeId)> onStartRun,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      actors_(actors),
      cards_(cards),
      archetypes_(std::move(archetypes)),
      onStartRun_(std::move(onStartRun)),
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

    if (IsKeyPressed(KEY_ESCAPE)) {
        onBack_();
        return;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        moveSelection(-1);
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        moveSelection(1);
    }

    if (IsKeyPressed(KEY_SPACE)) {
        detailsOpen_ = true;
        return;
    }

    const ProfileHubLayout layout = calculateProfileHubLayout();

    if (BasicUi::contains(layout.leftArrow, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        moveSelection(-1);
    }

    if (BasicUi::contains(layout.rightArrow, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        moveSelection(1);
    }

    if (BasicUi::contains(layout.startButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !archetypes_.empty()) {
        onStartRun_(selectedArchetype().id);
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

    const ProfileHubLayout layout = calculateProfileHubLayout();

    BasicUi::drawButton(font_, layout.backButton, localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(font_, localization_.get(TextId("profile_hub.title")), layout.title, 38.f, Color{240, 240, 250, 255});

    BasicUi::drawButton(font_, layout.leftArrow, "<", mouse, !archetypes_.empty());
    BasicUi::drawButton(font_, layout.rightArrow, ">", mouse, !archetypes_.empty());

    const Rectangle characterPanel = layout.characterPanel;
    DrawRectangleRounded(characterPanel, 0.06f, 12, Color{31, 34, 44, 255});
    DrawRectangleRoundedLinesEx(characterPanel, 0.06f, 12, 2.f, Color{100, 110, 145, 255});

    if (!archetypes_.empty()) {
        const PlayableArchetypeDefinition& archetype = selectedArchetype();
        BasicUi::drawCenteredText(font_, localization_.get(archetype.nameTextId), Rectangle{characterPanel.x, characterPanel.y + 40.f, characterPanel.width, 50.f}, 34.f, Color{245, 245, 250, 255});
        BasicUi::drawCenteredText(font_, localization_.get(archetype.shortDescriptionTextId), Rectangle{characterPanel.x + 50.f, characterPanel.y + 100.f, characterPanel.width - 100.f, 60.f}, 20.f, Color{190, 198, 220, 255});

        const Color accent = archetypeAccentColor(archetype);
        DrawCircle(static_cast<int>(characterPanel.x + characterPanel.width * 0.5f), static_cast<int>(characterPanel.y + 265.f), 96.f, archetypeAccentColor(archetype, 70));
        DrawCircleLines(static_cast<int>(characterPanel.x + characterPanel.width * 0.5f), static_cast<int>(characterPanel.y + 265.f), 96.f, accent);
        DrawCircle(static_cast<int>(characterPanel.x + characterPanel.width * 0.5f), static_cast<int>(characterPanel.y + 265.f), 82.f, Color{75, 79, 96, 255});
        BasicUi::drawCenteredText(font_, localization_.get(TextId("ui.portrait_placeholder")), Rectangle{characterPanel.x, characterPanel.y + 240.f, characterPanel.width, 40.f}, 18.f, Color{160, 166, 190, 255});
        BasicUi::drawCenteredText(font_, localization_.get(archetype.palette.nameTextId), Rectangle{characterPanel.x + 45.f, characterPanel.y + 355.f, characterPanel.width - 90.f, 28.f}, 16.f, accent);

        BasicUi::drawCenteredText(font_, localization_.get(TextId("profile_hub.details_hint")), Rectangle{characterPanel.x, characterPanel.y + 405.f, characterPanel.width, 40.f}, 20.f, Color{220, 220, 235, 255});
    }

    BasicUi::drawButton(font_, layout.startButton, localization_.get(TextId("ui.to_the_road")), mouse, !archetypes_.empty());
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

    const Vector2 mouse = GetMousePosition();
    const Rectangle closeButton{GetScreenWidth() * 0.5f - 90.f, GetScreenHeight() - 96.f, 180.f, 48.f};

    if (BasicUi::contains(closeButton, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        detailsOpen_ = false;
    }
}

void ProfileHubScene::renderDetailsModal() const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 145});

    const PlayableArchetypeDefinition& archetype = selectedArchetype();
    const Rectangle modal{GetScreenWidth() * 0.5f - 450.f, 70.f, 900.f, GetScreenHeight() - 140.f};
    DrawRectangleRounded(modal, 0.035f, 12, Color{28, 31, 42, 255});
    DrawRectangleRoundedLinesEx(modal, 0.035f, 12, 2.f, Color{140, 150, 185, 255});

    float y = modal.y + 28.f;
    BasicUi::drawCenteredText(font_, localization_.get(archetype.nameTextId), Rectangle{modal.x, y, modal.width, 44.f}, 32.f, Color{245, 245, 250, 255});
    y += 62.f;

    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(archetype.detailsDescriptionTextId), 18.f, modal.width - 80.f)) {
        BasicUi::drawText(font_, line, Vector2{modal.x + 40.f, y}, 18.f, Color{200, 206, 226, 255});
        y += 24.f;
    }

    if (!archetype.visualIdentityTextId.value.empty()) {
        y += 18.f;
        BasicUi::drawText(font_, localization_.get(TextId("profile_hub.visual_identity")), Vector2{modal.x + 40.f, y}, 23.f, archetypeAccentColor(archetype));
        y += 32.f;
        for (const std::string& line : BasicUi::wrapText(font_, localization_.get(archetype.visualIdentityTextId), 17.f, modal.width - 100.f)) {
            BasicUi::drawText(font_, line, Vector2{modal.x + 62.f, y}, 17.f, Color{220, 224, 238, 255});
            y += 23.f;
        }
    }

    y += 14.f;
    BasicUi::drawText(font_, localization_.get(TextId("profile_hub.palette")), Vector2{modal.x + 40.f, y}, 23.f, archetypeAccentColor(archetype));
    y += 32.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(archetype.palette.nameTextId), 17.f, modal.width - 100.f)) {
        BasicUi::drawText(font_, line, Vector2{modal.x + 62.f, y}, 17.f, Color{220, 224, 238, 255});
        y += 23.f;
    }

    y += 18.f;
    BasicUi::drawText(font_, "Стартовые ресурсы", Vector2{modal.x + 40.f, y}, 23.f, Color{240, 235, 210, 255});
    y += 34.f;
    BasicUi::drawText(font_, "Золото: " + std::to_string(archetype.startingGold), Vector2{modal.x + 62.f, y}, 19.f, Color{220, 224, 238, 255});
    y += 28.f;

    BasicUi::drawText(font_, "Состав", Vector2{modal.x + 40.f, y}, 23.f, Color{240, 235, 210, 255});
    y += 32.f;
    for (const std::string& actorId : archetype.actorDefinitionIds) {
        BasicUi::drawText(font_, "• " + actorName(actorId), Vector2{modal.x + 62.f, y}, 18.f, Color{220, 224, 238, 255});
        y += 24.f;
    }

    y += 10.f;
    BasicUi::drawText(font_, "Стартовая реликвия", Vector2{modal.x + 40.f, y}, 23.f, Color{240, 235, 210, 255});
    y += 32.f;
    if (archetype.startingRelicIds.empty()) {
        BasicUi::drawText(font_, "• нет", Vector2{modal.x + 62.f, y}, 18.f, Color{190, 196, 216, 255});
        y += 24.f;
    } else {
        for (const std::string& relicId : archetype.startingRelicIds) {
            BasicUi::drawText(font_, "• " + relicId, Vector2{modal.x + 62.f, y}, 18.f, Color{220, 224, 238, 255});
            y += 24.f;
        }
    }

    y += 10.f;
    BasicUi::drawText(font_, "Стартовая колода", Vector2{modal.x + 40.f, y}, 23.f, Color{240, 235, 210, 255});
    y += 32.f;
    int shownCards = 0;
    for (const std::string& cardId : archetype.startingDeckCardIds) {
        BasicUi::drawText(font_, "• " + cardName(cardId), Vector2{modal.x + 62.f + static_cast<float>(shownCards / 6) * 260.f, y + static_cast<float>(shownCards % 6) * 24.f}, 17.f, Color{220, 224, 238, 255});
        ++shownCards;
    }
    y += 24.f * static_cast<float>(std::min(6, std::max(1, shownCards))) + 18.f;

    BasicUi::drawText(font_, "Сильные стороны", Vector2{modal.x + 40.f, y}, 23.f, Color{180, 235, 190, 255});
    y += 32.f;
    for (const TextId& textId : archetype.strengthTextIds) {
        BasicUi::drawText(font_, "+ " + localization_.get(textId), Vector2{modal.x + 62.f, y}, 17.f, Color{210, 235, 216, 255});
        y += 23.f;
    }

    y += 10.f;
    BasicUi::drawText(font_, "Слабые стороны", Vector2{modal.x + 40.f, y}, 23.f, Color{235, 190, 185, 255});
    y += 32.f;
    for (const TextId& textId : archetype.weaknessTextIds) {
        BasicUi::drawText(font_, "- " + localization_.get(textId), Vector2{modal.x + 62.f, y}, 17.f, Color{235, 210, 208, 255});
        y += 23.f;
    }

    y += 10.f;
    BasicUi::drawText(font_, "Уникальная механика", Vector2{modal.x + 40.f, y}, 23.f, Color{220, 210, 255, 255});
    y += 32.f;
    for (const std::string& line : BasicUi::wrapText(font_, localization_.get(archetype.uniqueMechanicTextId), 17.f, modal.width - 100.f)) {
        BasicUi::drawText(font_, line, Vector2{modal.x + 62.f, y}, 17.f, Color{220, 224, 238, 255});
        y += 23.f;
    }

    BasicUi::drawButton(font_, Rectangle{GetScreenWidth() * 0.5f - 90.f, GetScreenHeight() - 96.f, 180.f, 48.f}, "Закрыть", GetMousePosition());
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
