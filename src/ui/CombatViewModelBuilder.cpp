#include "CombatViewModelBuilder.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace {
std::string intentLabel(const EnemyIntent& intent) {
    switch (intent.type) {
        case EnemyIntentType::Attack:
            return intent.valueMin == intent.valueMax
                ? "Attack " + std::to_string(intent.valueMax)
                : "Attack " + std::to_string(intent.valueMin) + "-" + std::to_string(intent.valueMax);

        case EnemyIntentType::Block:
            return intent.valueMin == intent.valueMax
                ? "Block " + std::to_string(intent.valueMax)
                : "Block " + std::to_string(intent.valueMin) + "-" + std::to_string(intent.valueMax);

        case EnemyIntentType::Buff:
            return "Buff";

        case EnemyIntentType::Debuff:
            return "Debuff";

        case EnemyIntentType::Special:
            return "Special";

        case EnemyIntentType::Unknown:
            return "Unknown";
    }

    return "Unknown";
}
}

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
    model.phase = state.phase;
    model.turn = state.turn;
    model.energy = state.resources.energy();
    model.maxEnergy = state.resources.maxEnergy();
    model.drawPileSize = static_cast<int>(state.deck.drawPile.size());
    model.discardPileSize = static_cast<int>(state.deck.discardPile.size());
    model.exhaustPileSize = static_cast<int>(state.deck.exhaustPile.size());
    model.canEndTurn = state.phase == CombatPhase::PlayerTurn;

    model.players.reserve(state.players.size());
    for (const CombatEntity& player : state.players) {
        PlayerViewModel playerModel;
        playerModel.entityId = player.id;
        playerModel.name = localization_.get(player.nameTextId);
        playerModel.currentHp = player.health.current();
        playerModel.maxHp = player.health.maximum();
        playerModel.block = player.block;
        playerModel.statuses = player.statuses.all();
        playerModel.alive = player.isAlive();
        model.players.push_back(std::move(playerModel));
    }

    if (!state.players.empty()) {
        const CombatEntity& player = state.players.front();
        model.playerCurrentHp = player.health.current();
        model.playerMaxHp = player.health.maximum();
        model.playerBlock = player.block;
    }

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

    std::unordered_map<std::uint64_t, EnemyIntent> intentsByEnemy;
    intentsByEnemy.reserve(state.enemyIntents.size());
    for (const EnemyIntentState& intentState : state.enemyIntents) {
        intentsByEnemy.emplace(intentState.enemyId.value, intentState.intent);
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

        const auto intentIterator = intentsByEnemy.find(enemy.id.value);
        if (intentIterator != intentsByEnemy.end()) {
            enemyModel.intent = intentIterator->second;
            enemyModel.intentText = intentLabel(enemyModel.intent);
        } else {
            enemyModel.intent.type = EnemyIntentType::Unknown;
            enemyModel.intentText = enemyModel.alive ? "..." : "";
        }

        model.enemies.push_back(std::move(enemyModel));
    }

    model.recentLogEntries = recentLogEntries(state, 6);
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
