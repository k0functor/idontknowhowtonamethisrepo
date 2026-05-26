#pragma once

#include "ui/EnemyViewModel.hpp"

#include <raylib.h>

class EnemyView {
public:
    void setModel(EnemyViewModel model);
    const EnemyViewModel& model() const;

    void setPosition(Vector2 position);

    bool contains(Vector2 worldPosition) const;
    void render(const Font* font, bool hovered) const;
    Rectangle bounds() const;

private:
    EnemyViewModel model_;
    Vector2 position_{900.f, 235.f};
    Vector2 size_{230.f, 180.f};
};
