#include "PlayerView.hpp"

#include <algorithm>
#include <sstream>

void PlayerView::setModel(PlayerViewModel model) {
    model_ = std::move(model);
}

const PlayerViewModel& PlayerView::model() const {
    return model_;
}

void PlayerView::setPosition(const Vector2 position) {
    position_ = position;
}

void PlayerView::setSize(const Vector2 size) {
    size_ = size;
}

bool PlayerView::contains(const Vector2 worldPosition) const {
    return CheckCollisionPointRec(worldPosition, bounds());
}

void PlayerView::render(const Font* font, const bool hovered) const {
    const Rectangle body = bounds();
    const Color fill = model_.alive ? Color{58, 82, 92, 255} : Color{52, 52, 52, 255};
    const Color outline = hovered ? Color{255, 230, 130, 255} : Color{200, 220, 235, 255};

    DrawRectangleRounded(body, 0.12f, 12, fill);
    DrawRectangleRoundedLinesEx(body, 0.12f, 12, hovered ? 4.f : 2.f, outline);

    const float hpRatio = model_.maxHp <= 0
        ? 0.f
        : static_cast<float>(std::max(0, model_.currentHp)) / static_cast<float>(model_.maxHp);

    const Rectangle hpBack{position_.x + 14.f, position_.y + size_.y - 36.f, size_.x - 28.f, 18.f};
    DrawRectangleRec(hpBack, Color{30, 34, 38, 255});

    const Rectangle hpFill{hpBack.x, hpBack.y, hpBack.width * hpRatio, hpBack.height};
    DrawRectangleRec(hpFill, Color{70, 165, 120, 255});

    if (font == nullptr) {
        return;
    }

    DrawTextEx(*font, model_.name.c_str(), Vector2{position_.x + 14.f, position_.y + 12.f}, 20.f, 1.f, WHITE);

    const std::string hpText = std::to_string(model_.currentHp) + "/" + std::to_string(model_.maxHp);
    DrawTextEx(*font, hpText.c_str(), Vector2{position_.x + 18.f, position_.y + size_.y - 38.f}, 14.f, 1.f, WHITE);

    if (model_.block > 0) {
        const std::string blockText = "Block: " + std::to_string(model_.block);
        DrawTextEx(*font, blockText.c_str(), Vector2{position_.x + 14.f, position_.y + 44.f}, 15.f, 1.f, Color{180, 220, 255, 255});
    }

    std::ostringstream statusText;
    for (const StatusViewModel& status : model_.statuses) {
        if (!statusText.str().empty()) {
            statusText << "  ";
        }

        statusText << status.name << ":" << status.amount;
    }

    const std::string statuses = statusText.str();
    if (!statuses.empty()) {
        DrawTextEx(*font, statuses.c_str(), Vector2{position_.x + 14.f, position_.y + 72.f}, 13.f, 1.f, Color{220, 220, 180, 255});
    }
}

Rectangle PlayerView::bounds() const {
    return Rectangle{position_.x, position_.y, size_.x, size_.y};
}
