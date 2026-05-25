#include "HandLayout.hpp"

#include <algorithm>
#include <cmath>

HandLayout::HandLayout(Config config)
    : config_(config) {}

void HandLayout::setConfig(Config config) {
    config_ = config;
}

const HandLayout::Config& HandLayout::config() const {
    return config_;
}

std::vector<CardTransform> HandLayout::calculateBaseTransforms(const std::size_t cardCount) const {
    std::vector<CardTransform> transforms;
    transforms.reserve(cardCount);

    if (cardCount == 0) {
        return transforms;
    }

    const float naturalWidth = config_.cardSpacing * static_cast<float>(cardCount > 0 ? cardCount - 1 : 0);
    const float actualSpacing = cardCount <= 1
        ? 0.f
        : (naturalWidth > config_.maxTotalWidth
            ? config_.maxTotalWidth / static_cast<float>(cardCount - 1)
            : config_.cardSpacing);

    for (std::size_t i = 0; i < cardCount; ++i) {
        const float t = cardCount == 1
            ? 0.5f
            : static_cast<float>(i) / static_cast<float>(cardCount - 1);
        const float centered = t - 0.5f;

        CardTransform transform;
        transform.position.x = config_.centerX + centered * actualSpacing * static_cast<float>(cardCount - 1);
        transform.position.y = config_.centerY + std::abs(centered) * config_.arcHeight;
        transform.rotationDegrees = centered * config_.maxRotationDegrees;
        transform.scale = {1.f, 1.f};
        transform.zIndex = static_cast<int>(i);

        transforms.push_back(transform);
    }

    return transforms;
}

CardTransform HandLayout::transformForState(
    CardTransform baseTransform,
    const std::size_t currentIndex,
    const std::size_t,
    const bool hovered,
    const bool selected,
    const bool anyCardElevated,
    const std::size_t elevatedIndex
) const {
    if (anyCardElevated && currentIndex != elevatedIndex) {
        if (currentIndex < elevatedIndex) {
            baseTransform.position.x -= config_.neighborPush;
        } else {
            baseTransform.position.x += config_.neighborPush;
        }
    }

    if (selected) {
        baseTransform.position.y -= config_.selectedLift;
        baseTransform.rotationDegrees = 0.f;
        baseTransform.scale = {config_.selectedScale, config_.selectedScale};
        baseTransform.zIndex = 2000;
        return baseTransform;
    }

    if (hovered) {
        baseTransform.position.y -= config_.hoverLift;
        baseTransform.rotationDegrees = 0.f;
        baseTransform.scale = {config_.hoverScale, config_.hoverScale};
        baseTransform.zIndex = 1000;
        return baseTransform;
    }

    return baseTransform;
}
