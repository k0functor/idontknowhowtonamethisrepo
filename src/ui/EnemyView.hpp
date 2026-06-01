#pragma once

#include "ui/EnemyViewModel.hpp"

#include <raylib.h>

#include <cstddef>
#include <optional>

class EnemyView {
public:
    void setModel(EnemyViewModel model);
    const EnemyViewModel& model() const;

    void setPosition(Vector2 position);

    bool contains(Vector2 worldPosition) const;
    void render(const Font* font, bool hovered) const;
    Rectangle bounds() const;
    std::optional<std::size_t> statusIndexAt(Vector2 worldPosition) const;
    std::optional<Rectangle> statusBounds(std::size_t index) const;

private:
    Rectangle statusAreaBounds() const;

private:
    EnemyViewModel model_;
    Vector2 position_{900.f, 235.f};
    Vector2 size_{230.f, 180.f};
};
