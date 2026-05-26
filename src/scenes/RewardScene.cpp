#include "RewardScene.hpp"

#include "cards/CardDefinition.hpp"
#include "localization/TextFormatter.hpp"
#include "ui/BasicUi.hpp"

#include <raylib.h>

#include <algorithm>
#include <cstddef>
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

    if (cardChoiceOpen_) {
        updateCardChoice(mouse);
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (std::size_t i = 0; i < reward_.options.size(); ++i) {
            if (BasicUi::contains(rewardOptionBounds(i), mouse)) {
                takeOption(i);
                return;
            }
        }

        if (BasicUi::contains(continueButtonBounds(), mouse)) {
            onContinue_(selection_);
            return;
        }
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

    const Rectangle panel{GetScreenWidth() * 0.5f - 310.f, 150.f, 620.f, 430.f};
    DrawRectangleRounded(panel, 0.06f, 12, Color{31, 34, 44, 255});
    DrawRectangleRoundedLinesEx(panel, 0.06f, 12, 2.f, Color{120, 130, 160, 255});

    if (reward_.options.empty()) {
        BasicUi::drawCenteredText(
            font_,
            localization_.get(TextId("reward.no_rewards_remaining")),
            Rectangle{panel.x + 30.f, panel.y + 120.f, panel.width - 60.f, 80.f},
            24.f,
            Color{205, 210, 225, 255}
        );
    } else {
        for (std::size_t i = 0; i < reward_.options.size(); ++i) {
            const RewardOption& option = reward_.options[i];
            const Rectangle row = rewardOptionBounds(i);
            const bool hovered = BasicUi::contains(row, mouse);

            DrawRectangleRounded(row, 0.08f, 10, hovered ? Color{53, 58, 75, 255} : Color{40, 43, 56, 255});
            DrawRectangleRoundedLinesEx(row, 0.08f, 10, 2.f, hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255});

            BasicUi::drawText(font_, optionTitle(option), Vector2{row.x + 22.f, row.y + 18.f}, 23.f, Color{245, 245, 250, 255});
        }
    }

    BasicUi::drawButton(font_, continueButtonBounds(), localization_.get(TextId("reward.continue")), mouse);

    if (cardChoiceOpen_) {
        renderCardChoice();
    }
}

Rectangle RewardScene::rewardOptionBounds(const std::size_t index) const {
    const Rectangle panel{GetScreenWidth() * 0.5f - 310.f, 150.f, 620.f, 430.f};
    return Rectangle{panel.x + 35.f, panel.y + 92.f + static_cast<float>(index) * 76.f, panel.width - 70.f, 58.f};
}

Rectangle RewardScene::continueButtonBounds() const {
    return Rectangle{GetScreenWidth() * 0.5f - 170.f, 610.f, 340.f, 52.f};
}

Rectangle RewardScene::cardChoiceModalBounds() const {
    const float width = std::min(1040.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = std::min(560.f, static_cast<float>(GetScreenHeight()) - 72.f);
    return Rectangle{GetScreenWidth() * 0.5f - width * 0.5f, GetScreenHeight() * 0.5f - height * 0.5f, width, height};
}

Rectangle RewardScene::cardOptionBounds(const std::size_t index) const {
    const Rectangle panel = cardChoiceModalBounds();
    const RewardOption* option = activeOption();
    const std::size_t count = option != nullptr ? std::min<std::size_t>(3, option->cardOptions.size()) : 0;
    const float spacing = 24.f;
    const float width = count > 0 ? std::min(285.f, (panel.width - 80.f - spacing * static_cast<float>(count - 1)) / static_cast<float>(count)) : 285.f;
    const float height = 260.f;
    const float total = width * static_cast<float>(count) + spacing * static_cast<float>(count > 0 ? count - 1 : 0);
    const float startX = panel.x + panel.width * 0.5f - total * 0.5f;
    return Rectangle{startX + static_cast<float>(index) * (width + spacing), panel.y + 90.f, width, height};
}

Rectangle RewardScene::cancelButtonBounds() const {
    const Rectangle panel = cardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 250.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

Rectangle RewardScene::confirmButtonBounds() const {
    const Rectangle panel = cardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f + 30.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

void RewardScene::takeOption(const std::size_t index) {
    if (index >= reward_.options.size()) {
        return;
    }

    RewardOption& option = reward_.options[index];
    switch (option.type) {
        case RewardOptionType::Gold:
            selection_.goldTaken += option.gold;
            reward_.options.erase(reward_.options.begin() + static_cast<std::ptrdiff_t>(index));
            break;
        case RewardOptionType::CardChoice:
            activeOptionIndex_ = index;
            selectedCardIndex_.reset();
            cardChoiceOpen_ = true;
            break;
        case RewardOptionType::Consumable:
            selection_.selectedConsumableIds.push_back(option.consumableId);
            reward_.options.erase(reward_.options.begin() + static_cast<std::ptrdiff_t>(index));
            break;
    }
}

void RewardScene::updateCardChoice(const Vector2 mouse) {
    RewardOption* option = activeOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice) {
        cardChoiceOpen_ = false;
        activeOptionIndex_.reset();
        selectedCardIndex_.reset();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < option->cardOptions.size(); ++i) {
        if (BasicUi::contains(cardOptionBounds(i), mouse)) {
            selectedCardIndex_ = i;
            return;
        }
    }

    if (BasicUi::contains(cancelButtonBounds(), mouse)) {
        cardChoiceOpen_ = false;
        activeOptionIndex_.reset();
        selectedCardIndex_.reset();
        return;
    }

    if (selectedCardIndex_.has_value() && BasicUi::contains(confirmButtonBounds(), mouse)) {
        selection_.selectedCardIds.push_back(option->cardOptions[*selectedCardIndex_].cardId);
        reward_.options.erase(reward_.options.begin() + static_cast<std::ptrdiff_t>(*activeOptionIndex_));
        cardChoiceOpen_ = false;
        activeOptionIndex_.reset();
        selectedCardIndex_.reset();
    }
}

void RewardScene::renderCardChoice() const {
    const RewardOption* option = activeOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = cardChoiceModalBounds();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 120});
    DrawRectangleRounded(panel, 0.045f, 14, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(font_, localization_.get(TextId("reward.card_choice_title")), Rectangle{panel.x + 24.f, panel.y + 18.f, panel.width - 48.f, 38.f}, 30.f, Color{255, 235, 175, 255});

    for (std::size_t i = 0; i < option->cardOptions.size(); ++i) {
        const Rectangle bounds = cardOptionBounds(i);
        const bool hovered = BasicUi::contains(bounds, mouse);
        const bool selected = selectedCardIndex_.has_value() && *selectedCardIndex_ == i;
        DrawRectangleRounded(bounds, 0.08f, 10, hovered ? Color{53, 58, 75, 255} : Color{40, 43, 56, 255});
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, selected ? 4.f : 2.f, selected ? Color{255, 218, 90, 255} : Color{110, 120, 150, 255});
        const CardId& cardId = option->cardOptions[i].cardId;
        BasicUi::drawCenteredText(font_, cardName(cardId), Rectangle{bounds.x + 10.f, bounds.y + 14.f, bounds.width - 20.f, 38.f}, 22.f, Color{245, 245, 250, 255});
        const std::vector<std::string> lines = BasicUi::wrapText(font_, cardDescription(cardId), 15.f, bounds.width - 28.f);
        float y = bounds.y + 70.f;
        for (const std::string& line : lines) {
            if (y > bounds.y + bounds.height - 24.f) break;
            BasicUi::drawText(font_, line, Vector2{bounds.x + 16.f, y}, 15.f, Color{205, 210, 225, 255});
            y += 20.f;
        }
    }

    BasicUi::drawButton(font_, cancelButtonBounds(), localization_.get(TextId("reward.cancel")), mouse);
    BasicUi::drawButton(font_, confirmButtonBounds(), localization_.get(TextId("reward.confirm")), mouse, selectedCardIndex_.has_value());
}

const RewardOption* RewardScene::activeOption() const {
    if (!activeOptionIndex_.has_value() || *activeOptionIndex_ >= reward_.options.size()) {
        return nullptr;
    }
    return &reward_.options[*activeOptionIndex_];
}

RewardOption* RewardScene::activeOption() {
    if (!activeOptionIndex_.has_value() || *activeOptionIndex_ >= reward_.options.size()) {
        return nullptr;
    }
    return &reward_.options[*activeOptionIndex_];
}

std::string RewardScene::optionTitle(const RewardOption& option) const {
    switch (option.type) {
        case RewardOptionType::Gold:
            return localization_.format(TextId("reward.take_gold"), {{"amount", std::to_string(option.gold)}});
        case RewardOptionType::CardChoice:
            return localization_.get(TextId("reward.take_card"));
        case RewardOptionType::Consumable:
            return localization_.get(TextId("reward.take_consumable"));
    }
    return {};
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
    const std::string range = rangeToString(value.minimumPossibleValue(), value.maximumPossibleValue());
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

void RewardScene::fillVariablesFromEffect(TextFormatter::Variables& variables, const EffectDefinition& effect) {
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
