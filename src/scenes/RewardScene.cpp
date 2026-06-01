#include "RewardScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "consumables/ConsumableId.hpp"
#include "inspect/InspectPanelModel.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/BasicUi.hpp"
#include "ui/CardViewModelFactory.hpp"
#include "ui/CardVisualInstance.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <raylib.h>

RewardScene::RewardScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    RewardState reward,
    std::function<void(RewardSelection)> onContinue
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      relics_(relics),
      consumables_(consumables),
      reward_(std::move(reward)),
      onContinue_(std::move(onContinue)) {}

void RewardScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (cardChoiceOpen_) {
        updateCardChoice(mouse);
        return;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
        onContinue_(selection_);
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
        rewardTitle(),
        Rectangle{0.f, 65.f, static_cast<float>(VirtualViewport::width()), 70.f},
        42.f,
        Color{240, 240, 250, 255}
    );

    const Rectangle panel{VirtualViewport::width() * 0.5f - 310.f, 150.f, 620.f, 430.f};
    DrawRectangleRounded(panel, 0.06f, 12, Color{31, 34, 44, 255});
    DrawRectangleRoundedLinesEx(panel, 0.06f, 12, 2.f, Color{120, 130, 160, 255});

    BasicUi::drawText(
        font_,
        rewardHint(),
        Vector2{panel.x + 30.f, panel.y + 54.f},
        20.f,
        Color{190, 196, 215, 255}
    );

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

            BasicUi::drawTextFitted(font_, optionTitle(option), Vector2{row.x + 22.f, row.y + 10.f}, row.width - 44.f, 25.f, 18.f, Color{245, 245, 250, 255});

            const std::string description = optionDescription(option);
            if (!description.empty()) {
                BasicUi::drawText(
                    font_,
                    description,
                    Vector2{row.x + 22.f, row.y + 42.f},
                    18.f,
                    Color{190, 198, 220, 255}
                );
            }
        }

        for (std::size_t i = 0; i < reward_.options.size(); ++i) {
            const RewardOption& option = reward_.options[i];
            const Rectangle row = rewardOptionBounds(i);
            if (option.type == RewardOptionType::Relic && BasicUi::contains(row, mouse)) {
                renderRelicInspect(option, row);
                break;
            }
        }
    }

    BasicUi::drawButton(font_, continueButtonBounds(), localization_.get(TextId("reward.continue")), mouse);

    if (cardChoiceOpen_) {
        renderCardChoice();
    }
}

Rectangle RewardScene::rewardOptionBounds(const std::size_t index) const {
    const Rectangle panel{VirtualViewport::width() * 0.5f - 310.f, 150.f, 620.f, 430.f};
    return Rectangle{panel.x + 35.f, panel.y + 92.f + static_cast<float>(index) * 76.f, panel.width - 70.f, 58.f};
}

Rectangle RewardScene::continueButtonBounds() const {
    return Rectangle{VirtualViewport::width() * 0.5f - 170.f, 610.f, 340.f, 52.f};
}

Rectangle RewardScene::cardChoiceModalBounds() const {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = std::min(560.f, static_cast<float>(VirtualViewport::height()) - 72.f);
    return Rectangle{VirtualViewport::width() * 0.5f - width * 0.5f, VirtualViewport::height() * 0.5f - height * 0.5f, width, height};
}

Rectangle RewardScene::cardOptionBounds(const std::size_t index) const {
    const Rectangle panel = cardChoiceModalBounds();
    const RewardOption* option = activeOption();
    const std::size_t count = option != nullptr ? std::min<std::size_t>(3, option->cardOptions.size()) : 0;
    const float spacing = 34.f;
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    const float width = cardSize.x + 18.f;
    const float height = cardSize.y + 26.f;
    const float total = width * static_cast<float>(count) + spacing * static_cast<float>(count > 0 ? count - 1 : 0);
    const float startX = panel.x + panel.width * 0.5f - total * 0.5f;
    return Rectangle{startX + static_cast<float>(index) * (width + spacing), panel.y + 88.f, width, height};
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
        case RewardOptionType::Relic:
            if (!option.relicId.empty()) {
                selection_.selectedRelicIds.push_back(option.relicId);
            }
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
    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 120});
    DrawRectangleRounded(panel, 0.045f, 14, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(font_, localization_.get(TextId("reward.card_choice_title")), Rectangle{panel.x + 24.f, panel.y + 18.f, panel.width - 48.f, 38.f}, 30.f, Color{255, 235, 175, 255});

    for (std::size_t i = 0; i < option->cardOptions.size(); ++i) {
        const Rectangle bounds = cardOptionBounds(i);
        const bool selected = selectedCardIndex_.has_value() && *selectedCardIndex_ == i;
        const CardId& cardId = option->cardOptions[i].cardId;
        if (!cards_.contains(cardId)) {
            BasicUi::drawCenteredText(font_, cardId.value, bounds, 18.f, Color{245, 245, 250, 255});
            continue;
        }

        CardViewModel model = CardViewModelFactory::buildStatic(
            cards_.get(cardId),
            localization_,
            CardInstanceId{static_cast<std::uint64_t>(i + 1)},
            false,
            selected
        );
        const CardTransform transform = CardVisualInstance::transformForStandardSlot(bounds, static_cast<int>(i));
        CardVisualInstance::renderStatic(model, font_.available() ? &font_.font() : nullptr, transform);
    }

    BasicUi::drawButton(font_, cancelButtonBounds(), localization_.get(TextId("reward.cancel")), mouse);
    BasicUi::drawButton(font_, confirmButtonBounds(), localization_.get(TextId("reward.confirm")), mouse, selectedCardIndex_.has_value());
}

void RewardScene::renderRelicInspect(const RewardOption& option, const Rectangle row) const {
    if (option.type != RewardOptionType::Relic || option.relicId.empty()) {
        return;
    }

    InspectPanelModel panel;
    panel.header = relicName(option.relicId);
    panel.subheader = relicDescription(option.relicId);
    panel.entries.push_back(InspectEntry{
        localization_.get(TextId("inspect.relic.reward.name")),
        localization_.get(TextId("inspect.relic.reward.description"))
    });

    constexpr float gap = 14.f;
    constexpr float screenMargin = 18.f;
    constexpr float preferredWidth = 460.f;
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    Rectangle bounds{
        row.x + row.width + gap,
        row.y,
        std::min(preferredWidth, screenWidth - screenMargin * 2.f),
        std::min(380.f, screenHeight - screenMargin * 2.f)
    };

    if (bounds.x + bounds.width > screenWidth - screenMargin) {
        bounds.x = std::max(screenMargin, row.x - gap - bounds.width);
    }

    bounds.y = std::clamp(bounds.y, screenMargin, screenHeight - bounds.height - screenMargin);
    inspectPanelView_.render(font_, panel, bounds);
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
            return localization_.format(
                TextId("reward.take_consumable_named"),
                {{"consumable", consumableName(option.consumableId)}}
            );
        case RewardOptionType::Relic:
            return localization_.format(TextId("reward.take_relic"), {{"relic", relicName(option.relicId)}});
    }
    return {};
}

std::string RewardScene::optionDescription(const RewardOption& option) const {
    switch (option.type) {
        case RewardOptionType::Gold:
            return localization_.get(TextId("reward.gold_description"));
        case RewardOptionType::CardChoice:
            return localization_.format(
                TextId("reward.card_choice_description"),
                {{"count", std::to_string(option.cardOptions.size())}}
            );
        case RewardOptionType::Consumable:
            return consumableDescription(option.consumableId);
        case RewardOptionType::Relic:
            return {};
    }

    return {};
}

std::string RewardScene::cardName(const CardId& cardId) const {
    return localization_.get(cards_.get(cardId).nameTextId);
}

std::string RewardScene::cardDescription(const CardId& cardId) const {
    const CardDescriptionFormatter descriptionFormatter(localization_);
    return descriptionFormatter.formatStaticDescription(cards_.get(cardId));
}

std::string RewardScene::relicName(const std::string& relicId) const {
    if (relicId.empty() || !relics_.contains(RelicId(relicId))) {
        return relicId;
    }

    return localization_.get(relics_.get(RelicId(relicId)).nameTextId);
}

std::string RewardScene::relicDescription(const std::string& relicId) const {
    if (relicId.empty() || !relics_.contains(RelicId(relicId))) {
        return relicId;
    }

    return localization_.get(relics_.get(RelicId(relicId)).descriptionTextId);
}

std::string RewardScene::consumableName(const std::string& consumableId) const {
    if (consumableId.empty() || !consumables_.contains(ConsumableId(consumableId))) {
        return consumableId;
    }

    return localization_.get(consumables_.get(ConsumableId(consumableId)).nameTextId);
}

std::string RewardScene::consumableDescription(const std::string& consumableId) const {
    if (consumableId.empty() || !consumables_.contains(ConsumableId(consumableId))) {
        return consumableId;
    }

    return localization_.get(consumables_.get(ConsumableId(consumableId)).descriptionTextId);
}

std::string RewardScene::rewardTitle() const {
    if (reward_.sourceNodeType == RunMapNodeType::Chest) {
        return localization_.get(TextId("reward.chest_title"));
    }

    if (reward_.sourceNodeType == RunMapNodeType::Boss) {
        return localization_.get(TextId("reward.boss_title"));
    }

    return localization_.get(TextId("reward.title"));
}

std::string RewardScene::rewardHint() const {
    if (reward_.sourceNodeType == RunMapNodeType::Chest) {
        return localization_.get(TextId("reward.chest_optional_hint"));
    }

    if (reward_.sourceNodeType == RunMapNodeType::Boss) {
        return localization_.get(TextId("reward.boss_optional_hint"));
    }

    return localization_.get(TextId("reward.optional_hint"));
}
