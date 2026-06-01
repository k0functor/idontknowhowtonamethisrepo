#pragma once

#include "ui/CardTransform.hpp"
#include "ui/CardViewModel.hpp"

#include <raylib.h>

class CardVisualInstance {
public:
    CardVisualInstance() = default;

    void setModel(CardViewModel model);
    const CardViewModel& model() const;

    void setCurrentTransform(CardTransform transform);
    void setTargetTransform(CardTransform transform);

    const CardTransform& currentTransform() const;
    const CardTransform& targetTransform() const;

    void update(float deltaSeconds);
    void render(const Font* font) const;

    static CardTransform transformForBounds(Rectangle bounds, int zIndex = 0, float fill = 0.94f);
    static CardTransform transformForStandardSlot(Rectangle bounds, int zIndex = 0);
    static float standardScale();
    static Vector2 standardDisplaySize();
    static void renderStatic(const CardViewModel& model, const Font* font, CardTransform transform);
    static void renderStaticWithOverlay(const CardViewModel& model, const Font* font, CardTransform transform, Color overlayColor);

    bool contains(Vector2 worldPosition) const;
    bool containsAtTransform(Vector2 worldPosition, const CardTransform& transform) const;

    Vector2 center() const;
    int zIndex() const;

    static Vector2 size();

private:
    Vector2 localToWorld(Vector2 localPosition) const;
    Vector2 worldToLocal(Vector2 worldPosition) const;
    static Vector2 worldToLocalUsingTransform(Vector2 worldPosition, const CardTransform& transform);

    static float approach(float current, float target, float alpha);

private:
    CardViewModel model_;
    CardTransform currentTransform_;
    CardTransform targetTransform_;
};
