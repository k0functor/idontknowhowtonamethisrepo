#pragma once

#include "ui/PlayerViewModel.hpp"

#include <raylib.h>

class PlayerView {
public:
    void setModel(PlayerViewModel model);
    const PlayerViewModel& model() const;

    void setPosition(Vector2 position);
    void setSize(Vector2 size);

    bool contains(Vector2 worldPosition) const;
    void render(const Font* font, bool hovered) const;

private:
    Rectangle bounds() const;

private:
    PlayerViewModel model_;
    Vector2 position_{160.f, 240.f};
    Vector2 size_{230.f, 180.f};
};
