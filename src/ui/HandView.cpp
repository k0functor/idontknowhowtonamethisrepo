#include "HandView.hpp"

#include <algorithm>
#include <numeric>

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

void HandView::update(const float deltaSeconds, const Vector2 mousePosition) {
    const std::optional<std::size_t> hoveredIndex = findHoveredIndex(mousePosition);
    hoveredCardId_.reset();

    if (hoveredIndex.has_value()) {
        hoveredCardId_ = cards_[*hoveredIndex].model().instanceId;
    }

    updateTargets();

    for (CardView& card : cards_) {
        card.update(deltaSeconds);
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

    cards_.clear();
    cards_.resize(models.size());

    const std::vector<CardTransform> baseTransforms = layout_.calculateBaseTransforms(models.size());

    for (std::size_t i = 0; i < models.size(); ++i) {
        cards_[i].setModel(models[i]);
        cards_[i].setCurrentTransform(baseTransforms[i]);
        cards_[i].setTargetTransform(baseTransforms[i]);
    }
}

void HandView::updateTargets() {
    const std::vector<CardTransform> baseTransforms = layout_.calculateBaseTransforms(cards_.size());

    std::optional<std::size_t> elevatedIndex;

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        const CardInstanceId id = cards_[i].model().instanceId;

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

        CardTransform target = layout_.transformForState(
            baseTransforms[i],
            i,
            cards_.size(),
            hovered,
            selected,
            elevatedIndex.has_value(),
            elevatedIndex.value_or(0)
        );

        cards_[i].setTargetTransform(target);
    }
}

std::optional<std::size_t> HandView::findHoveredIndex(const Vector2 mousePosition) const {
    std::vector<std::size_t> order(cards_.size());
    std::iota(order.begin(), order.end(), 0);

    std::sort(order.begin(), order.end(), [this](const std::size_t lhs, const std::size_t rhs) {
        return cards_[lhs].zIndex() > cards_[rhs].zIndex();
    });

    for (const std::size_t index : order) {
        if (cards_[index].contains(mousePosition)) {
            return index;
        }
    }

    return std::nullopt;
}
