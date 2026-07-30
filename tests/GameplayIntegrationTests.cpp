#include "cards/CardId.hpp"
#include "cards/CardUpgrade.hpp"
#include "cards/DrawSystem.hpp"
#include "active_items/ActiveItemDatabase.hpp"
#include "active_items/ActiveItemRerollSystem.hpp"
#include "active_items/ActiveItemSystem.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "combat/BlockSystem.hpp"
#include "combat/BossPhaseSystem.hpp"
#include "combat/CardPlaySystem.hpp"
#include "combat/CardPlayValidator.hpp"
#include "combat/CombatController.hpp"
#include "combat/CombatState.hpp"
#include "combat/DamageSystem.hpp"
#include "combat/EffectResolver.hpp"
#include "combat/EffectSystem.hpp"
#include "combat/EnemyMoveSelector.hpp"
#include "combat/EnemyTurnSystem.hpp"
#include "combat/EnergySystem.hpp"
#include "combat/ModifierSystem.hpp"
#include "combat/PlayerTurnSystem.hpp"
#include "combat/Targeting.hpp"
#include "combat/TurnSystem.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "data/EnemyDatabase.hpp"
#include "dice/DiceCorruption.hpp"
#include "drones/DroneDatabase.hpp"
#include "drones/DroneSystem.hpp"
#include "effects/EffectDefinition.hpp"
#include "entities/CombatEntity.hpp"
#include "game/GameEventBus.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicSystem.hpp"
#include "rewards/RewardTuning.hpp"
#include "statuses/StatusDatabase.hpp"
#include "statuses/StatusSystem.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

std::string LocalizationManager::get(const TextId& textId) const {
    return textId.value;
}

std::string LocalizationManager::format(
    const TextId& textId,
    const TextFormatter::Variables&
) const {
    return textId.value;
}

void StatusDatabase::add(StatusDefinition definition) {
    const std::string id = definition.id.value;
    if (id.empty() || statuses_.contains(id)) {
        throw std::runtime_error("invalid integration-test status id");
    }
    statuses_.emplace(id, std::move(definition));
}

bool StatusDatabase::contains(const StatusId& id) const {
    return statuses_.contains(id.value);
}

const StatusDefinition& StatusDatabase::get(const StatusId& id) const {
    const auto iterator = statuses_.find(id.value);
    if (iterator == statuses_.end()) {
        throw std::runtime_error("unknown integration-test status");
    }
    return iterator->second;
}

void CardDatabase::add(CardDefinition definition) {
    cards_.emplace(definition.id.value, std::move(definition));
}

bool CardDatabase::contains(const CardId& id) const {
    return cards_.contains(id.value);
}

const CardDefinition& CardDatabase::get(const CardId& id) const {
    const auto iterator = cards_.find(id.value);
    if (iterator == cards_.end()) {
        throw std::runtime_error("unknown integration-test card");
    }
    return iterator->second;
}

std::vector<const CardDefinition*> CardDatabase::all() const {
    std::vector<const CardDefinition*> result;
    result.reserve(cards_.size());
    for (const auto& [_, definition] : cards_) {
        result.push_back(&definition);
    }
    return result;
}

void EnemyDatabase::add(EnemyDefinition definition) {
    enemies_.emplace(definition.id.value, std::move(definition));
}

bool EnemyDatabase::contains(const EnemyId& id) const {
    return enemies_.contains(id.value);
}

const EnemyDefinition& EnemyDatabase::get(const EnemyId& id) const {
    const auto iterator = enemies_.find(id.value);
    if (iterator == enemies_.end()) {
        throw std::runtime_error("unknown integration-test enemy");
    }
    return iterator->second;
}

void RelicDatabase::add(RelicDefinition definition) {
    relics_.emplace(definition.id.value, std::move(definition));
}

bool RelicDatabase::contains(const RelicId& id) const {
    return relics_.contains(id.value);
}

const RelicDefinition& RelicDatabase::get(const RelicId& id) const {
    const auto iterator = relics_.find(id.value);
    if (iterator == relics_.end()) {
        throw std::runtime_error("unknown integration-test relic");
    }
    return iterator->second;
}

std::vector<const RelicDefinition*> RelicDatabase::all() const {
    std::vector<const RelicDefinition*> result;
    result.reserve(relics_.size());
    for (const auto& [_, definition] : relics_) {
        result.push_back(&definition);
    }
    return result;
}


void ConsumableDatabase::add(ConsumableDefinition definition) {
    consumables_.emplace(definition.id.value, std::move(definition));
}

std::vector<const ConsumableDefinition*> ConsumableDatabase::all() const {
    std::vector<const ConsumableDefinition*> result;
    result.reserve(consumables_.size());
    for (const auto& [_, definition] : consumables_) {
        result.push_back(&definition);
    }
    return result;
}

void ActiveItemDatabase::add(ActiveItemDefinition definition) {
    items_.emplace(definition.id.value, std::move(definition));
}

bool ActiveItemDatabase::contains(const ActiveItemId& id) const {
    return items_.contains(id.value);
}

const ActiveItemDefinition& ActiveItemDatabase::get(const ActiveItemId& id) const {
    const auto iterator = items_.find(id.value);
    if (iterator == items_.end()) {
        throw std::runtime_error("unknown integration-test active item");
    }
    return iterator->second;
}

std::vector<const ActiveItemDefinition*> ActiveItemDatabase::all() const {
    std::vector<const ActiveItemDefinition*> result;
    result.reserve(items_.size());
    for (const auto& [_, definition] : items_) {
        result.push_back(&definition);
    }
    return result;
}

bool DroneDatabase::contains(const DroneId&) const {
    return false;
}

const DroneDefinition& DroneDatabase::get(const DroneId&) const {
    throw std::runtime_error("unknown integration-test drone");
}

namespace {
int failures = 0;

void check(const bool condition, const std::string& message) {
    if (condition) {
        return;
    }
    ++failures;
    std::cerr << "[FAIL] " << message << '\n';
}

CombatEntity makeEntity(
    const std::uint64_t id,
    const EntityType type,
    const std::string& definitionId,
    const int hp
) {
    CombatEntity entity;
    entity.id = EntityId{id};
    entity.type = type;
    entity.definitionId = definitionId;
    entity.nameTextId = TextId(definitionId + ".name");
    entity.health = Health(hp);
    return entity;
}

StatusModifierDefinition additiveModifier(
    const EffectType effectType,
    const StatusModifierEntity entity,
    const int amount,
    const int priority
) {
    StatusModifierDefinition result;
    result.effectType = effectType;
    result.entity = entity;
    result.operation = StatusModifierOperation::AddPerStack;
    result.addAmount = amount;
    result.priority = priority;
    result.descriptionTextId = TextId("integration.modifier");
    return result;
}

StatusDatabase makeStatuses() {
    StatusDatabase statuses;

    const auto add = [&statuses](StatusDefinition definition) {
        statuses.add(std::move(definition));
    };

    StatusDefinition strength;
    strength.id = StatusId("strength");
    strength.nameTextId = TextId("status.strength.name");
    strength.descriptionTextId = TextId("status.strength.description");
    strength.type = StatusType::Buff;
    strength.durationRule = StatusDurationRule::PersistentCombat;
    strength.modifiers.push_back(additiveModifier(EffectType::Damage, StatusModifierEntity::Source, 1, 100));
    add(std::move(strength));

    StatusDefinition dexterity;
    dexterity.id = StatusId("dexterity");
    dexterity.nameTextId = TextId("status.dexterity.name");
    dexterity.descriptionTextId = TextId("status.dexterity.description");
    dexterity.type = StatusType::Buff;
    dexterity.durationRule = StatusDurationRule::PersistentCombat;
    dexterity.modifiers.push_back(additiveModifier(EffectType::Block, StatusModifierEntity::Source, 1, 100));
    add(std::move(dexterity));

    StatusDefinition poison;
    poison.id = StatusId("poison");
    poison.nameTextId = TextId("status.poison.name");
    poison.descriptionTextId = TextId("status.poison.description");
    poison.type = StatusType::Debuff;
    poison.durationRule = StatusDurationRule::Custom;
    StatusTriggerDefinition poisonTrigger;
    poisonTrigger.event = StatusTriggerEvent::EndOwnerTurn;
    poisonTrigger.effect = StatusTriggeredEffect::DamageHp;
    poisonTrigger.valuePerStack = 1;
    poisonTrigger.removeStacks = 1;
    poisonTrigger.logType = StatusTriggerLogType::PoisonDamage;
    poison.triggers.push_back(poisonTrigger);
    add(std::move(poison));

    return statuses;
}

EffectDefinition fixedEffect(
    const EffectType type,
    const EffectTarget target,
    const int value,
    std::string statusId = {}
) {
    EffectDefinition effect;
    effect.type = type;
    effect.target = target;
    effect.value = EffectValue::fixed(value);
    effect.statusId = std::move(statusId);
    return effect;
}

CardDefinition makeCard(
    const char* id,
    const CardType type,
    const int cost,
    std::vector<EffectDefinition> effects,
    std::vector<CardKeyword> keywords = {}
) {
    CardDefinition card;
    card.id = CardId(id);
    card.type = type;
    card.energyCost = cost;
    card.effects = std::move(effects);
    card.keywords = std::move(keywords);
    return card;
}

EnemyActionDefinition attackAction(const char* id, const int damage) {
    EnemyActionDefinition action;
    action.id = id;
    action.intentType = EnemyIntentType::Attack;
    action.weight = 1;
    action.maxConsecutiveUses = 1;
    action.effects.push_back(fixedEffect(EffectType::Damage, EffectTarget::RandomAlly, damage));
    return action;
}

CardInstanceId instanceIdFor(const CombatState& state, const CardId& definitionId) {
    for (const CardInstance& card : state.hand.cards()) {
        if (card.definitionId == definitionId) {
            return card.instanceId;
        }
    }
    return CardInstanceId{};
}

void testCompleteMultiEnemyCombatScenario() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatuses();
    ModifierSystem modifiers(localization, statuses);
    GameEventBus eventBus;
    DamageSystem damageSystem(modifiers, &eventBus);
    BlockSystem blockSystem(modifiers, &eventBus);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver effectResolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses, &eventBus);
    DroneDatabase drones;
    DroneSystem droneSystem(
        drones,
        effectResolver,
        targeting,
        damageSystem,
        blockSystem,
        energySystem,
        drawSystem,
        statusSystem
    );
    EffectSystem effectSystem(
        effectResolver,
        targeting,
        damageSystem,
        blockSystem,
        energySystem,
        drawSystem,
        statusSystem,
        droneSystem,
        &eventBus
    );

    CardDatabase cards;
    cards.add(makeCard(
        "battle_focus",
        CardType::Skill,
        0,
        {fixedEffect(EffectType::ApplyStatus, EffectTarget::Self, 1, "strength")},
        {CardKeyword::Innate, CardKeyword::Exhaust}
    ));
    cards.add(makeCard(
        "guard",
        CardType::Skill,
        1,
        {fixedEffect(EffectType::Block, EffectTarget::Self, 5)}
    ));
    cards.add(makeCard(
        "sweep",
        CardType::Attack,
        1,
        {fixedEffect(EffectType::Damage, EffectTarget::AllEnemies, 6)}
    ));
    cards.add(makeCard(
        "venom",
        CardType::Skill,
        1,
        {fixedEffect(EffectType::ApplyStatus, EffectTarget::SingleEnemy, 2, "poison")}
    ));
    cards.add(makeCard(
        "strike",
        CardType::Attack,
        1,
        {fixedEffect(EffectType::Damage, EffectTarget::SingleEnemy, 3)}
    ));

    EnemyDefinition firstEnemy;
    firstEnemy.id = EnemyId("enemy_a");
    firstEnemy.nameTextId = TextId("enemy.enemy_a.name");
    firstEnemy.maxHp = 8;
    firstEnemy.actions.push_back(attackAction("enemy_a.attack", 4));

    EnemyDefinition secondEnemy;
    secondEnemy.id = EnemyId("enemy_b");
    secondEnemy.nameTextId = TextId("enemy.enemy_b.name");
    secondEnemy.maxHp = 8;
    secondEnemy.actions.push_back(attackAction("enemy_b.attack", 3));

    EnemyDatabase enemies;
    enemies.add(firstEnemy);
    enemies.add(secondEnemy);

    RelicDefinition relic;
    relic.id = RelicId("training_glove");
    relic.nameTextId = TextId("relic.training_glove.name");
    relic.descriptionTextId = TextId("relic.training_glove.description");
    RelicTriggerDefinition trigger;
    trigger.eventType = GameEventType::CombatStarted;
    trigger.oncePerCombat = true;
    trigger.effects.push_back(fixedEffect(EffectType::ApplyStatus, EffectTarget::Self, 1, "dexterity"));
    relic.triggers.push_back(std::move(trigger));

    RelicDatabase relics;
    relics.add(std::move(relic));
    RelicSystem relicSystem(relics);
    relicSystem.setRelics({"training_glove"});
    relicSystem.startCombat();

    CardPlayValidator validator;
    CardPlaySystem cardPlay(cards, validator, energySystem, effectSystem, &eventBus);
    PlayerTurnSystem playerTurns(drawSystem, cards, &eventBus, &effectSystem);
    EnemyMoveSelector moveSelector(modifiers);
    EnemyTurnSystem enemyTurns(moveSelector, effectSystem);
    BossPhaseSystem bossPhases(enemies, effectSystem);
    CombatController combatController;
    TurnSystem turns(
        enemies,
        playerTurns,
        enemyTurns,
        moveSelector,
        bossPhases,
        statusSystem,
        droneSystem,
        combatController,
        5,
        &eventBus
    );

    CombatState state;
    state.players.push_back(makeEntity(1, EntityType::Player, "tester", 40));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "enemy_a", 8));
    state.enemies.push_back(makeEntity(3, EntityType::Enemy, "enemy_b", 8));
    state.resources.setMaxEnergy(state.players.front().id, 3);

    std::uint64_t nextInstanceId = 10;
    for (const CardDefinition* definition : cards.all()) {
        CardInstance instance;
        instance.instanceId = CardInstanceId{nextInstanceId++};
        instance.definitionId = definition->id;
        state.deck.drawPile.addTop(std::move(instance));
    }

    Random random(1901u);
    eventBus.subscribe([&](const GameEvent& event) {
        relicSystem.handleEvent(state, event, effectSystem, random);
    });

    turns.startCombat(state, random);
    check(state.phase == CombatPhase::PlayerTurn, "combat must start in the player phase");
    check(state.hand.size() == 5u, "opening hand must contain the complete five-card test deck");
    check(state.players.front().statuses.stacks("dexterity") == 1,
          "combat-start relic must grant dexterity through a status");
    check(state.enemyIntents.size() == 2u, "both enemies must receive intentions at combat start");

    const EntityId playerId = state.players.front().id;
    const EntityId firstEnemyId = state.enemies[0].id;

    const auto play = [&](const char* cardId, const std::optional<EntityId> target = std::nullopt) {
        const CardInstanceId instanceId = instanceIdFor(state, CardId(cardId));
        check(instanceId.value != 0, std::string("card must be present in hand: ") + cardId);
        const PlayCardResult result = cardPlay.playCard(
            state,
            PlayCardRequest{instanceId, playerId, target},
            random
        );
        check(result.played, std::string("card must play successfully: ") + cardId + " (" + result.reason + ")");
        combatController.updateAfterAction(state);
    };

    play("battle_focus");
    play("guard");
    play("sweep");
    play("venom", firstEnemyId);

    check(state.players.front().statuses.stacks("strength") == 1,
          "innate focus must grant strength before attack resolution");
    check(state.players.front().block == 6,
          "five card block plus one dexterity must produce six block");
    check(state.enemies[0].health.current() == 1 && state.enemies[1].health.current() == 1,
          "strength must raise the all-enemy attack from six to seven damage");
    check(state.enemies[0].statuses.stacks("poison") == 2,
          "the selected enemy must receive poison");

    turns.endPlayerTurn(state, random);

    check(state.turn == 2 && state.phase == CombatPhase::PlayerTurn,
          "a complete enemy phase must advance to the next player turn");
    check(!state.enemies[0].isAlive() && state.enemies[1].isAlive(),
          "poison must kill only the selected one-HP enemy at end of enemy turn");
    check(state.players.front().health.current() == 39,
          "six block must absorb six of the seven total enemy attack damage");
    check(state.enemyIntents.size() == 1u && state.enemyIntents.front().enemyId == state.enemies[1].id,
          "intent refresh must remove the dead enemy and retain the survivor");

    play("strike", state.enemies[1].id);
    const CombatResult result = combatController.updateAfterAction(state);
    check(result.outcome == CombatOutcome::Victory, "killing the final enemy must complete the combat");
    check(result.enemiesKilled == 2, "the final result must report both defeated enemies");
    check(state.phase == CombatPhase::Won, "victory must leave combat in the Won phase");
}


void testActiveItemRewardPersistenceScenario() {
    CardDatabase cards;
    const auto addRewardCard = [&cards](const char* id) {
        CardDefinition card;
        card.id = CardId(id);
        card.nameTextId = TextId(std::string("card.") + id + ".name");
        card.descriptionTextId = TextId(std::string("card.") + id + ".description");
        card.rarity = CardRarity::Common;
        card.type = CardType::Attack;
        card.ownerActorId = "tester";
        card.rewardPoolId = "tester";
        cards.add(std::move(card));
    };
    addRewardCard("old_card_a");
    addRewardCard("old_card_b");
    addRewardCard("new_card_a");
    addRewardCard("new_card_b");
    addRewardCard("new_card_c");

    RelicDatabase relics;
    RelicDefinition oldRelic;
    oldRelic.id = RelicId("old_relic");
    oldRelic.rarity = RelicRarity::Common;
    relics.add(std::move(oldRelic));
    RelicDefinition newRelic;
    newRelic.id = RelicId("new_relic");
    newRelic.rarity = RelicRarity::Common;
    relics.add(std::move(newRelic));

    ConsumableDatabase consumables;
    ConsumableDefinition oldConsumable;
    oldConsumable.id = ConsumableId("old_consumable");
    consumables.add(std::move(oldConsumable));
    ConsumableDefinition newConsumable;
    newConsumable.id = ConsumableId("new_consumable");
    consumables.add(std::move(newConsumable));

    ActiveItemDatabase activeItems;
    ActiveItemDefinition die;
    die.id = ActiveItemId("reroll_die");
    die.maxCharge = 4;
    die.chargeCost = 4;
    die.useContexts = {ActiveItemUseContext::Reward};
    die.effects.push_back(ActiveItemEffectDefinition{ActiveItemEffectType::RerollOffers, 0});
    activeItems.add(die);

    ActiveItemDefinition oldItem;
    oldItem.id = ActiveItemId("old_active");
    oldItem.canAppearInRewards = true;
    activeItems.add(std::move(oldItem));
    ActiveItemDefinition newItem;
    newItem.id = ActiveItemId("new_active");
    newItem.canAppearInRewards = true;
    activeItems.add(std::move(newItem));

    RunState run;
    run.actorDefinitionIds = {"tester"};
    run.rewardCardPoolIds = {"tester"};
    run.activeItem.itemId = "reroll_die";
    run.activeItem.charge = 4;
    run.pendingRoom.type = RunPendingRoomType::CombatReward;
    run.pendingRoom.nodeId = 11;
    run.pendingRoom.reward.sourceNodeType = RunMapNodeType::Elite;
    run.pendingRoom.reward.options = {
        RewardOption::goldReward(50),
        RewardOption::cardChoice({CardRewardOption{CardId("old_card_a")}, CardRewardOption{CardId("old_card_b")}}),
        RewardOption::relic("old_relic"),
        RewardOption::consumable("old_consumable"),
        RewardOption::activeItem("old_active")
    };

    NodeRewardTuning tuning;
    tuning.offerCards = true;
    tuning.cardChoices = 2;
    Random random(1904u);

    check(ActiveItemSystem::canUse(run.activeItem, die, ActiveItemUseContext::Reward),
          "a fully charged die must be usable in a reward room");
    const ActiveItemRerollResult rerolled = ActiveItemRerollSystem::rerollReward(
        run.pendingRoom.reward,
        run,
        cards,
        relics,
        consumables,
        activeItems,
        tuning,
        random
    );
    check(rerolled.cardOffersChanged == 1 && rerolled.relicOffersChanged == 1 &&
          rerolled.consumableOffersChanged == 1 && rerolled.activeItemOffersChanged == 1,
          "the die must reroll every supported pending reward offer in one operation");
    check(ActiveItemSystem::spendCharge(run, die, ActiveItemUseContext::Reward),
          "charge must be spent only after a successful persistent reroll");

    const RewardState& reward = run.pendingRoom.reward;
    check(reward.options[0].gold == 50, "rerolling must preserve already generated gold");
    check(reward.options[1].cardOptions[0].cardId != CardId("old_card_a") &&
          reward.options[1].cardOptions[1].cardId != CardId("old_card_b"),
          "rerolled card choices must not restore either old offer");
    check(reward.options[2].relicId == "new_relic", "rerolled relic must use the alternative reward pool");
    check(reward.options[3].consumableId == "new_consumable", "rerolled consumable must persist in pending state");
    check(reward.options[4].activeItemId == "new_active", "rerolled active item must persist in pending state");
    check(run.activeItem.charge == 0 && run.stats.activeItemsUsed == 1,
          "successful reroll must consume charge and update run statistics");

    const RunState checkpoint = run;
    check(checkpoint.pendingRoom.type == RunPendingRoomType::CombatReward &&
          checkpoint.pendingRoom.reward.options[2].relicId == "new_relic" &&
          checkpoint.activeItem.charge == 0,
          "a run checkpoint must retain rerolled offers and spent charge together");
}

void testBossPhaseAndArenaScenario() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatuses();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver effectResolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses);
    DroneDatabase drones;
    DroneSystem droneSystem(
        drones,
        effectResolver,
        targeting,
        damageSystem,
        blockSystem,
        energySystem,
        drawSystem,
        statusSystem
    );
    EffectSystem effectSystem(
        effectResolver,
        targeting,
        damageSystem,
        blockSystem,
        energySystem,
        drawSystem,
        statusSystem,
        droneSystem
    );

    EnemyDefinition minion;
    minion.id = EnemyId("summoned_guard");
    minion.nameTextId = TextId("enemy.summoned_guard.name");
    minion.maxHp = 10;
    minion.actions.push_back(attackAction("summoned_guard.attack", 1));

    EnemyDefinition boss;
    boss.id = EnemyId("integration_boss");
    boss.nameTextId = TextId("enemy.integration_boss.name");
    boss.maxHp = 30;
    boss.actions.push_back(attackAction("integration_boss.opening", 2));
    boss.actions.push_back(attackAction("integration_boss.final", 5));

    EnemyPhaseDefinition opening;
    opening.id = "opening";
    opening.nameTextId = TextId("enemy.phase.integration_boss.opening");
    opening.activateBelowHpPercent = 100;
    opening.actionIds = {"integration_boss.opening"};

    EnemyPhaseDefinition final;
    final.id = "final";
    final.nameTextId = TextId("enemy.phase.integration_boss.final");
    final.activateBelowHpPercent = 50;
    final.actionIds = {"integration_boss.final"};
    final.summonEnemyIds = {"summoned_guard", "summoned_guard", "summoned_guard"};
    final.maximumAliveEnemies = 3;
    final.onEnterEffects.push_back(fixedEffect(EffectType::ApplyStatus, EffectTarget::Self, 2, "strength"));
    final.playerTurnEffects.push_back(fixedEffect(EffectType::Damage, EffectTarget::AllAllies, 2));
    boss.phases = {opening, final};

    EnemyDatabase enemies;
    enemies.add(std::move(minion));
    enemies.add(std::move(boss));

    CombatState state;
    state.players.push_back(makeEntity(100, EntityType::Player, "tester", 20));
    state.enemies.push_back(makeEntity(101, EntityType::Enemy, "integration_boss", 30));
    state.enemyHpMultiplier = 2.f;
    state.turn = 1;

    BossPhaseSystem phases(enemies, effectSystem);
    EnemyMoveSelector selector(modifiers);
    Random random(1902u);

    check(phases.synchronizePhases(state, random), "boss opening phase must activate");
    state.enemies.front().health.setCurrent(14);
    check(phases.synchronizePhases(state, random), "boss final phase must activate below half HP");
    check(state.enemies.front().statuses.stacks("strength") == 2,
          "phase-entry strength must be represented as a status");
    check(state.aliveEnemyCount() == 3u,
          "summoning must stop at the configured three-enemy combat limit");
    check(state.enemies[1].health.maximum() == 20,
          "summoned enemies must inherit the run HP multiplier");
    check(selector.selectAction(state, enemies.get(EnemyId("integration_boss")), state.enemies.front().id, random).id
              == "integration_boss.final",
          "boss AI must use the final-phase action pool immediately");

    phases.applyPlayerTurnEffects(state, random);
    phases.applyPlayerTurnEffects(state, random);
    check(state.players.front().health.current() == 18,
          "arena rule must apply once, not once per refresh, during the same turn");
    ++state.turn;
    phases.applyPlayerTurnEffects(state, random);
    check(state.players.front().health.current() == 16,
          "arena rule must trigger again on a later player turn");
}

} // namespace

int main() {
    testCompleteMultiEnemyCombatScenario();
    testActiveItemRewardPersistenceScenario();
    testBossPhaseAndArenaScenario();

    if (failures != 0) {
        std::cerr << failures << " gameplay integration test(s) failed\n";
        return 1;
    }

    std::cout << "All gameplay integration tests passed\n";
    return 0;
}
