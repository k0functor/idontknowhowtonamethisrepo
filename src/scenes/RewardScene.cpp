#include "RewardScene.hpp"

#include "cards/CardDefinition.hpp"
#include "dice/DiceExpression.hpp"
#include "localization/TextFormatter.hpp"
#include "ui/BasicUi.hpp"

#include <raylib.h>

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

RewardScene::RewardScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const CardDatabase& cards,
    RewardState reward,
    std::function<void(RewardSelection)> onContinue
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      reward_(std::move(reward)),
      onContinue_(std::move(onContinue)) {}

void RewardScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    for (std::size_t i = 0; i < reward_.cardOptions.size(); ++i) {
        if (BasicUi::contains(cardOptionBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedCardIndex_ = i;
            skipCardReward_ = false;
            return;
        }
    }

    if (!reward_.cardOptions.empty() && BasicUi::contains(skipCardBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        selectedCardIndex_.reset();
        skipCardReward_ = true;
        return;
    }

    if (BasicUi::contains(continueButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        RewardSelection selection;
        selection.takeGold = true;
        selection.skippedCardReward = skipCardReward_;

        if (selectedCardIndex_.has_value()) {
            selection.selectedCardId = reward_.cardOptions[*selectedCardIndex_].cardId;
        }

        onContinue_(selection);
    }
}

void RewardScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("reward.title")),
        Rectangle{0.f, 65.f, static_cast<float>(GetScreenWidth()), 70.f},
        42.f,
        Color{240, 240, 250, 255}
    );

    const Rectangle panel{GetScreenWidth() * 0.5f - 520.f, 150.f, 1040.f, 465.f};
    DrawRectangleRounded(panel, 0.06f, 12, Color{31, 34, 44, 255});
    DrawRectangleRoundedLinesEx(panel, 0.06f, 12, 2.f, Color{120, 130, 160, 255});

    BasicUi::drawText(
        font_,
        localization_.format(TextId("reward.gold"), {{"amount", std::to_string(reward_.gold)}}),
        Vector2{panel.x + 35.f, panel.y + 32.f},
        26.f,
        Color{238, 226, 150, 255}
    );

    if (reward_.cardOptions.empty()) {
        BasicUi::drawText(
            font_,
            localization_.get(TextId("reward.no_card_options")),
            Vector2{panel.x + 35.f, panel.y + 90.f},
            22.f,
            Color{205, 210, 225, 255}
        );
    } else {
        BasicUi::drawText(
            font_,
            localization_.get(TextId("reward.choose_card")),
            Vector2{panel.x + 35.f, panel.y + 90.f},
            24.f,
            Color{225, 228, 240, 255}
        );

        for (std::size_t i = 0; i < reward_.cardOptions.size(); ++i) {
            const Rectangle bounds = cardOptionBounds(i);
            const bool hovered = BasicUi::contains(bounds, mouse);
            const bool selected = selectedCardIndex_.has_value() && *selectedCardIndex_ == i;

            const Color fill = hovered ? Color{53, 58, 75, 255} : Color{40, 43, 56, 255};
            const Color border = selected ? Color{255, 218, 90, 255} : Color{110, 120, 150, 255};

            DrawRectangleRounded(bounds, 0.08f, 10, fill);
            DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, selected ? 4.f : 2.f, border);

            const CardId& cardId = reward_.cardOptions[i].cardId;
            BasicUi::drawCenteredText(
                font_,
                cardName(cardId),
                Rectangle{bounds.x + 10.f, bounds.y + 18.f, bounds.width - 20.f, 42.f},
                24.f,
                Color{245, 245, 250, 255}
            );

            const CardDefinition& definition = cards_.get(cardId);
            const std::string meta = std::to_string(definition.energyCost) + " energy / " + toString(definition.rarity);
            BasicUi::drawCenteredText(
                font_,
                meta,
                Rectangle{bounds.x + 10.f, bounds.y + 58.f, bounds.width - 20.f, 24.f},
                14.f,
                Color{178, 184, 205, 255}
            );

            const std::vector<std::string> lines = BasicUi::wrapText(
                font_,
                cardDescription(cardId),
                16.f,
                bounds.width - 28.f
            );

            float y = bounds.y + 98.f;
            for (const std::string& line : lines) {
                if (y > bounds.y + bounds.height - 30.f) {
                    break;
                }
                BasicUi::drawText(font_, line, Vector2{bounds.x + 16.f, y}, 16.f, Color{205, 210, 225, 255});
                y += 21.f;
            }
        }

        BasicUi::drawButton(
            font_,
            skipCardBounds(),
            localization_.get(TextId("reward.skip_card")),
            mouse,
            true,
            BasicUi::ButtonStyle{
                Color{42, 43, 50, 255},
                Color{60, 62, 72, 255},
                Color{35, 36, 42, 255},
                skipCardReward_ ? Color{255, 218, 90, 255} : Color{120, 130, 160, 255},
                Color{235, 235, 242, 255},
                Color{120, 124, 140, 255}
            }
        );
    }

    BasicUi::drawButton(
        font_,
        continueButtonBounds(),
        localization_.get(TextId("reward.continue")),
        mouse
    );
}

Rectangle RewardScene::cardOptionBounds(const std::size_t index) const {
    const float cardWidth = 285.f;
    const float cardHeight = 230.f;
    const float spacing = 28.f;
    const float optionCount = static_cast<float>(std::min<std::size_t>(3, reward_.cardOptions.size()));
    const float totalWidth = cardWidth * optionCount + spacing * std::max(0.f, optionCount - 1.f);
    const float startX = GetScreenWidth() * 0.5f - totalWidth * 0.5f;

    return Rectangle{
        startX + static_cast<float>(index) * (cardWidth + spacing),
        300.f,
        cardWidth,
        cardHeight
    };
}

Rectangle RewardScene::skipCardBounds() const {
    return Rectangle{GetScreenWidth() * 0.5f - 170.f, 545.f, 340.f, 46.f};
}

Rectangle RewardScene::continueButtonBounds() const {
    return Rectangle{GetScreenWidth() * 0.5f - 190.f, 650.f, 380.f, 54.f};
}

std::string RewardScene::cardName(const CardId& cardId) const {
    return localization_.get(cards_.get(cardId).nameTextId);
}

std::string RewardScene::cardDescription(const CardId& cardId) const {
    const CardDefinition& card = cards_.get(cardId);

    TextFormatter::Variables variables;
    variables.emplace("damage", "?");
    variables.emplace("hp_damage", "?");
    variables.emplace("block", "?");
    variables.emplace("poison", "?");
    variables.emplace("value", "?");

    for (const EffectDefinition& effect : card.effects) {
        fillVariablesFromEffect(variables, effect);
    }

    return localization_.format(card.descriptionTextId, variables);
}

std::string RewardScene::effectValueText(const EffectValue& value) {
    const std::string range = rangeToString(
        value.minimumPossibleValue(),
        value.maximumPossibleValue()
    );

    if (value.isDice()) {
        return range + " (" + ::toString(value.diceExpression()) + ")";
    }

    return range;
}

std::string RewardScene::rangeToString(const int minimum, const int maximum) {
    if (minimum == maximum) {
        return std::to_string(minimum);
    }

    return std::to_string(minimum) + "-" + std::to_string(maximum);
}

void RewardScene::fillVariablesFromEffect(
    TextFormatter::Variables& variables,
    const EffectDefinition& effect
) {
    const std::string value = effectValueText(effect.value);

    switch (effect.type) {
        case EffectType::Damage:
            variables["damage"] = value;
            variables["hp_damage"] = value;
            break;

        case EffectType::Block:
            variables["block"] = value;
            break;

        case EffectType::ApplyStatus:
            variables["value"] = value;
            if (effect.statusId.has_value()) {
                variables[*effect.statusId] = value;
            }
            break;

        default:
            variables["value"] = value;
            break;
    }
}
