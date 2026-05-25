#pragma once

#include <raylib.h>

struct CardTransform {
    Vector2 position{0.f, 0.f};
    Vector2 scale{1.f, 1.f};
    float rotationDegrees = 0.f;
    int zIndex = 0;
};
