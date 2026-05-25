#pragma once

#include "ui/CardTransform.hpp"

#include <cstddef>
#include <vector>

class HandLayout {
public:
    struct Config {
        float centerX = 640.f;
        float centerY = 625.f;
        float cardSpacing = 105.f;
        float maxTotalWidth = 900.f;
        float arcHeight = 36.f;
        float maxRotationDegrees = 10.f;
        float hoverLift = 95.f;
        float hoverScale = 1.12f;
        float selectedLift = 115.f;
        float selectedScale = 1.15f;
        float neighborPush = 42.f;
    };

    HandLayout() = default;
    explicit HandLayout(Config config);

    void setConfig(Config config);
    const Config& config() const;

    std::vector<CardTransform> calculateBaseTransforms(std::size_t cardCount) const;

    CardTransform transformForState(
        CardTransform baseTransform,
        std::size_t currentIndex,
        std::size_t cardCount,
        bool hovered,
        bool selected,
        bool anyCardElevated,
        std::size_t elevatedIndex
    ) const;

private:
    Config config_;
};
