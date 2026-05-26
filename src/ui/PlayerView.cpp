#include "PlayerView.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

namespace {
bool isStanceStatus(const std::string& id) {
    return id == "stance_flame" || id == "stance_ash" || id == "stance_smoke";
}

std::string shorten(const std::string& text, const std::size_t maxLength) {
    if (text.size() <= maxLength) {
        return text;
    }

    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }

    return text.substr(0, maxLength - 3) + "...";
}
}

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

    DrawTextEx(*font, shorten(model_.name, 22).c_str(), Vector2{position_.x + 14.f, position_.y + 12.f}, 20.f, 1.f, WHITE);

    const std::string hpText = std::to_string(model_.currentHp) + "/" + std::to_string(model_.maxHp);
    DrawTextEx(*font, hpText.c_str(), Vector2{position_.x + 18.f, position_.y + size_.y - 38.f}, 14.f, 1.f, WHITE);

    const std::string energyText = "Energy: " + std::to_string(model_.energy) + "/" + std::to_string(model_.maxEnergy);
    DrawTextEx(*font, energyText.c_str(), Vector2{position_.x + 14.f, position_.y + 42.f}, 15.f, 1.f, Color{255, 220, 130, 255});

    float lineY = position_.y + 64.f;

    if (!model_.stanceName.empty()) {
        const std::string stanceText = "Stance: " + model_.stanceName;
        DrawTextEx(*font, shorten(stanceText, 26).c_str(), Vector2{position_.x + 14.f, lineY}, 14.f, 1.f, Color{255, 185, 120, 255});
        lineY += 20.f;
    }

    if (model_.block > 0) {
        const std::string blockText = "Block: " + std::to_string(model_.block);
        DrawTextEx(*font, blockText.c_str(), Vector2{position_.x + 14.f, lineY}, 15.f, 1.f, Color{180, 220, 255, 255});
        lineY += 22.f;
    }

    std::ostringstream statusText;
    for (const StatusViewModel& status : model_.statuses) {
        if (isStanceStatus(status.id)) {
            continue;
        }

        if (!statusText.str().empty()) {
            statusText << "  ";
        }

        statusText << status.name << ":" << status.amount;
    }

    const std::string statuses = statusText.str();
    if (!statuses.empty()) {
        DrawTextEx(*font, shorten(statuses, 38).c_str(), Vector2{position_.x + 14.f, lineY}, 13.f, 1.f, Color{220, 220, 180, 255});
    }
}

Rectangle PlayerView::bounds() const {
    return Rectangle{position_.x, position_.y, size_.x, size_.y};
}
