#include "RelicInspectModal.hpp"

#include "ui/BasicUi.hpp"

#include <algorithm>
#include <string>

namespace {
Rectangle modalBounds() {
    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());

    const float width = std::min(560.f, screenWidth - 80.f);
    const float height = 300.f;

    return Rectangle{
        (screenWidth - width) * 0.5f,
        (screenHeight - height) * 0.5f,
        width,
        height
    };
}

Rectangle leftButtonBounds(const Rectangle bounds) {
    return Rectangle{
        bounds.x + 22.f,
        bounds.y + bounds.height - 58.f,
        44.f,
        36.f
    };
}

Rectangle rightButtonBounds(const Rectangle bounds) {
    return Rectangle{
        bounds.x + bounds.width - 66.f,
        bounds.y + bounds.height - 58.f,
        44.f,
        36.f
    };
}

Color buttonFill() {
    return Color{48, 45, 56, 255};
}

Color buttonBorder() {
    return Color{238, 196, 86, 255};
}

void drawNavigationButton(
    const UiFont& font,
    const Rectangle bounds,
    const std::string& label
) {
    DrawRectangleRounded(bounds, 0.2f, 8, buttonFill());
    DrawRectangleRoundedLinesEx(bounds, 0.2f, 8, 2.f, buttonBorder());

    BasicUi::drawCenteredText(
        font,
        label,
        bounds,
        22.f,
        Color{255, 234, 170, 255}
    );
}
}

void RelicInspectModal::open(const std::size_t index) {
    currentIndex_ = index;
}

void RelicInspectModal::close() {
    currentIndex_.reset();
}

bool RelicInspectModal::isOpen() const {
    return currentIndex_.has_value();
}

std::optional<std::size_t> RelicInspectModal::currentIndex() const {
    return currentIndex_;
}

void RelicInspectModal::update(const std::size_t relicCount) {
    if (!currentIndex_.has_value()) {
        return;
    }

    if (relicCount == 0) {
        close();
        return;
    }

    if (*currentIndex_ >= relicCount) {
        currentIndex_ = relicCount - 1;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        close();
        return;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        previous(relicCount);
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        next(relicCount);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle bounds = modalBounds();
        const Rectangle leftButton = leftButtonBounds(bounds);
        const Rectangle rightButton = rightButtonBounds(bounds);

        if (CheckCollisionPointRec(mouse, leftButton)) {
            previous(relicCount);
            return;
        }

        if (CheckCollisionPointRec(mouse, rightButton)) {
            next(relicCount);
            return;
        }

        if (!CheckCollisionPointRec(mouse, bounds)) {
            close();
        }
    }
}

void RelicInspectModal::render(
    const UiFont& font,
    const LocalizationManager& localization,
    const std::vector<RelicViewModel>& relics
) const {
    if (!currentIndex_.has_value() || relics.empty() || *currentIndex_ >= relics.size()) {
        return;
    }

    const RelicViewModel& relic = relics[*currentIndex_];

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 120});

    const Rectangle bounds = modalBounds();

    DrawRectangleRounded(bounds, 0.08f, 12, Color{28, 26, 32, 245});
    DrawRectangleRoundedLinesEx(bounds, 0.08f, 12, 3.f, Color{238, 196, 86, 255});

    const Rectangle icon{bounds.x + 24.f, bounds.y + 34.f, 82.f, 82.f};
    DrawRectangleRounded(icon, 0.18f, 8, Color{72, 58, 34, 255});
    DrawRectangleRoundedLinesEx(icon, 0.18f, 8, 2.f, Color{238, 196, 86, 255});

    BasicUi::drawText(
        font,
        relic.name,
        Vector2{bounds.x + 126.f, bounds.y + 34.f},
        26.f,
        Color{255, 234, 170, 255}
    );

    float y = bounds.y + 76.f;
    const Rectangle textBounds{bounds.x + 126.f, y, bounds.width - 154.f, bounds.height - 130.f};

    for (const std::string& line : BasicUi::wrapText(font, relic.description, 17.f, textBounds.width)) {
        if (y + 20.f > textBounds.y + textBounds.height) {
            break;
        }

        BasicUi::drawText(
            font,
            line,
            Vector2{textBounds.x, y},
            17.f,
            Color{222, 224, 235, 255}
        );
        y += 23.f;
    }

    const Rectangle leftButton = leftButtonBounds(bounds);
    const Rectangle rightButton = rightButtonBounds(bounds);

    drawNavigationButton(font, leftButton, "<");
    drawNavigationButton(font, rightButton, ">");

    const std::string counter = std::to_string(*currentIndex_ + 1) + "/" + std::to_string(relics.size());
    BasicUi::drawCenteredText(
        font,
        counter,
        Rectangle{bounds.x + 76.f, bounds.y + bounds.height - 58.f, bounds.width - 152.f, 36.f},
        16.f,
        Color{185, 190, 205, 255}
    );

    BasicUi::drawCenteredText(
        font,
        localization.get(TextId("ui.relic_inspect_close_hint")),
        Rectangle{bounds.x + 90.f, bounds.y + bounds.height - 28.f, bounds.width - 180.f, 18.f},
        12.f,
        Color{140, 145, 160, 255}
    );
}

void RelicInspectModal::previous(const std::size_t relicCount) {
    if (!currentIndex_.has_value() || relicCount == 0) {
        return;
    }

    currentIndex_ = *currentIndex_ == 0
        ? relicCount - 1
        : *currentIndex_ - 1;
}

void RelicInspectModal::next(const std::size_t relicCount) {
    if (!currentIndex_.has_value() || relicCount == 0) {
        return;
    }

    currentIndex_ = (*currentIndex_ + 1) % relicCount;
}
