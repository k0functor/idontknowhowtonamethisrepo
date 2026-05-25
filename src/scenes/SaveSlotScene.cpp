#include "SaveSlotScene.hpp"

#include "ui/BasicUi.hpp"

#include <raylib.h>

SaveSlotScene::SaveSlotScene(
    const UiFont& font,
    const ProfileManager& profiles,
    std::function<void(std::size_t)> onSlotSelected,
    std::function<void()> onBack
)
    : font_(font),
      profiles_(profiles),
      onSlotSelected_(std::move(onSlotSelected)),
      onBack_(std::move(onBack)) {}

void SaveSlotScene::update(float) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        onBack_();
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const float centerX = GetScreenWidth() * 0.5f;
    const float startY = 210.f;
    const float width = 420.f;
    const float height = 92.f;
    const float gap = 20.f;

    for (std::size_t i = 0; i < profiles_.slots().size(); ++i) {
        const Rectangle bounds{centerX - width * 0.5f, startY + static_cast<float>(i) * (height + gap), width, height};
        if (BasicUi::contains(bounds, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onSlotSelected_(i);
            return;
        }
    }

    const Rectangle back{32.f, 32.f, 140.f, 48.f};
    if (BasicUi::contains(back, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
    }
}

void SaveSlotScene::render() const {
    const Vector2 mouse = GetMousePosition();
    const float centerX = GetScreenWidth() * 0.5f;
    const float startY = 210.f;
    const float width = 420.f;
    const float height = 92.f;
    const float gap = 20.f;

    BasicUi::drawButton(font_, Rectangle{32.f, 32.f, 140.f, 48.f}, "Назад", mouse);
    BasicUi::drawCenteredText(font_, "Выбор слота", Rectangle{0.f, 95.f, static_cast<float>(GetScreenWidth()), 70.f}, 40.f, Color{240, 240, 250, 255});

    for (std::size_t i = 0; i < profiles_.slots().size(); ++i) {
        const ProfileData& profile = profiles_.slots()[i].data;
        const Rectangle bounds{centerX - width * 0.5f, startY + static_cast<float>(i) * (height + gap), width, height};

        const std::string label = profile.isEmpty
            ? "Слот " + std::to_string(i + 1) + ": пустой"
            : "Слот " + std::to_string(i + 1) + ": побед " + std::to_string(profile.victories);

        BasicUi::drawButton(font_, bounds, label, mouse);
    }
}
