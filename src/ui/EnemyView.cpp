#include "EnemyView.hpp"

#include <algorithm>
#include <sstream>

void EnemyView::setModel(EnemyViewModel model) {
    model_ = std::move(model);
}

const EnemyViewModel& EnemyView::model() const {
    return model_;
}

void EnemyView::setPosition(const Vector2 position) {
    position_ = position;
}

bool EnemyView::contains(const Vector2 worldPosition) const {
    return CheckCollisionPointRec(worldPosition, bounds());
}

void EnemyView::render(const Font* font, const bool hovered) const {
    const Rectangle body{position_.x, position_.y, size_.x, size_.y};
    const Color fill = model_.alive ? Color{82, 58, 58, 255} : Color{52, 52, 52, 255};
    const Color outline = hovered ? Color{255, 220, 120, 255} : Color{210, 210, 210, 255};

    DrawRectangleRec(body, fill);
    DrawRectangleLinesEx(body, hovered ? 4.f : 2.f, outline);

    const float hpRatio = model_.maxHp <= 0
        ? 0.f
        : static_cast<float>(std::max(0, model_.currentHp)) / static_cast<float>(model_.maxHp);

    const Rectangle hpBack{position_.x + 14.f, position_.y + size_.y - 36.f, size_.x - 28.f, 18.f};
    DrawRectangleRec(hpBack, Color{32, 32, 36, 255});

    const Rectangle hpFill{hpBack.x, hpBack.y, hpBack.width * hpRatio, hpBack.height};
    DrawRectangleRec(hpFill, Color{160, 60, 60, 255});

    if (font == nullptr) {
        return;
    }

    DrawTextEx(*font, model_.name.c_str(), Vector2{position_.x + 14.f, position_.y + 12.f}, 20.f, 1.f, WHITE);

    const std::string hpText = std::to_string(model_.currentHp) + "/" + std::to_string(model_.maxHp);
    DrawTextEx(*font, hpText.c_str(), Vector2{position_.x + 18.f, position_.y + size_.y - 38.f}, 14.f, 1.f, WHITE);

    if (model_.block > 0) {
        const std::string blockText = "Block: " + std::to_string(model_.block);
        DrawTextEx(*font, blockText.c_str(), Vector2{position_.x + 14.f, position_.y + 46.f}, 15.f, 1.f, Color{180, 215, 255, 255});
    }

    std::ostringstream statusText;
    for (const auto& [statusId, amount] : model_.statuses) {
        if (!statusText.str().empty()) {
            statusText << "  ";
        }

        statusText << statusId << ":" << amount;
    }

    const std::string statuses = statusText.str();
    if (!statuses.empty()) {
        DrawTextEx(*font, statuses.c_str(), Vector2{position_.x + 14.f, position_.y + 74.f}, 13.f, 1.f, Color{220, 220, 180, 255});
    }
}

Rectangle EnemyView::bounds() const {
    return Rectangle{position_.x, position_.y, size_.x, size_.y};
}
