#include "CardView.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace {
Color cardFillColor(const CardViewModel& model) {
    if (!model.playable) {
        return Color{80, 80, 86, 255};
    }

    switch (model.type) {
        case CardType::Attack:
            return Color{118, 72, 72, 255};
        case CardType::Skill:
            return Color{72, 96, 118, 255};
        case CardType::Power:
            return Color{95, 76, 124, 255};
        case CardType::Status:
            return Color{96, 96, 96, 255};
        case CardType::Curse:
            return Color{58, 50, 66, 255};
    }

    return Color{90, 90, 96, 255};
}

std::string truncateLine(const std::string& text, const std::size_t limit) {
    if (text.size() <= limit) {
        return text;
    }

    if (limit <= 3) {
        return text.substr(0, limit);
    }

    return text.substr(0, limit - 3) + "...";
}

std::string wrapText(const std::string& text, const std::size_t lineLength, const std::size_t maxLines) {
    std::istringstream input(text);
    std::string word;
    std::string result;
    std::string line;
    std::size_t lines = 0;

    while (input >> word) {
        if (line.empty()) {
            line = word;
        } else if (line.size() + 1 + word.size() <= lineLength) {
            line += " " + word;
        } else {
            if (!result.empty()) {
                result += '\n';
            }

            result += truncateLine(line, lineLength);
            ++lines;

            if (lines >= maxLines) {
                return result;
            }

            line = word;
        }
    }

    if (!line.empty() && lines < maxLines) {
        if (!result.empty()) {
            result += '\n';
        }

        result += truncateLine(line, lineLength);
    }

    return result;
}

float uniformScale(const CardTransform& transform) {
    return (transform.scale.x + transform.scale.y) * 0.5f;
}

void drawTextLocal(
    const Font* font,
    const CardTransform& transform,
    const std::string& text,
    const Vector2 localPosition,
    const float fontSize,
    const float spacing,
    const Color color
) {
    if (font == nullptr) {
        return;
    }

    const float radians = transform.rotationDegrees * DEG2RAD;
    const float cosValue = std::cos(radians);
    const float sinValue = std::sin(radians);

    const Vector2 scaledLocal{
        localPosition.x * transform.scale.x,
        localPosition.y * transform.scale.y
    };

    const Vector2 world{
        transform.position.x + scaledLocal.x * cosValue - scaledLocal.y * sinValue,
        transform.position.y + scaledLocal.x * sinValue + scaledLocal.y * cosValue
    };

    DrawTextPro(
        *font,
        text.c_str(),
        world,
        Vector2{0.f, 0.f},
        transform.rotationDegrees,
        fontSize * uniformScale(transform),
        spacing,
        color
    );
}

void drawLocalRectangle(
    const CardTransform& transform,
    const Vector2 localTopLeft,
    const Vector2 localSize,
    const Color color
) {
    const Vector2 localCenter{
        localTopLeft.x + localSize.x * 0.5f,
        localTopLeft.y + localSize.y * 0.5f
    };

    const float radians = transform.rotationDegrees * DEG2RAD;
    const float cosValue = std::cos(radians);
    const float sinValue = std::sin(radians);

    const Vector2 scaledCenter{
        localCenter.x * transform.scale.x,
        localCenter.y * transform.scale.y
    };

    const Vector2 worldCenter{
        transform.position.x + scaledCenter.x * cosValue - scaledCenter.y * sinValue,
        transform.position.y + scaledCenter.x * sinValue + scaledCenter.y * cosValue
    };

    const Vector2 scaledSize{
        localSize.x * transform.scale.x,
        localSize.y * transform.scale.y
    };

    DrawRectanglePro(
        Rectangle{worldCenter.x, worldCenter.y, scaledSize.x, scaledSize.y},
        Vector2{scaledSize.x * 0.5f, scaledSize.y * 0.5f},
        transform.rotationDegrees,
        color
    );
}
}

void CardView::setModel(CardViewModel model) {
    model_ = std::move(model);
}

const CardViewModel& CardView::model() const {
    return model_;
}

void CardView::setCurrentTransform(const CardTransform transform) {
    currentTransform_ = transform;
}

void CardView::setTargetTransform(const CardTransform transform) {
    targetTransform_ = transform;
}

const CardTransform& CardView::currentTransform() const {
    return currentTransform_;
}

const CardTransform& CardView::targetTransform() const {
    return targetTransform_;
}

void CardView::update(const float deltaSeconds) {
    const float speed = 18.f;
    const float alpha = 1.f - std::exp(-speed * deltaSeconds);

    currentTransform_.position.x = approach(currentTransform_.position.x, targetTransform_.position.x, alpha);
    currentTransform_.position.y = approach(currentTransform_.position.y, targetTransform_.position.y, alpha);
    currentTransform_.scale.x = approach(currentTransform_.scale.x, targetTransform_.scale.x, alpha);
    currentTransform_.scale.y = approach(currentTransform_.scale.y, targetTransform_.scale.y, alpha);
    currentTransform_.rotationDegrees = approach(currentTransform_.rotationDegrees, targetTransform_.rotationDegrees, alpha);
    currentTransform_.zIndex = targetTransform_.zIndex;
}

void CardView::render(const Font* font) const {
    const Vector2 cardSize = size();
    const float outlineThickness = model_.selected ? 5.f : 2.f;
    const Color outlineColor = model_.selected
        ? Color{255, 218, 90, 255}
        : (model_.playable ? Color{220, 220, 220, 255} : Color{130, 130, 130, 255});

    drawLocalRectangle(
        currentTransform_,
        Vector2{-cardSize.x * 0.5f, -cardSize.y * 0.5f},
        cardSize,
        outlineColor
    );

    drawLocalRectangle(
        currentTransform_,
        Vector2{-cardSize.x * 0.5f + outlineThickness, -cardSize.y * 0.5f + outlineThickness},
        Vector2{cardSize.x - outlineThickness * 2.f, cardSize.y - outlineThickness * 2.f},
        cardFillColor(model_)
    );

    const Vector2 costCenter = localToWorld(Vector2{-cardSize.x * 0.5f + 25.f, -cardSize.y * 0.5f + 25.f});
    DrawCircleV(costCenter, 18.f * uniformScale(currentTransform_), Color{30, 34, 46, 255});
    DrawCircleLines(static_cast<int>(costCenter.x), static_cast<int>(costCenter.y), 18.f * uniformScale(currentTransform_), Color{220, 220, 220, 255});

    if (font == nullptr) {
        return;
    }

    drawTextLocal(
        font,
        currentTransform_,
        std::to_string(model_.energyCost),
        Vector2{-cardSize.x * 0.5f + 18.f, -cardSize.y * 0.5f + 12.f},
        18.f,
        1.f,
        WHITE
    );

    drawTextLocal(
        font,
        currentTransform_,
        wrapText(model_.name, 17, 2),
        Vector2{-cardSize.x * 0.5f + 48.f, -cardSize.y * 0.5f + 12.f},
        15.f,
        1.f,
        WHITE
    );

    drawLocalRectangle(
        currentTransform_,
        Vector2{-cardSize.x * 0.5f + 13.f, -cardSize.y * 0.5f + 58.f},
        Vector2{cardSize.x - 26.f, 68.f},
        Color{38, 42, 52, 255}
    );

    drawTextLocal(
        font,
        currentTransform_,
        toString(model_.type) + " / " + toString(model_.rarity),
        Vector2{-cardSize.x * 0.5f + 15.f, -cardSize.y * 0.5f + 132.f},
        11.f,
        1.f,
        Color{210, 210, 210, 255}
    );

    drawTextLocal(
        font,
        currentTransform_,
        wrapText(model_.description, 24, 5),
        Vector2{-cardSize.x * 0.5f + 15.f, -cardSize.y * 0.5f + 154.f},
        12.f,
        1.f,
        model_.playable ? WHITE : Color{170, 170, 170, 255}
    );
}

bool CardView::contains(const Vector2 worldPosition) const {
    return containsAtTransform(worldPosition, currentTransform_);
}

bool CardView::containsAtTransform(
    const Vector2 worldPosition,
    const CardTransform& transform
) const {
    const Vector2 local = worldToLocalUsingTransform(worldPosition, transform);
    const Vector2 cardSize = size();

    return local.x >= -cardSize.x * 0.5f &&
           local.x <= cardSize.x * 0.5f &&
           local.y >= -cardSize.y * 0.5f &&
           local.y <= cardSize.y * 0.5f;
}

Vector2 CardView::center() const {
    return currentTransform_.position;
}

int CardView::zIndex() const {
    return currentTransform_.zIndex;
}

Vector2 CardView::size() {
    return Vector2{170.f, 240.f};
}

Vector2 CardView::localToWorld(const Vector2 localPosition) const {
    const float radians = currentTransform_.rotationDegrees * DEG2RAD;
    const float cosValue = std::cos(radians);
    const float sinValue = std::sin(radians);

    const Vector2 scaled{
        localPosition.x * currentTransform_.scale.x,
        localPosition.y * currentTransform_.scale.y
    };

    return Vector2{
        currentTransform_.position.x + scaled.x * cosValue - scaled.y * sinValue,
        currentTransform_.position.y + scaled.x * sinValue + scaled.y * cosValue
    };
}

Vector2 CardView::worldToLocal(const Vector2 worldPosition) const {
    return worldToLocalUsingTransform(worldPosition, currentTransform_);
}

Vector2 CardView::worldToLocalUsingTransform(
    const Vector2 worldPosition,
    const CardTransform& transform
) {
    const float radians = -transform.rotationDegrees * DEG2RAD;
    const float cosValue = std::cos(radians);
    const float sinValue = std::sin(radians);

    const Vector2 translated{
        worldPosition.x - transform.position.x,
        worldPosition.y - transform.position.y
    };

    const Vector2 unrotated{
        translated.x * cosValue - translated.y * sinValue,
        translated.x * sinValue + translated.y * cosValue
    };

    return Vector2{
        transform.scale.x == 0.f ? 0.f : unrotated.x / transform.scale.x,
        transform.scale.y == 0.f ? 0.f : unrotated.y / transform.scale.y
    };
}

float CardView::approach(const float current, const float target, const float alpha) {
    return current + (target - current) * std::clamp(alpha, 0.f, 1.f);
}
