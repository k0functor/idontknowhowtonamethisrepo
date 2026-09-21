#include "CombatView.hpp"
#include "ui/VirtualViewport.hpp"

#include "ui/UiTheme.hpp"
#include "ui/Utf8.hpp"

#include <algorithm>
#include <string>

namespace {
constexpr float minContentWidth = 960.f;
constexpr float maxContentWidth = 1520.f;

UiTheme::Tone journalUiTone(const CombatJournalTone tone) {
    switch (tone) {
        case CombatJournalTone::Damage:
            return UiTheme::Tone::Danger;
        case CombatJournalTone::Defense:
            return UiTheme::Tone::Positive;
        case CombatJournalTone::Status:
            return UiTheme::Tone::Info;
        case CombatJournalTone::Stress:
            return UiTheme::Tone::Warning;
        case CombatJournalTone::Resource:
            return UiTheme::Tone::Accent;
        case CombatJournalTone::Important:
            return UiTheme::Tone::Primary;
        case CombatJournalTone::Neutral:
            return UiTheme::Tone::Neutral;
    }
    return UiTheme::Tone::Neutral;
}

}

void CombatView::setModel(const CombatViewModel& model) {
    model_ = model;

    playerViews_.resize(model_.players.size());
    for (std::size_t i = 0; i < model_.players.size(); ++i) {
        playerViews_[i].setModel(model_.players[i]);
    }

    enemyViews_.resize(model_.enemies.size());
    for (std::size_t i = 0; i < model_.enemies.size(); ++i) {
        enemyViews_[i].setModel(model_.enemies[i]);
    }

    applyResponsiveLayout();
    handView_.setCards(model_.handCards);
}

const CombatViewModel& CombatView::model() const {
    return model_;
}

void CombatView::setSelectedCard(const std::optional<CardInstanceId> selectedCardId) {
    handView_.setSelectedCard(selectedCardId);
}

void CombatView::setDraggedCard(
    const std::optional<CardInstanceId> draggedCardId,
    const Vector2 dragPosition
) {
    draggedCardId_ = draggedCardId;
    dragPosition_ = dragPosition;
    handView_.setDraggedCard(draggedCardId, dragPosition);
}

void CombatView::update(const float deltaSeconds, const Vector2 mousePosition) {
    applyResponsiveLayout();

    handView_.setDraggedCard(draggedCardId_, dragPosition_);
    handView_.update(deltaSeconds, mousePosition);

    hoveredEndTurnButton_ = endTurnButtonContains(mousePosition);

    hoveredRelicIndex_.reset();
    if (model_.showTopRelics) {
        for (std::size_t i = 0; i < model_.relics.size(); ++i) {
            if (CheckCollisionPointRec(mousePosition, relicBounds(i))) {
                hoveredRelicIndex_ = i;
                break;
            }
        }
    }

    hoveredConsumableIndex_.reset();
    for (std::size_t i = 0; i < model_.consumables.size(); ++i) {
        if (CheckCollisionPointRec(mousePosition, consumableBounds(i))) {
            hoveredConsumableIndex_ = i;
            break;
        }
    }

    hoveredDroneSlotIndex_.reset();
    for (std::size_t i = 0; i < model_.droneSlots.size(); ++i) {
        if (CheckCollisionPointRec(mousePosition, droneSlotBounds(i))) {
            hoveredDroneSlotIndex_ = i;
            break;
        }
    }

    hoveredStatus_.reset();
    hoveredStatusBounds_.reset();

    hoveredPlayerId_.reset();
    for (const PlayerView& playerView : playerViews_) {
        const std::optional<std::size_t> statusIndex = playerView.statusIndexAt(mousePosition);
        const std::optional<std::size_t> relicIndex = playerView.relicIndexAt(mousePosition);
        if (playerView.contains(mousePosition) || statusIndex.has_value() || relicIndex.has_value()) {
            hoveredPlayerId_ = playerView.model().entityId;

            if (statusIndex.has_value() && *statusIndex < playerView.model().statuses.size()) {
                hoveredStatus_ = playerView.model().statuses[*statusIndex];
                hoveredStatusBounds_ = playerView.statusBounds(*statusIndex);
            }

            if (relicIndex.has_value() && *relicIndex < playerView.model().relics.size()) {
                hoveredRelicIndex_ = globalRelicIndexFor(playerView.model().relics[*relicIndex]);
            }
            break;
        }
    }

    hoveredEnemyId_.reset();
    for (const EnemyView& enemyView : enemyViews_) {
        if (enemyView.model().opacity <= 0.05f) {
            continue;
        }

        const std::optional<std::size_t> statusIndex = enemyView.statusIndexAt(mousePosition);
        if (enemyView.contains(mousePosition) || statusIndex.has_value()) {
            hoveredEnemyId_ = enemyView.model().entityId;

            if (statusIndex.has_value() && *statusIndex < enemyView.model().statuses.size()) {
                hoveredStatus_ = enemyView.model().statuses[*statusIndex];
                hoveredStatusBounds_ = enemyView.statusBounds(*statusIndex);
            }
            break;
        }
    }
}

void CombatView::render(const Font* font) const {
    const int screenWidth = VirtualViewport::width();
    const int screenHeight = VirtualViewport::height();

    DrawRectangle(0, 0, screenWidth, screenHeight, UiTheme::canvas);
    DrawRectangle(0, 0, screenWidth, 72, UiTheme::topBar);

    const Rectangle battlefield = battlefieldBounds();
    const Rectangle handArea = handBounds();
    DrawRectangleLinesEx(battlefield, 1.f, UiTheme::borderSoft);
    DrawRectangleLinesEx(handArea, 1.f, UiTheme::borderSoft);

    const Rectangle endTurnBounds = endTurnButtonBounds();
    const Color endTurnColor = model_.canEndTurn
        ? (hoveredEndTurnButton_
            ? UiTheme::toneHoverFill(UiTheme::Tone::Accent)
            : UiTheme::toneFill(UiTheme::Tone::Accent))
        : UiTheme::toneFill(UiTheme::Tone::Disabled);
    const Color endTurnBorder = model_.canEndTurn
        ? UiTheme::toneBorder(UiTheme::Tone::Accent)
        : UiTheme::toneBorder(UiTheme::Tone::Disabled);
    DrawRectangleRounded(endTurnBounds, UiTheme::controlRoundness, 8, endTurnColor);
    DrawRectangleRoundedLinesEx(
        endTurnBounds,
        UiTheme::controlRoundness,
        8,
        UiTheme::borderThickness,
        endTurnBorder
    );

    if (font != nullptr) {
        DrawTextEx(*font, model_.endTurnLabel.c_str(), Vector2{endTurnBounds.x + 24.f, endTurnBounds.y + 18.f}, 18.f, 1.f, model_.canEndTurn ? UiTheme::toneText(UiTheme::Tone::Accent) : UiTheme::textMuted);

        if (!model_.keyboardHintLabel.empty()) {
            DrawTextEx(*font, model_.keyboardHintLabel.c_str(), Vector2{handArea.x + 14.f, handArea.y - 34.f}, 14.f, 1.f, UiTheme::textMuted);
        }

        if (!model_.targetHintLabel.empty()) {
            DrawTextEx(*font, model_.targetHintLabel.c_str(), Vector2{handArea.x + 14.f, handArea.y - 16.f}, 14.f, 1.f, UiTheme::toneText(UiTheme::Tone::Positive));
        }

        if (!model_.turnOrderLabel.empty()) {
            DrawTextEx(*font, model_.turnOrderLabel.c_str(), Vector2{battlefield.x + battlefield.width * 0.5f - 180.f, battlefield.y + 10.f}, 16.f, 1.f, UiTheme::toneText(UiTheme::Tone::Accent));
        }

        if (!model_.activeActorLabel.empty()) {
            DrawTextEx(*font, model_.activeActorLabel.c_str(), Vector2{battlefield.x + battlefield.width * 0.5f - 120.f, battlefield.y + 32.f}, 16.f, 1.f, UiTheme::toneText(UiTheme::Tone::Info));
        }

        if (model_.showTopRelics) {
            for (std::size_t i = 0; i < model_.relics.size(); ++i) {
                const RelicViewModel& relic = model_.relics[i];
                const Rectangle bounds = relicBounds(i);
                const bool hovered = hoveredRelicIndex_.has_value() && *hoveredRelicIndex_ == i;
                const Color fill = hovered ? UiTheme::toneHoverFill(UiTheme::Tone::Accent) : UiTheme::toneFill(UiTheme::Tone::Accent);
                const Color border = hovered ? UiTheme::toneText(UiTheme::Tone::Accent) : UiTheme::toneBorder(UiTheme::Tone::Accent);

                DrawRectangleRounded(bounds, 0.22f, 6, fill);
                DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, hovered ? 2.5f : 1.5f, border);
                const std::string relicText = relic.ownerName.empty()
                    ? relic.name
                    : relic.ownerName + ": " + relic.name;
                DrawTextEx(*font, UiUtf8::truncateWithEllipsis(relicText, 18).c_str(), Vector2{bounds.x + 8.f, bounds.y + 5.f}, 13.f, 1.f, UiTheme::toneText(UiTheme::Tone::Accent));
            }
        }

        for (std::size_t i = 0; i < model_.consumables.size(); ++i) {
            const ConsumableViewModel& consumable = model_.consumables[i];
            const Rectangle bounds = consumableBounds(i);
            const bool hovered = hoveredConsumableIndex_.has_value() && *hoveredConsumableIndex_ == i;
            const Color fill = !consumable.filled
                ? UiTheme::toneFill(UiTheme::Tone::Disabled)
                : (hovered ? UiTheme::toneHoverFill(UiTheme::Tone::Info) : UiTheme::toneFill(UiTheme::Tone::Info));
            const Color border = hovered ? UiTheme::toneText(UiTheme::Tone::Info) : UiTheme::toneBorder(UiTheme::Tone::Info);

            DrawRectangleRounded(bounds, 0.22f, 6, fill);
            DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, hovered ? 2.5f : 1.5f, border);

            const std::string text = consumable.filled ? UiUtf8::truncateWithEllipsis(consumable.name, 12) : model_.emptyLabel;
            DrawTextEx(*font, text.c_str(), Vector2{bounds.x + 8.f, bounds.y + 5.f}, 13.f, 1.f, UiTheme::toneText(UiTheme::Tone::Info));
        }

        if (!model_.recentJournalEntries.empty()) {
            const float journalX = battlefield.x + 10.f;
            const float journalY = battlefield.y + 8.f;
            const float journalWidth = std::clamp(battlefield.width * 0.31f, 300.f, 430.f);
            float journalHeight = 12.f;
            for (const CombatJournalEntryViewModel& entry : model_.recentJournalEntries) {
                journalHeight += entry.detail.empty() ? 24.f : 38.f;
            }
            journalHeight = std::min(journalHeight, battlefield.height - 16.f);

            const Rectangle journalBounds{journalX, journalY, journalWidth, journalHeight};
            DrawRectangleRounded(journalBounds, 0.06f, 8, UiTheme::withAlpha(UiTheme::panel, 0.88f));
            DrawRectangleRoundedLinesEx(journalBounds, 0.06f, 8, 1.f, UiTheme::borderSoft);

            float entryY = journalY + 8.f;
            for (const CombatJournalEntryViewModel& entry : model_.recentJournalEntries) {
                const UiTheme::Tone tone = journalUiTone(entry.tone);
                const float entryHeight = entry.detail.empty() ? 22.f : 36.f;
                if (entryY + entryHeight > journalY + journalHeight - 4.f) {
                    break;
                }

                DrawRectangle(
                    static_cast<int>(journalX + 5.f),
                    static_cast<int>(entryY + 2.f),
                    3,
                    static_cast<int>(entryHeight - 4.f),
                    UiTheme::toneBorder(tone)
                );
                const std::string title = UiUtf8::truncateWithEllipsis(entry.text, 52);
                DrawTextEx(
                    *font,
                    title.c_str(),
                    Vector2{journalX + 14.f, entryY + 1.f},
                    13.f,
                    1.f,
                    UiTheme::toneText(tone)
                );
                if (!entry.detail.empty()) {
                    const std::string detail = UiUtf8::truncateWithEllipsis(entry.detail, 66);
                    DrawTextEx(
                        *font,
                        detail.c_str(),
                        Vector2{journalX + 14.f, entryY + 17.f},
                        11.f,
                        1.f,
                        UiTheme::textMuted
                    );
                }
                entryY += entryHeight;
            }
        }
    }

    renderThreatSummary(font);

    for (const PlayerView& playerView : playerViews_) {
        const bool hovered = hoveredPlayerId_.has_value() && playerView.model().entityId == *hoveredPlayerId_;
        playerView.render(font, hovered);
    }

    for (const EnemyView& enemyView : enemyViews_) {
        const bool hovered = hoveredEnemyId_.has_value() && enemyView.model().entityId == *hoveredEnemyId_;
        enemyView.render(font, hovered);
    }

    renderDronePanel(font);
    handView_.render(font);
    renderCardTooltip(font);
}

std::optional<CardInstanceId> CombatView::hoveredCardId() const {
    return handView_.hoveredCardId();
}

std::optional<Vector2> CombatView::cardCenter(const CardInstanceId cardId) const {
    return handView_.cardCenter(cardId);
}

std::optional<EntityId> CombatView::hoveredEnemyId() const {
    return hoveredEnemyId_;
}

std::optional<Rectangle> CombatView::hoveredEnemyBounds() const {
    if (!hoveredEnemyId_.has_value()) {
        return std::nullopt;
    }

    return enemyBounds(*hoveredEnemyId_);
}

std::optional<Rectangle> CombatView::enemyBounds(const EntityId entityId) const {
    for (const EnemyView& enemyView : enemyViews_) {
        if (enemyView.model().entityId == entityId) {
            return enemyView.bounds();
        }
    }

    return std::nullopt;
}

std::optional<EntityId> CombatView::hoveredPlayerId() const {
    return hoveredPlayerId_;
}

std::optional<Rectangle> CombatView::hoveredPlayerBounds() const {
    if (!hoveredPlayerId_.has_value()) {
        return std::nullopt;
    }

    return playerBounds(*hoveredPlayerId_);
}

std::optional<Rectangle> CombatView::playerBounds(const EntityId entityId) const {
    for (const PlayerView& playerView : playerViews_) {
        if (playerView.model().entityId == entityId) {
            return playerView.bounds();
        }
    }

    return std::nullopt;
}

std::optional<StatusViewModel> CombatView::hoveredStatus() const {
    return hoveredStatus_;
}

std::optional<Rectangle> CombatView::hoveredStatusBounds() const {
    return hoveredStatusBounds_;
}

std::optional<std::size_t> CombatView::hoveredRelicIndex() const {
    return hoveredRelicIndex_;
}

std::optional<std::size_t> CombatView::hoveredConsumableIndex() const {
    return hoveredConsumableIndex_;
}

std::optional<std::size_t> CombatView::hoveredDroneSlotIndex() const {
    return hoveredDroneSlotIndex_;
}

std::optional<Rectangle> CombatView::hoveredDroneSlotBounds() const {
    if (!hoveredDroneSlotIndex_.has_value() || *hoveredDroneSlotIndex_ >= model_.droneSlots.size()) {
        return std::nullopt;
    }

    return droneSlotBounds(*hoveredDroneSlotIndex_);
}

bool CombatView::endTurnButtonContains(const Vector2 mousePosition) const {
    return model_.canEndTurn && CheckCollisionPointRec(mousePosition, endTurnButtonBounds());
}

void CombatView::applyResponsiveLayout() {
    handView_.setViewport(
        static_cast<float>(VirtualViewport::width()),
        static_cast<float>(VirtualViewport::height())
    );
    layoutPlayers();
    layoutEnemies();
}

void CombatView::layoutPlayers() {
    const Rectangle battlefield = battlefieldBounds();
    const float viewWidth = std::clamp(battlefield.width * (playerViews_.size() > 1 ? 0.16f : 0.20f), 160.f, 235.f);
    const float viewHeight = 180.f;
    const float spacingX = viewWidth + 24.f;

    const float totalWidth = playerViews_.empty()
        ? 0.f
        : viewWidth * static_cast<float>(playerViews_.size()) + 24.f * static_cast<float>(playerViews_.size() - 1);

    const float centerX = battlefield.x + battlefield.width * 0.27f;
    const float startX = centerX - totalWidth * 0.5f;
    const float y = battlefield.y + battlefield.height * 0.48f - viewHeight * 0.5f;

    for (std::size_t i = 0; i < playerViews_.size(); ++i) {
        playerViews_[i].setSize(Vector2{viewWidth, viewHeight});
        playerViews_[i].setPosition(Vector2{startX + spacingX * static_cast<float>(i), y});

        const bool hasSeveralPlayers = playerViews_.size() > 1;
        const bool isRightmostPlayer = i + 1 == playerViews_.size();
        playerViews_[i].setStatusesOnRight(!hasSeveralPlayers || isRightmostPlayer);
    }
}

void CombatView::layoutEnemies() {
    const Rectangle battlefield = battlefieldBounds();
    if (enemyViews_.empty()) {
        return;
    }

    const std::size_t enemyCount = enemyViews_.size();
    const float gap = enemyCount >= 3 ? 14.f : 22.f;
    const float regionFactor = enemyCount == 1 ? 0.54f : (enemyCount == 2 ? 0.48f : 0.42f);
    const float regionX = battlefield.x + battlefield.width * regionFactor;
    const float regionWidth = battlefield.x + battlefield.width - regionX - 18.f;
    const float count = static_cast<float>(enemyCount);
    const float availablePerEnemy = (regionWidth - gap * std::max(0.f, count - 1.f)) / count;

    const float minimumWidth = enemyCount >= 3 ? 150.f : 170.f;
    const float maximumWidth = enemyCount == 1 ? 240.f : (enemyCount == 2 ? 220.f : 195.f);
    const float viewWidth = std::clamp(availablePerEnemy, minimumWidth, maximumWidth);
    const float viewHeight = enemyCount >= 3 ? 162.f : 180.f;
    const float totalWidth = viewWidth * count + gap * std::max(0.f, count - 1.f);
    const float startX = regionX + std::max(0.f, (regionWidth - totalWidth) * 0.5f);

    const float minimumBodyY = battlefield.y + 64.f;
    const float centeredBodyY = battlefield.y + battlefield.height * 0.54f - viewHeight * 0.5f;
    const float baseY = std::max(minimumBodyY, centeredBodyY);
    const bool compactStatuses = enemyCount > 1;

    for (std::size_t i = 0; i < enemyCount; ++i) {
        float verticalOffset = 0.f;
        if (enemyCount == 3) {
            verticalOffset = i == 1 ? -14.f : 12.f;
        } else if (enemyCount == 2) {
            verticalOffset = i == 0 ? 6.f : -6.f;
        }

        enemyViews_[i].setSize(Vector2{viewWidth, viewHeight});
        enemyViews_[i].setPosition(Vector2{
            startX + (viewWidth + gap) * static_cast<float>(i),
            baseY + verticalOffset
        });
        enemyViews_[i].setCompactStatuses(compactStatuses);
        enemyViews_[i].setStatusesOnRight(i + 1 == enemyCount);
    }
}

void CombatView::renderThreatSummary(const Font* font) const {
    if (font == nullptr || model_.incomingDamageLabel.empty()) {
        return;
    }

    const Rectangle battlefield = battlefieldBounds();
    constexpr float height = 30.f;
    constexpr float gap = 8.f;
    const float partyWidth = model_.partyWideThreatLabel.empty() ? 0.f : 142.f;
    const float damageWidth = 174.f;
    const float totalWidth = damageWidth + (partyWidth > 0.f ? gap + partyWidth : 0.f);
    float x = battlefield.x + battlefield.width - totalWidth - 14.f;
    const float y = battlefield.y + 10.f;

    const UiTheme::Tone damageTone = model_.attackingEnemyCount > 0
        ? UiTheme::Tone::Danger
        : UiTheme::Tone::Positive;
    const Rectangle damageBounds{x, y, damageWidth, height};
    DrawRectangleRounded(damageBounds, UiTheme::chipRoundness, 8, UiTheme::toneFill(damageTone));
    DrawRectangleRoundedLinesEx(
        damageBounds,
        UiTheme::chipRoundness,
        8,
        1.5f,
        UiTheme::toneBorder(damageTone)
    );
    const std::string damageText = UiUtf8::truncateWithEllipsis(model_.incomingDamageLabel, 24);
    const Vector2 damageSize = MeasureTextEx(*font, damageText.c_str(), 13.f, 1.f);
    DrawTextEx(
        *font,
        damageText.c_str(),
        Vector2{damageBounds.x + (damageBounds.width - damageSize.x) * 0.5f, damageBounds.y + 7.f},
        13.f,
        1.f,
        UiTheme::toneText(damageTone)
    );

    if (partyWidth <= 0.f) {
        return;
    }

    x += damageWidth + gap;
    const Rectangle partyBounds{x, y, partyWidth, height};
    DrawRectangleRounded(partyBounds, UiTheme::chipRoundness, 8, UiTheme::toneFill(UiTheme::Tone::Warning));
    DrawRectangleRoundedLinesEx(
        partyBounds,
        UiTheme::chipRoundness,
        8,
        1.5f,
        UiTheme::toneBorder(UiTheme::Tone::Warning)
    );
    const std::string partyText = UiUtf8::truncateWithEllipsis(model_.partyWideThreatLabel, 20);
    const Vector2 partySize = MeasureTextEx(*font, partyText.c_str(), 12.f, 1.f);
    DrawTextEx(
        *font,
        partyText.c_str(),
        Vector2{partyBounds.x + (partyBounds.width - partySize.x) * 0.5f, partyBounds.y + 8.f},
        12.f,
        1.f,
        UiTheme::toneText(UiTheme::Tone::Warning)
    );
}

void CombatView::renderDronePanel(const Font* font) const {
    if (model_.droneSlots.empty() || font == nullptr) {
        return;
    }

    const float slotWidth = 118.f;
    const float gap = 10.f;
    const float totalWidth = slotWidth * static_cast<float>(model_.droneSlots.size()) + gap * static_cast<float>(model_.droneSlots.size() - 1);
    const Rectangle content = contentBounds();
    const float x = content.x + content.width * 0.5f - totalWidth * 0.5f;
    const float y = 82.f;

    DrawTextEx(*font, model_.droneSlotsLabel.c_str(), Vector2{x, y - 22.f}, 14.f, 1.f, UiTheme::toneText(UiTheme::Tone::Info));

    for (std::size_t i = 0; i < model_.droneSlots.size(); ++i) {
        const DroneSlotViewModel& slot = model_.droneSlots[i];
        const Rectangle bounds = droneSlotBounds(i);
        const bool hovered = hoveredDroneSlotIndex_.has_value() && *hoveredDroneSlotIndex_ == i;
        const Color fill = slot.filled
            ? (hovered ? UiTheme::toneHoverFill(UiTheme::Tone::Info) : UiTheme::toneFill(UiTheme::Tone::Info))
            : (hovered ? UiTheme::toneHoverFill(UiTheme::Tone::Neutral) : UiTheme::panelInset);
        const Color border = hovered
            ? UiTheme::toneText(UiTheme::Tone::Accent)
            : (slot.cardActivationAvailable ? UiTheme::toneBorder(UiTheme::Tone::Info) : UiTheme::border);
        const Color nameColor = UiTheme::toneText(UiTheme::Tone::Info);
        const Color stateColor = slot.cardActivationAvailable
            ? UiTheme::toneText(UiTheme::Tone::Positive)
            : UiTheme::textMuted;

        DrawRectangleRounded(bounds, 0.18f, 8, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.18f, 8, hovered ? 3.f : 2.f, border);
        DrawTextEx(*font, UiUtf8::truncateWithEllipsis(slot.name, 13).c_str(), Vector2{bounds.x + 8.f, bounds.y + 8.f}, 14.f, 1.f, nameColor);
        if (!slot.cardActivationLabel.empty()) {
            DrawTextEx(*font, UiUtf8::truncateWithEllipsis(slot.cardActivationLabel, 13).c_str(), Vector2{bounds.x + 8.f, bounds.y + 28.f}, 12.f, 1.f, stateColor);
        }
    }
}


void CombatView::renderCardTooltip(const Font* font) const {
    if (font == nullptr) {
        return;
    }

    const std::optional<CardInstanceId> hoveredCardId = handView_.hoveredCardId();
    if (!hoveredCardId.has_value()) {
        return;
    }

    const auto cardIterator = std::find_if(
        model_.handCards.begin(),
        model_.handCards.end(),
        [&hoveredCardId](const CardViewModel& card) {
            return card.instanceId == *hoveredCardId;
        }
    );

    if (cardIterator == model_.handCards.end()) {
        return;
    }

    std::vector<std::string> lines;
    if (!cardIterator->stressPreviewLabel.empty()) {
        lines.push_back(cardIterator->stressPreviewLabel);
    }
    if (!cardIterator->playable && !cardIterator->unplayableReason.empty()) {
        lines.push_back(cardIterator->unplayableReason);
    }
    if (lines.empty()) {
        return;
    }

    const std::optional<Vector2> center = handView_.cardCenter(cardIterator->instanceId);
    if (!center.has_value()) {
        return;
    }

    constexpr float fontSize = 15.f;
    constexpr float lineHeight = 20.f;
    constexpr float paddingX = 14.f;
    constexpr float paddingY = 10.f;
    float measuredWidth = 0.f;
    for (const std::string& line : lines) {
        measuredWidth = std::max(measuredWidth, MeasureTextEx(*font, line.c_str(), fontSize, 1.f).x);
    }
    const float width = std::clamp(measuredWidth + paddingX * 2.f, 220.f, 560.f);
    const float height = paddingY * 2.f + lineHeight * static_cast<float>(lines.size());

    Rectangle bounds{
        center->x - width * 0.5f,
        center->y - CardView::size().y * 0.65f - height - 10.f,
        width,
        height
    };

    const Rectangle content = contentBounds();
    bounds.x = std::clamp(bounds.x, content.x + 8.f, content.x + content.width - bounds.width - 8.f);
    bounds.y = std::max(80.f, bounds.y);

    UiTheme::Tone tone = UiTheme::Tone::Info;
    if (!cardIterator->playable) {
        tone = UiTheme::Tone::Danger;
    } else if (cardIterator->previewRequiresTarget) {
        tone = UiTheme::Tone::Warning;
    }

    DrawRectangleRounded(bounds, 0.12f, 8, UiTheme::toneFill(tone));
    DrawRectangleRoundedLinesEx(bounds, 0.12f, 8, 2.f, UiTheme::toneBorder(tone));
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const std::string visible = UiUtf8::truncateWithEllipsis(lines[index], 72);
        DrawTextEx(
            *font,
            visible.c_str(),
            Vector2{bounds.x + paddingX, bounds.y + paddingY + lineHeight * static_cast<float>(index)},
            fontSize,
            1.f,
            UiTheme::toneText(tone)
        );
    }
}

Rectangle CombatView::contentBounds() const {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());
    const float margin = 14.f;
    const float availableWidth = std::max(1.f, screenWidth - margin * 2.f);
    const float contentWidth = std::clamp(availableWidth, std::min(minContentWidth, availableWidth), maxContentWidth);
    return Rectangle{
        (screenWidth - contentWidth) * 0.5f,
        0.f,
        contentWidth,
        screenHeight
    };
}

Rectangle CombatView::battlefieldBounds() const {
    const Rectangle content = contentBounds();
    const float margin = 14.f;
    const float top = 82.f;
    const float handHeight = std::clamp(static_cast<float>(VirtualViewport::height()) * 0.36f, 250.f, 330.f);
    const float bottom = static_cast<float>(VirtualViewport::height()) - handHeight - 10.f;
    return Rectangle{
        content.x + margin,
        top,
        std::max(1.f, content.width - margin * 2.f),
        std::max(220.f, bottom - top)
    };
}

Rectangle CombatView::handBounds() const {
    const Rectangle content = contentBounds();
    const float margin = 14.f;
    const float handHeight = std::clamp(static_cast<float>(VirtualViewport::height()) * 0.36f, 250.f, 330.f);
    return Rectangle{
        content.x + margin,
        static_cast<float>(VirtualViewport::height()) - handHeight,
        std::max(1.f, content.width - margin * 2.f),
        handHeight - 10.f
    };
}

Rectangle CombatView::endTurnButtonBounds() const {
    const Rectangle hand = handBounds();
    const float width = 150.f;
    const float height = 58.f;
    return Rectangle{
        hand.x + hand.width - width - 24.f,
        hand.y - height - 16.f,
        width,
        height
    };
}

Rectangle CombatView::relicBounds(const std::size_t index) const {
    const Rectangle content = contentBounds();
    const float x = content.x + 24.f + static_cast<float>(index) * 174.f;
    return Rectangle{x, 44.f, 162.f, 24.f};
}

Rectangle CombatView::consumableBounds(const std::size_t index) const {
    const Rectangle content = contentBounds();
    const float width = 136.f;
    const float height = 24.f;
    const float x = content.x + content.width - 24.f - width - static_cast<float>(index) * 148.f;
    return Rectangle{x, 44.f, width, height};
}

Rectangle CombatView::droneSlotBounds(const std::size_t index) const {
    const Rectangle content = contentBounds();
    const float slotWidth = 118.f;
    const float slotHeight = 54.f;
    const float gap = 10.f;
    const float totalWidth = slotWidth * static_cast<float>(model_.droneSlots.size()) + gap * static_cast<float>(model_.droneSlots.size() - 1);
    const float x = content.x + content.width * 0.5f - totalWidth * 0.5f + static_cast<float>(index) * (slotWidth + gap);
    return Rectangle{x, 82.f, slotWidth, slotHeight};
}

std::optional<std::size_t> CombatView::globalRelicIndexFor(const RelicViewModel& relic) const {
    for (std::size_t i = 0; i < model_.relics.size(); ++i) {
        const RelicViewModel& candidate = model_.relics[i];
        if (candidate.id == relic.id && candidate.ownerActorDefinitionId == relic.ownerActorDefinitionId) {
            return i;
        }
    }

    return std::nullopt;
}
