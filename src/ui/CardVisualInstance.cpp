#include "CardVisualInstance.hpp"

#include "ui/Utf8.hpp"

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

void CardVisualInstance::setModel(CardViewModel model) {
    model_ = std::move(model);
}

const CardViewModel& CardVisualInstance::model() const {
    return model_;
}

void CardVisualInstance::setCurrentTransform(const CardTransform transform) {
    currentTransform_ = transform;
}

void CardVisualInstance::setTargetTransform(const CardTransform transform) {
    targetTransform_ = transform;
}

const CardTransform& CardVisualInstance::currentTransform() const {
    return currentTransform_;
}

const CardTransform& CardVisualInstance::targetTransform() const {
    return targetTransform_;
}

void CardVisualInstance::update(const float deltaSeconds) {
    const float speed = 18.f;
    const float alpha = 1.f - std::exp(-speed * deltaSeconds);

    currentTransform_.position.x = approach(currentTransform_.position.x, targetTransform_.position.x, alpha);
    currentTransform_.position.y = approach(currentTransform_.position.y, targetTransform_.position.y, alpha);
    currentTransform_.scale.x = approach(currentTransform_.scale.x, targetTransform_.scale.x, alpha);
    currentTransform_.scale.y = approach(currentTransform_.scale.y, targetTransform_.scale.y, alpha);
    currentTransform_.rotationDegrees = approach(currentTransform_.rotationDegrees, targetTransform_.rotationDegrees, alpha);
    currentTransform_.zIndex = targetTransform_.zIndex;
}

void CardVisualInstance::render(const Font* font) const {
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
        UiUtf8::wrapByCodepoints(model_.name, 17, 2),
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
        Vector2{-cardSize.x * 0.5f + 15.f, -cardSize.y * 0.5f + 129.f},
        11.f,
        1.f,
        Color{210, 210, 210, 255}
    );

    if (!model_.ownerLabel.empty()) {
        drawTextLocal(
            font,
            currentTransform_,
            UiUtf8::truncateWithEllipsis(model_.ownerLabel, 22),
            Vector2{-cardSize.x * 0.5f + 15.f, -cardSize.y * 0.5f + 143.f},
            10.f,
            1.f,
            model_.playable ? Color{185, 195, 215, 255} : Color{145, 145, 152, 255}
        );
    }

    drawTextLocal(
        font,
        currentTransform_,
        UiUtf8::wrapByCodepoints(model_.description, 24, 5),
        Vector2{-cardSize.x * 0.5f + 15.f, -cardSize.y * 0.5f + 160.f},
        12.f,
        1.f,
        model_.playable ? WHITE : Color{170, 170, 170, 255}
    );

    if (!model_.playable) {
        drawLocalRectangle(
            currentTransform_,
            Vector2{-cardSize.x * 0.5f + 8.f, -cardSize.y * 0.5f + 8.f},
            Vector2{cardSize.x - 16.f, cardSize.y - 16.f},
            Color{0, 0, 0, 72}
        );

        if (!model_.unplayableReason.empty()) {
            drawLocalRectangle(
                currentTransform_,
                Vector2{-cardSize.x * 0.5f + 10.f, cardSize.y * 0.5f - 44.f},
                Vector2{cardSize.x - 20.f, 32.f},
                Color{34, 24, 26, 220}
            );

            drawTextLocal(
                font,
                currentTransform_,
                UiUtf8::truncateWithEllipsis(model_.unplayableReason, 22),
                Vector2{-cardSize.x * 0.5f + 16.f, cardSize.y * 0.5f - 36.f},
                10.f,
                1.f,
                Color{255, 205, 190, 255}
            );
        }
    }

}

CardTransform CardVisualInstance::transformForBounds(
    const Rectangle bounds,
    const int zIndex,
    const float fill
) {
    const Vector2 cardSize = size();
    const float safeFill = std::clamp(fill, 0.05f, 1.25f);
    const float scale = std::max(0.01f, std::min(bounds.width / cardSize.x, bounds.height / cardSize.y) * safeFill);

    return CardTransform{
        Vector2{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f},
        Vector2{scale, scale},
        0.f,
        zIndex
    };
}

void CardVisualInstance::renderStatic(
    const CardViewModel& model,
    const Font* font,
    const CardTransform transform
) {
    CardVisualInstance instance;
    instance.setModel(model);
    instance.setCurrentTransform(transform);
    instance.setTargetTransform(transform);
    instance.render(font);
}


bool CardVisualInstance::contains(const Vector2 worldPosition) const {
    return containsAtTransform(worldPosition, currentTransform_);
}

bool CardVisualInstance::containsAtTransform(
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

Vector2 CardVisualInstance::center() const {
    return currentTransform_.position;
}

int CardVisualInstance::zIndex() const {
    return currentTransform_.zIndex;
}

Vector2 CardVisualInstance::size() {
    return Vector2{170.f, 240.f};
}

Vector2 CardVisualInstance::localToWorld(const Vector2 localPosition) const {
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

Vector2 CardVisualInstance::worldToLocal(const Vector2 worldPosition) const {
    return worldToLocalUsingTransform(worldPosition, currentTransform_);
}

Vector2 CardVisualInstance::worldToLocalUsingTransform(
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

float CardVisualInstance::approach(const float current, const float target, const float alpha) {
    return current + (target - current) * std::clamp(alpha, 0.f, 1.f);
}
