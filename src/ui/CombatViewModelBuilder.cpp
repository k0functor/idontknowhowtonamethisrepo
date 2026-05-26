#include "CombatViewModelBuilder.hpp"

#include "statuses/StatusType.hpp"

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
    const StatusDatabase& statusDatabase,
    const CardViewModelBuilder& cardViewModelBuilder
)
    : localization_(localization),
      statusDatabase_(statusDatabase),
      cardViewModelBuilder_(cardViewModelBuilder) {}

CombatViewModel CombatViewModelBuilder::build(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> previewTarget
) const {
    return build(
        state,
        source,
        previewTarget,
        [source](const CardInstance&) {
            return source;
        }
    );
}

CombatViewModel CombatViewModelBuilder::build(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> previewTarget,
    const std::function<EntityId(const CardInstance&)>& sourceForCard
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
        playerModel.statuses = buildStatuses(player.statuses);
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
                sourceForCard(card),
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
        enemyModel.statuses = buildStatuses(enemy.statuses);
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

std::vector<StatusViewModel> CombatViewModelBuilder::buildStatuses(
    const StatusContainer& statuses
) const {
    std::vector<StatusViewModel> result;

    const std::vector<std::pair<std::string, int>> entries = statuses.all();
    result.reserve(entries.size());

    for (const auto& [statusId, amount] : entries) {
        StatusViewModel model;
        model.id = statusId;
        model.amount = amount;

        if (statusDatabase_.contains(StatusId(statusId))) {
            const StatusDefinition& definition = statusDatabase_.get(StatusId(statusId));
            model.name = localization_.get(definition.nameTextId);
            model.debuff = definition.type == StatusType::Debuff;
        } else {
            model.name = statusId;
            model.debuff = false;
        }

        result.push_back(std::move(model));
    }

    return result;
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
