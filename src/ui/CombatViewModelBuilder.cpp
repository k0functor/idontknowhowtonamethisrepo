#include "CombatViewModelBuilder.hpp"

#include <algorithm>

CombatViewModelBuilder::CombatViewModelBuilder(
    const LocalizationManager& localization,
    const CardViewModelBuilder& cardViewModelBuilder
)
    : localization_(localization),
      cardViewModelBuilder_(cardViewModelBuilder) {}

CombatViewModel CombatViewModelBuilder::build(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> previewTarget
) const {
    CombatViewModel model;
    model.energy = state.resources.energy();
    model.maxEnergy = state.resources.maxEnergy();
    model.drawPileSize = static_cast<int>(state.deck.drawPile.size());
    model.discardPileSize = static_cast<int>(state.deck.discardPile.size());
    model.exhaustPileSize = static_cast<int>(state.deck.exhaustPile.size());

    model.handCards.reserve(state.hand.cards().size());
    for (const CardInstance& card : state.hand.cards()) {
        model.handCards.push_back(
            cardViewModelBuilder_.build(
                state,
                card.instanceId,
                source,
                previewTarget
            )
        );
    }

    model.enemies.reserve(state.enemies.size());
    for (const CombatEntity& enemy : state.enemies) {
        EnemyViewModel enemyModel;
        enemyModel.entityId = enemy.id;
        enemyModel.name = localization_.get(enemy.nameTextId);
        enemyModel.currentHp = enemy.health.current();
        enemyModel.maxHp = enemy.health.maximum();
        enemyModel.block = enemy.block;
        enemyModel.statuses = enemy.statuses.all();
        enemyModel.alive = enemy.isAlive();
        model.enemies.push_back(std::move(enemyModel));
    }

    model.recentLogEntries = recentLogEntries(state, 5);
    return model;
}

std::vector<std::string> CombatViewModelBuilder::recentLogEntries(
    const CombatState& state,
    const std::size_t maxCount
) {
    const std::vector<std::string>& entries = state.log.entries();

    if (entries.size() <= maxCount) {
        return entries;
    }

    return std::vector<std::string>(entries.end() - static_cast<std::ptrdiff_t>(maxCount), entries.end());
}
