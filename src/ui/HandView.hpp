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

    void update(float deltaSeconds, Vector2 mousePosition);
    void render(const Font* font) const;

    std::optional<CardInstanceId> hoveredCardId() const;

private:
    void rebuildIfNeeded(const std::vector<CardViewModel>& models);
    void updateTargets();
    std::optional<std::size_t> findHoveredIndex(Vector2 mousePosition) const;

private:
    HandLayout layout_;
    std::vector<CardView> cards_;

    std::optional<CardInstanceId> selectedCardId_;
    std::optional<CardInstanceId> hoveredCardId_;
};
