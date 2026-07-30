#include "CombatScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "cards/CardDescriptionFormatter.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/BasicUi.hpp"
#include "ui/CardViewModelFactory.hpp"
#include "ui/CardVisualInstance.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace {
void drawModalBackdrop() {
    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 155});
}
}

void CombatScene::openRewardModalIfNeeded() {
    if (reward_.has_value() || rewardAccepted_) {
        return;
    }

    reward_ = createRewardOnVictory_(finalResult_);
    rewardSelection_ = RewardSelection{};
    activeRewardOptionIndex_.reset();
    selectedRewardCardIndex_.reset();
    rewardCardChoiceOpen_ = false;
}

void CombatScene::updateRewardModalInput(const Vector2 mousePosition) {
    if (!reward_.has_value() || rewardAccepted_) {
        return;
    }

    if (rewardCardChoiceOpen_) {
        updateRewardCardChoiceInput(mousePosition);
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        rewardAccepted_ = true;
        onRewardAccepted_(*reward_, rewardSelection_);
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (std::size_t i = 0; i < reward_->options.size(); ++i) {
            if (BasicUi::contains(rewardOptionRowBounds(i), mousePosition)) {
                takeRewardOption(i);
                return;
            }
        }

        if (BasicUi::contains(rewardContinueButtonBounds(), mousePosition)) {
            rewardAccepted_ = true;
            onRewardAccepted_(*reward_, rewardSelection_);
            return;
        }
    }
}


void CombatScene::renderDefeatModal() const {
    drawModalBackdrop();

    const float width = std::min(560.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = 260.f;
    const Rectangle panel{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };

    DrawRectangleRounded(panel, 0.045f, 14, Color{29, 31, 42, 248});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{180, 70, 75, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("combat.defeat.title"), "Defeat"),
        Rectangle{panel.x + 24.f, panel.y + 28.f, panel.width - 48.f, 44.f},
        34.f,
        Color{255, 210, 210, 255}
    );

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("combat.defeat.description"), "Your run has ended."),
        Rectangle{panel.x + 42.f, panel.y + 96.f, panel.width - 84.f, 56.f},
        22.f,
        Color{215, 220, 235, 255}
    );

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("combat.defeat.continue_hint"), "Press Enter, Space, or click to continue."),
        Rectangle{panel.x + 42.f, panel.y + 176.f, panel.width - 84.f, 42.f},
        18.f,
        Color{170, 178, 205, 255}
    );
}

void CombatScene::renderRewardModal() const {
    drawModalBackdrop();

    if (!reward_.has_value()) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = rewardModalBounds();

    DrawRectangleRounded(panel, 0.045f, 14, Color{29, 31, 42, 248});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("reward.title"), "Rewards"),
        Rectangle{panel.x + 24.f, panel.y + 18.f, panel.width - 48.f, 42.f},
        32.f,
        Color{255, 235, 175, 255}
    );

    BasicUi::drawText(
        uiFont_,
        localizedOrFallback(TextId("reward.optional_hint"), "Take what you want, then continue."),
        Vector2{panel.x + 38.f, panel.y + 68.f},
        18.f,
        Color{190, 196, 215, 255}
    );

    if (reward_->options.empty()) {
        BasicUi::drawCenteredText(
            uiFont_,
            localizedOrFallback(TextId("reward.no_rewards_remaining"), "No rewards remaining."),
            Rectangle{panel.x + 40.f, panel.y + 136.f, panel.width - 80.f, 56.f},
            24.f,
            Color{205, 210, 225, 255}
        );
    } else {
        for (std::size_t i = 0; i < reward_->options.size(); ++i) {
            const RewardOption& option = reward_->options[i];
            const Rectangle row = rewardOptionRowBounds(i);
            const bool hovered = BasicUi::contains(row, mouse);

            const Color fill = hovered ? Color{55, 59, 78, 255} : Color{41, 44, 58, 255};
            const Color border = hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255};

            DrawRectangleRounded(row, 0.08f, 10, fill);
            DrawRectangleRoundedLinesEx(row, 0.08f, 10, 2.5f, border);

            BasicUi::drawText(
                uiFont_,
                rewardOptionTitle(option),
                Vector2{row.x + 22.f, row.y + 14.f},
                23.f,
                Color{245, 245, 250, 255}
            );

            const std::string description = rewardOptionDescription(option);
            if (!description.empty()) {
                BasicUi::drawText(
                    uiFont_,
                    description,
                    Vector2{row.x + 22.f, row.y + 47.f},
                    16.f,
                    Color{190, 198, 220, 255}
                );
            }
        }

        for (std::size_t i = 0; i < reward_->options.size(); ++i) {
            const RewardOption& option = reward_->options[i];
            const Rectangle row = rewardOptionRowBounds(i);
            if (option.type == RewardOptionType::Relic && BasicUi::contains(row, mouse)) {
                renderRewardRelicInspect(option, row);
                break;
            }
        }
    }

    BasicUi::drawButton(
        uiFont_,
        rewardContinueButtonBounds(),
        localizedOrFallback(TextId("reward.continue"), "Continue"),
        mouse
    );

    if (rewardCardChoiceOpen_) {
        renderRewardCardChoiceModal();
    }
}

void CombatScene::renderRewardRelicInspect(const RewardOption& option, const Rectangle row) const {
    if (option.type != RewardOptionType::Relic || option.relicId.empty()) {
        return;
    }

    InspectPanelModel panel;
    panel.header = rewardRelicName(option.relicId);
    panel.subheader = rewardRelicDescription(option.relicId);
    panel.entries.push_back(InspectEntry{
        localizedOrFallback(TextId("inspect.relic.reward.name"), "Relic reward"),
        localizedOrFallback(TextId("inspect.relic.reward.description"), "Click the reward row to take this relic, or continue to skip it.")
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
    inspectPanelView_.render(uiFont_, panel, bounds);
}

void CombatScene::renderRewardCardChoiceModal() const {
    const RewardOption* option = activeRewardOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = rewardCardChoiceModalBounds();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 95});
    DrawRectangleRounded(panel, 0.045f, 14, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("reward.card_choice_title"), "Choose one card"),
        Rectangle{panel.x + 24.f, panel.y + 18.f, panel.width - 48.f, 38.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    for (std::size_t i = 0; i < option->cardOptions.size(); ++i) {
        const Rectangle bounds = rewardCardChoiceOptionBounds(i);
        const bool selected = selectedRewardCardIndex_.has_value() && *selectedRewardCardIndex_ == i;
        const CardId& cardId = option->cardOptions[i].cardId;
        if (!content_.cards().contains(cardId)) {
            BasicUi::drawCenteredText(uiFont_, cardId.value, bounds, 18.f, Color{245, 245, 250, 255});
            continue;
        }

        CardViewModel model = CardViewModelFactory::buildStatic(
            content_.cards().get(cardId),
            localization_,
            CardInstanceId{static_cast<std::uint64_t>(i + 1)},
            false,
            selected
        );
        const CardTransform transform = CardVisualInstance::transformForStandardSlot(bounds, static_cast<int>(i));
        CardVisualInstance::renderStatic(model, uiFont_.available() ? &uiFont_.font() : nullptr, transform);
    }

    BasicUi::drawButton(
        uiFont_,
        rewardCardChoiceCancelBounds(),
        localizedOrFallback(TextId("reward.cancel"), "Cancel"),
        mouse
    );

    BasicUi::drawButton(
        uiFont_,
        rewardCardChoiceConfirmBounds(),
        localizedOrFallback(TextId("reward.confirm"), "Confirm"),
        mouse,
        selectedRewardCardIndex_.has_value()
    );
}

void CombatScene::updateRewardCardChoiceInput(const Vector2 mousePosition) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        closeRewardCardChoice();
        return;
    }

    RewardOption* option = activeRewardOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice) {
        closeRewardCardChoice();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < option->cardOptions.size(); ++i) {
        if (BasicUi::contains(rewardCardChoiceOptionBounds(i), mousePosition)) {
            selectedRewardCardIndex_ = i;
            return;
        }
    }

    if (BasicUi::contains(rewardCardChoiceCancelBounds(), mousePosition)) {
        closeRewardCardChoice();
        return;
    }

    if (selectedRewardCardIndex_.has_value() &&
        BasicUi::contains(rewardCardChoiceConfirmBounds(), mousePosition)) {
        confirmRewardCardChoice();
        return;
    }
}

Rectangle CombatScene::rewardModalBounds() const {
    const float width = std::min(620.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = std::min(460.f, static_cast<float>(VirtualViewport::height()) - 72.f);

    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::rewardOptionRowBounds(const std::size_t index) const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{
        panel.x + 38.f,
        panel.y + 112.f + static_cast<float>(index) * 78.f,
        panel.width - 76.f,
        62.f
    };
}

Rectangle CombatScene::rewardContinueButtonBounds() const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 160.f, panel.y + panel.height - 68.f, 320.f, 50.f};
}

Rectangle CombatScene::rewardCardChoiceModalBounds() const {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = std::min(560.f, static_cast<float>(VirtualViewport::height()) - 72.f);

    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::rewardCardChoiceOptionBounds(const std::size_t index) const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    const RewardOption* option = activeRewardOption();
    const std::size_t optionCount = option != nullptr
        ? std::min<std::size_t>(3, option->cardOptions.size())
        : 0;

    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    const float cardWidth = cardSize.x + 18.f;
    const float cardHeight = cardSize.y + 26.f;
    if (optionCount == 0) {
        return Rectangle{panel.x + 40.f, panel.y + 100.f, cardWidth, cardHeight};
    }

    const float spacing = 34.f;
    const float totalWidth = cardWidth * static_cast<float>(optionCount) + spacing * static_cast<float>(optionCount - 1);
    const float startX = panel.x + panel.width * 0.5f - totalWidth * 0.5f;

    return Rectangle{
        startX + static_cast<float>(index) * (cardWidth + spacing),
        panel.y + 90.f,
        cardWidth,
        cardHeight
    };
}

Rectangle CombatScene::rewardCardChoiceCancelBounds() const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 250.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

Rectangle CombatScene::rewardCardChoiceConfirmBounds() const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f + 30.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

void CombatScene::openRewardCardChoice(const std::size_t optionIndex) {
    activeRewardOptionIndex_ = optionIndex;
    selectedRewardCardIndex_.reset();
    rewardCardChoiceOpen_ = true;
}

void CombatScene::closeRewardCardChoice() {
    activeRewardOptionIndex_.reset();
    selectedRewardCardIndex_.reset();
    rewardCardChoiceOpen_ = false;
}

void CombatScene::confirmRewardCardChoice() {
    RewardOption* option = activeRewardOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice || !selectedRewardCardIndex_.has_value()) {
        return;
    }

    const std::size_t selectedIndex = *selectedRewardCardIndex_;
    if (selectedIndex >= option->cardOptions.size()) {
        return;
    }

    rewardSelection_.selectedCardIds.push_back(option->cardOptions[selectedIndex].cardId);

    if (reward_.has_value() && activeRewardOptionIndex_.has_value() && *activeRewardOptionIndex_ < reward_->options.size()) {
        reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(*activeRewardOptionIndex_));
    }

    closeRewardCardChoice();
}

void CombatScene::takeRewardOption(const std::size_t optionIndex) {
    if (!reward_.has_value() || optionIndex >= reward_->options.size()) {
        return;
    }

    RewardOption& option = reward_->options[optionIndex];

    switch (option.type) {
        case RewardOptionType::Gold:
            if (option.gold > 0) {
                rewardSelection_.goldTaken += option.gold;
            }
            reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(optionIndex));
            break;

        case RewardOptionType::CardChoice:
            openRewardCardChoice(optionIndex);
            break;

        case RewardOptionType::Consumable:
            if (!option.consumableId.empty()) {
                rewardSelection_.selectedConsumableIds.push_back(option.consumableId);
            }
            reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(optionIndex));
            break;

        case RewardOptionType::Relic:
            if (!option.relicId.empty()) {
                rewardSelection_.selectedRelicIds.push_back(option.relicId);
            }
            reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(optionIndex));
            break;

        case RewardOptionType::ActiveItem:
            if (!option.activeItemId.empty()) {
                rewardSelection_.selectedActiveItemId = option.activeItemId;
            }
            reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(optionIndex));
            break;
    }
}

const RewardOption* CombatScene::activeRewardOption() const {
    if (!reward_.has_value() || !activeRewardOptionIndex_.has_value()) {
        return nullptr;
    }

    if (*activeRewardOptionIndex_ >= reward_->options.size()) {
        return nullptr;
    }

    return &reward_->options[*activeRewardOptionIndex_];
}

RewardOption* CombatScene::activeRewardOption() {
    if (!reward_.has_value() || !activeRewardOptionIndex_.has_value()) {
        return nullptr;
    }

    if (*activeRewardOptionIndex_ >= reward_->options.size()) {
        return nullptr;
    }

    return &reward_->options[*activeRewardOptionIndex_];
}

std::string CombatScene::rewardOptionTitle(const RewardOption& option) const {
    switch (option.type) {
        case RewardOptionType::Gold:
            return localization_.format(
                TextId("reward.take_gold"),
                {{"amount", std::to_string(option.gold)}}
            );

        case RewardOptionType::CardChoice:
            return localizedOrFallback(TextId("reward.take_card"), "Take a card");

        case RewardOptionType::Consumable:
            return localizedOrFallback(TextId("reward.take_consumable"), "Take consumable");

        case RewardOptionType::Relic:
            return localization_.format(
                TextId("reward.take_relic"),
                {{"relic", rewardRelicName(option.relicId)}}
            );

        case RewardOptionType::ActiveItem:
            if (content_.activeItems().contains(ActiveItemId(option.activeItemId))) {
                return localization_.format(
                    TextId("reward.take_active_item"),
                    {{"item", localization_.get(content_.activeItems().get(ActiveItemId(option.activeItemId)).nameTextId)}}
                );
            }
            return option.activeItemId;
    }

    return {};
}

std::string CombatScene::rewardOptionDescription(const RewardOption& option) const {
    switch (option.type) {
        case RewardOptionType::Gold:
            return localizedOrFallback(TextId("reward.gold_description"), "Add this gold to your run.");

        case RewardOptionType::CardChoice:
            return localization_.format(
                TextId("reward.card_choice_description"),
                {{"count", std::to_string(option.cardOptions.size())}}
            );

        case RewardOptionType::Consumable:
            return option.consumableId;

        case RewardOptionType::Relic:
            return {};

        case RewardOptionType::ActiveItem:
            if (content_.activeItems().contains(ActiveItemId(option.activeItemId))) {
                return localization_.get(content_.activeItems().get(ActiveItemId(option.activeItemId)).descriptionTextId);
            }
            return {};
    }

    return {};
}

std::string CombatScene::rewardCardName(const CardId& cardId) const {
    return localization_.get(content_.cards().get(cardId).nameTextId);
}

std::string CombatScene::rewardRelicName(const std::string& relicId) const {
    if (relicId.empty() || !content_.relics().contains(RelicId(relicId))) {
        return relicId;
    }

    return localization_.get(content_.relics().get(RelicId(relicId)).nameTextId);
}

std::string CombatScene::rewardRelicDescription(const std::string& relicId) const {
    if (relicId.empty() || !content_.relics().contains(RelicId(relicId))) {
        return relicId;
    }

    return localization_.get(content_.relics().get(RelicId(relicId)).descriptionTextId);
}

std::string CombatScene::rewardCardDescription(const CardId& cardId) const {
    const CardDescriptionFormatter descriptionFormatter(localization_);
    return descriptionFormatter.formatStaticDescription(content_.cards().get(cardId));
}

std::string CombatScene::localizedOrFallback(const TextId& textId, const std::string&) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return textId.value;
}
