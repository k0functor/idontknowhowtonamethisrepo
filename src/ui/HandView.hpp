#pragma once

#include "cards/CardInstanceId.hpp"
#include "ui/CardView.hpp"
#include "ui/HandLayout.hpp"

#include <raylib.h>

#include <optional>
#include <vector>

class HandView {
public:
    void setCards(const std::vector<CardViewModel>& models);
    void setSelectedCard(std::optional<CardInstanceId> selectedCardId);
    void setDraggedCard(std::optional<CardInstanceId> draggedCardId, Vector2 dragPosition);
    void setViewport(float width, float height);

    void update(float deltaSeconds, Vector2 mousePosition);
    void render(const Font* font) const;

    std::optional<CardInstanceId> hoveredCardId() const;
    std::optional<Vector2> cardCenter(CardInstanceId cardId) const;

private:
    void rebuildIfNeeded(const std::vector<CardViewModel>& models);
    void updateTargets();
    std::optional<std::size_t> findHoveredIndex(Vector2 mousePosition) const;

private:
    HandLayout layout_;
    std::vector<CardView> cards_;

    std::optional<CardInstanceId> selectedCardId_;
    std::optional<CardInstanceId> hoveredCardId_;
    std::optional<CardInstanceId> draggedCardId_;
    Vector2 dragPosition_{0.f, 0.f};
};
