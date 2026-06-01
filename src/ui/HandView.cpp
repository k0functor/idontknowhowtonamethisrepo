#include "HandView.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>

namespace {
float clampFloat(const float value, const float minimum, const float maximum) {
    return std::max(minimum, std::min(maximum, value));
}
}

void HandView::setCards(const std::vector<CardViewModel>& models) {
    rebuildIfNeeded(models);

    for (std::size_t i = 0; i < models.size(); ++i) {
        CardViewModel model = models[i];
        model.selected = selectedCardId_.has_value() && model.instanceId == *selectedCardId_;
        cards_[i].setModel(std::move(model));
    }

    updateTargets();
}

void HandView::setSelectedCard(const std::optional<CardInstanceId> selectedCardId) {
    selectedCardId_ = selectedCardId;
    updateTargets();
}

void HandView::setDraggedCard(
    const std::optional<CardInstanceId> draggedCardId,
    const Vector2 dragPosition
) {
    draggedCardId_ = draggedCardId;
    dragPosition_ = dragPosition;
    updateTargets();
}

void HandView::setViewport(const float width, const float height) {
    drawPileOrigin_ = Vector2{82.f, height - 41.f};
    const Vector2 cardSize = CardView::size();
    const float widthScale = width / 1280.f;
    const float heightScale = height / 720.f;
    const float baseScale = clampFloat(std::min(widthScale, heightScale), 0.70f, 1.10f);

    HandLayout::Config config;
    config.baseScale = baseScale;
    config.centerX = width * 0.5f;
    config.arcHeight = 36.f;
    config.cardSpacing = 108.f * baseScale;
    config.maxTotalWidth = std::max(260.f, std::min(width - 360.f, 1040.f * baseScale));

    const float bottomMargin = 20.f;
    config.centerY = height
        - cardSize.y * baseScale * 0.5f
        - bottomMargin
        - config.arcHeight * baseScale;
    config.centerY = std::max(320.f * baseScale, config.centerY);

    config.maxRotationDegrees = 10.f;
    config.hoverLift = 95.f;
    config.hoverScale = 1.12f;
    config.selectedLift = 115.f;
    config.selectedScale = 1.15f;
    config.draggedScale = 1.16f;
    config.neighborPush = 42.f;

    layout_.setConfig(config);
    updateTargets();
}

void HandView::setDrawPileOrigin(const Vector2 origin) {
    drawPileOrigin_ = origin;
}

void HandView::update(const float deltaSeconds, const Vector2 mousePosition) {
    const std::optional<std::size_t> hoveredIndex = draggedCardId_.has_value()
        ? std::nullopt
        : findHoveredIndex(mousePosition);
    hoveredCardId_.reset();

    if (hoveredIndex.has_value()) {
        hoveredCardId_ = cards_[*hoveredIndex].model().instanceId;
    }

    updateTargets();

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        if (i < entryDelaySeconds_.size() && entryDelaySeconds_[i] > 0.f) {
            entryDelaySeconds_[i] = std::max(0.f, entryDelaySeconds_[i] - deltaSeconds);
            continue;
        }

        cards_[i].update(deltaSeconds);
    }
}

void HandView::render(const Font* font) const {
    std::vector<std::size_t> order(cards_.size());
    std::iota(order.begin(), order.end(), 0);

    std::sort(order.begin(), order.end(), [this](const std::size_t lhs, const std::size_t rhs) {
        return cards_[lhs].zIndex() < cards_[rhs].zIndex();
    });

    for (const std::size_t index : order) {
        cards_[index].render(font);
    }
}

std::optional<CardInstanceId> HandView::hoveredCardId() const {
    return hoveredCardId_;
}

std::optional<Vector2> HandView::cardCenter(const CardInstanceId cardId) const {
    for (const CardView& card : cards_) {
        if (card.model().instanceId == cardId) {
            return card.center();
        }
    }

    return std::nullopt;
}

void HandView::rebuildIfNeeded(const std::vector<CardViewModel>& models) {
    bool rebuild = cards_.size() != models.size();

    if (!rebuild) {
        for (std::size_t i = 0; i < models.size(); ++i) {
            if (cards_[i].model().instanceId != models[i].instanceId) {
                rebuild = true;
                break;
            }
        }
    }

    if (!rebuild) {
        return;
    }

    struct PreviousCardState {
        CardInstanceId id;
        CardTransform transform;
        float entryDelaySeconds = 0.f;
    };

    std::vector<PreviousCardState> previousStates;
    previousStates.reserve(cards_.size());
    for (std::size_t i = 0; i < cards_.size(); ++i) {
        PreviousCardState state;
        state.id = cards_[i].model().instanceId;
        state.transform = cards_[i].currentTransform();
        if (i < entryDelaySeconds_.size()) {
            state.entryDelaySeconds = entryDelaySeconds_[i];
        }
        previousStates.push_back(state);
    }

    cards_.clear();
    cards_.resize(models.size());
    entryDelaySeconds_.assign(models.size(), 0.f);

    const std::vector<CardTransform> baseTransforms = layout_.calculateBaseTransforms(models.size());

    std::size_t newCardSequence = 0u;
    for (std::size_t i = 0; i < models.size(); ++i) {
        cards_[i].setModel(models[i]);

        const auto previous = std::find_if(
            previousStates.begin(),
            previousStates.end(),
            [&](const PreviousCardState& entry) {
                return entry.id == models[i].instanceId;
            }
        );

        CardTransform current = baseTransforms[i];
        if (previous != previousStates.end()) {
            current = previous->transform;
            entryDelaySeconds_[i] = previous->entryDelaySeconds;
        } else {
            const float sourceScale = std::max(0.20f, layout_.config().baseScale * 0.42f);
            current.position = drawPileOrigin_;
            current.scale = Vector2{sourceScale, sourceScale};
            current.rotationDegrees = -12.f;
            current.zIndex = baseTransforms[i].zIndex + 500;
            entryDelaySeconds_[i] = 0.24f * static_cast<float>(newCardSequence);
            ++newCardSequence;
        }

        cards_[i].setCurrentTransform(current);
        cards_[i].setTargetTransform(baseTransforms[i]);
    }
}

void HandView::updateTargets() {
    const std::vector<CardTransform> baseTransforms = layout_.calculateBaseTransforms(cards_.size());

    std::optional<std::size_t> elevatedIndex;

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        const CardInstanceId id = cards_[i].model().instanceId;

        if (draggedCardId_.has_value() && id == *draggedCardId_) {
            elevatedIndex = i;
            break;
        }

        if (selectedCardId_.has_value() && id == *selectedCardId_) {
            elevatedIndex = i;
            break;
        }

        if (hoveredCardId_.has_value() && id == *hoveredCardId_) {
            elevatedIndex = i;
        }
    }

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        const CardInstanceId id = cards_[i].model().instanceId;
        const bool hovered = hoveredCardId_.has_value() && id == *hoveredCardId_;
        const bool selected = selectedCardId_.has_value() && id == *selectedCardId_;
        const bool dragged = draggedCardId_.has_value() && id == *draggedCardId_;

        CardTransform target = layout_.transformForState(
            baseTransforms[i],
            i,
            cards_.size(),
            hovered,
            selected,
            dragged,
            dragPosition_,
            elevatedIndex.has_value(),
            elevatedIndex.value_or(0)
        );

        cards_[i].setTargetTransform(target);
    }
}

std::optional<std::size_t> HandView::findHoveredIndex(const Vector2 mousePosition) const {
    const std::vector<CardTransform> baseTransforms = layout_.calculateBaseTransforms(cards_.size());

    std::vector<std::size_t> order(cards_.size());
    std::iota(order.begin(), order.end(), 0);

    std::sort(order.begin(), order.end(), [this](const std::size_t lhs, const std::size_t rhs) {
        return cards_[lhs].zIndex() > cards_[rhs].zIndex();
    });

    for (const std::size_t index : order) {
        const bool insideCurrentCard = cards_[index].contains(mousePosition);
        const bool insideBaseSlot = cards_[index].containsAtTransform(mousePosition, baseTransforms[index]);

        if (insideCurrentCard || insideBaseSlot) {
            return index;
        }
    }

    return std::nullopt;
}
