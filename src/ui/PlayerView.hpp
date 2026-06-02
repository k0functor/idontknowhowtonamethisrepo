#pragma once

#include "ui/PlayerViewModel.hpp"

#include <raylib.h>

#include <cstddef>
#include <optional>

class PlayerView {
public:
    void setModel(PlayerViewModel model);
    const PlayerViewModel& model() const;

    void setPosition(Vector2 position);
    void setSize(Vector2 size);
    void setStatusesOnRight(bool statusesOnRight);

    bool contains(Vector2 worldPosition) const;
    void render(const Font* font, bool hovered) const;
    Rectangle bounds() const;
    std::optional<std::size_t> statusIndexAt(Vector2 worldPosition) const;
    std::optional<Rectangle> statusBounds(std::size_t index) const;

private:
    Rectangle statusAreaBounds() const;

private:
    PlayerViewModel model_;
    Vector2 position_{160.f, 240.f};
    Vector2 size_{230.f, 180.f};
    bool statusesOnRight_ = true;
};
