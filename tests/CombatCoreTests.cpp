#include "active_items/ActiveItemAcquisitionSystem.hpp"
#include "active_items/ActiveItemContextSystem.hpp"
#include "active_items/ActiveItemSystem.hpp"
#include "active_items/ActiveItemRerollSystem.hpp"
#include "cards/CardId.hpp"
#include "cards/DrawSystem.hpp"
#include "combat/BlockSystem.hpp"
#include "combat/BossPhaseSystem.hpp"
#include "combat/CombatController.hpp"
#include "combat/CardPlaySystem.hpp"
#include "combat/CardPlayValidator.hpp"
#include "combat/CardCost.hpp"
#include "combat/CardStressCost.hpp"
#include "combat/CombatState.hpp"
#include "combat/DamageSystem.hpp"
#include "combat/EffectResolver.hpp"
#include "combat/EffectScaling.hpp"
#include "combat/EffectSystem.hpp"
#include "combat/EnergySystem.hpp"
#include "combat/ModifierSystem.hpp"
#include "combat/PlayerTurnSystem.hpp"
#include "combat/EnemyMoveSelector.hpp"
#include "combat/Targeting.hpp"
#include "core/Random.hpp"
#include "dice/DiceCorruption.hpp"
#include "drones/DroneDatabase.hpp"
#include "drones/DroneSystem.hpp"
#include "effects/EffectTarget.hpp"
#include "effects/EffectType.hpp"
#include "entities/CombatEntity.hpp"
#include "enemies/EnemyRole.hpp"
#include "events/RunEventRequirement.hpp"
#include "events/RunEventSelector.hpp"
#include "enemies/EnemyDefinition.hpp"
#include "data/EnemyDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "relics/RelicDatabase.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "rewards/CardRewardQuality.hpp"
#include "run/StressEconomyRules.hpp"
#include "run/StressPsychopathRules.hpp"
#include "run/StressRules.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "game/GameEvent.hpp"
#include "game/GameEventBus.hpp"
#include "localization/Locale.hpp"
#include "localization/LocalizationManager.hpp"
#include "statuses/StatusDatabase.hpp"
#include "statuses/StatusSystem.hpp"
#include "shop/ShopEconomy.hpp"
#include "ui/EnemyIntentPresentation.hpp"

#include <cstdint>
#include <iostream>
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
        throw std::runtime_error("invalid test status id");
    }
    statuses_.emplace(id, std::move(definition));
}

bool StatusDatabase::contains(const StatusId& id) const {
    return statuses_.contains(id.value);
}

const StatusDefinition& StatusDatabase::get(const StatusId& id) const {
    const auto iterator = statuses_.find(id.value);
    if (iterator == statuses_.end()) {
        throw std::runtime_error("unknown test status");
    }
    return iterator->second;
}

bool DroneDatabase::contains(const DroneId&) const {
    return false;
}

const DroneDefinition& DroneDatabase::get(const DroneId&) const {
    throw std::runtime_error("unknown test drone");
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
        throw std::runtime_error("unknown test enemy");
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
        throw std::runtime_error("unknown test card");
    }
    return iterator->second;
}

std::vector<const CardDefinition*> CardDatabase::all() const {
    std::vector<const CardDefinition*> result;
    for (const auto& [_, definition] : cards_) {
        result.push_back(&definition);
    }
    return result;
}

void RelicDatabase::add(RelicDefinition definition) {
    relics_.emplace(definition.id.value, std::move(definition));
}

std::vector<const RelicDefinition*> RelicDatabase::all() const {
    std::vector<const RelicDefinition*> result;
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
        throw std::runtime_error("unknown test active item");
    }
    return iterator->second;
}

std::vector<const ActiveItemDefinition*> ActiveItemDatabase::all() const {
    std::vector<const ActiveItemDefinition*> result;
    for (const auto& [_, definition] : items_) {
        result.push_back(&definition);
    }
    return result;
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

EnemyActionDefinition makeEnemyAction(
    std::string id,
    const int weight = 1,
    const int cooldown = 0,
    const int maxConsecutiveUses = 0
) {
    EnemyActionDefinition action;
    action.id = std::move(id);
    action.intentType = EnemyIntentType::Attack;
    action.weight = weight;
    action.cooldown = cooldown;
    action.maxConsecutiveUses = maxConsecutiveUses;

    EffectDefinition damage;
    damage.type = EffectType::Damage;
    damage.target = EffectTarget::RandomAlly;
    damage.value = EffectValue::fixed(1);
    action.effects.push_back(damage);
    return action;
}

StatusDatabase makeStatusDatabase() {
    StatusDatabase statuses;

    auto modifier = [](
        const EffectType effectType,
        const StatusModifierEntity entity,
        const StatusModifierOperation operation,
        const int addAmount,
        const double multiplier,
        const int priority,
        const char* description
    ) {
        StatusModifierDefinition result;
        result.effectType = effectType;
        result.entity = entity;
        result.operation = operation;
        result.addAmount = addAmount;
        result.multiplier = multiplier;
        result.priority = priority;
        result.descriptionTextId = TextId(description);
        return result;
    };

    auto add = [&statuses](
        const char* id,
        const StatusType type,
        const StatusDurationRule duration,
        std::vector<StatusModifierDefinition> modifiers = {},
        std::vector<StatusTriggerDefinition> triggers = {},
        std::string exclusiveGroup = {}
    ) {
        StatusDefinition definition;
        definition.id = StatusId(id);
        definition.nameTextId = TextId(std::string("status.") + id + ".name");
        definition.descriptionTextId = TextId(std::string("status.") + id + ".description");
        definition.type = type;
        definition.durationRule = duration;
        definition.modifiers = std::move(modifiers);
        definition.triggers = std::move(triggers);
        definition.exclusiveGroup = std::move(exclusiveGroup);
        statuses.add(std::move(definition));
    };

    add("strength", StatusType::Buff, StatusDurationRule::PersistentCombat, {
        modifier(EffectType::Damage, StatusModifierEntity::Source, StatusModifierOperation::AddPerStack,
                 1, 1.0, 100, "modifier.status.strength.outgoing_damage_add")
    });
    add("dexterity", StatusType::Buff, StatusDurationRule::PersistentCombat, {
        modifier(EffectType::Block, StatusModifierEntity::Source, StatusModifierOperation::AddPerStack,
                 1, 1.0, 100, "modifier.status.dexterity.block_add")
    });
    add("ferocity", StatusType::Buff, StatusDurationRule::PersistentCombat, {
        modifier(EffectType::Damage, StatusModifierEntity::Source, StatusModifierOperation::MultiplyPerStack,
                 0, 0.05, 145, "modifier.status.ferocity.outgoing_damage_increase")
    });
    add("weak", StatusType::Debuff, StatusDurationRule::DecreaseEndOfOwnerTurn, {
        modifier(EffectType::Damage, StatusModifierEntity::Source, StatusModifierOperation::MultiplyFixed,
                 0, 0.75, 200, "modifier.status.weak.outgoing_damage_reduce")
    });
    add("vulnerable", StatusType::Debuff, StatusDurationRule::DecreaseEndOfOwnerTurn, {
        modifier(EffectType::Damage, StatusModifierEntity::Target, StatusModifierOperation::MultiplyFixed,
                 0, 1.5, 300, "modifier.status.vulnerable.incoming_damage_increase")
    });

    StatusTriggerDefinition poisonTrigger;
    poisonTrigger.event = StatusTriggerEvent::EndOwnerTurn;
    poisonTrigger.effect = StatusTriggeredEffect::DamageHp;
    poisonTrigger.valuePerStack = 1;
    poisonTrigger.removeStacks = 1;
    poisonTrigger.logType = StatusTriggerLogType::PoisonDamage;
    add("poison", StatusType::Debuff, StatusDurationRule::Custom, {}, {poisonTrigger});

    StatusTriggerDefinition burnTrigger = poisonTrigger;
    burnTrigger.logType = StatusTriggerLogType::BurnDamage;
    add("burn", StatusType::Debuff, StatusDurationRule::Custom, {}, {burnTrigger});

    add("stance_flame", StatusType::Buff, StatusDurationRule::PersistentCombat, {}, {}, "monk_stance");
    add("stance_ash", StatusType::Buff, StatusDurationRule::PersistentCombat, {}, {}, "monk_stance");
    add("stance_smoke", StatusType::Buff, StatusDurationRule::PersistentCombat, {}, {}, "monk_stance");
    return statuses;
}

void testDamageAndBlockModifiers() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damage(modifiers);
    BlockSystem block(modifiers);

    CombatState state;
    state.players.push_back(makeEntity(1, EntityType::Player, "tester", 50));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "target", 50));

    CombatEntity& source = state.players.front();
    CombatEntity& target = state.enemies.front();
    source.statuses.set("strength", 2);
    source.statuses.set("weak", 1);
    source.statuses.set("dexterity", 2);
    target.statuses.set("vulnerable", 1);
    target.block = 5;

    const DamageResult damageResult = damage.dealDamage(
        state,
        source.id,
        target.id,
        10,
        CardId("test.attack"),
        DiceCorruption{},
        true
    );

    check(damageResult.modifiedDamage == 13, "strength, weak and vulnerable must be applied in priority order");
    check(damageResult.blockedDamage == 5, "damage must consume the target's block");
    check(damageResult.hpDamage == 8, "remaining damage must reach HP");
    check(target.health.current() == 42, "target HP must reflect modified and blocked damage");
    const CombatLogEntry& damageLog = state.log.entries().back();
    check(damageLog.type == CombatLogEntryType::DamageDealt, "damage must create a structured journal entry");
    check(damageLog.variables.contains("source_text_id") && damageLog.variables.contains("target_text_id"),
          "damage journal entry must identify source and target");
    check(damageLog.variables.contains("modifiers") && !damageLog.variables.at("modifiers").empty(),
          "damage journal entry must explain modifier-driven value changes");

    const BlockResult blockResult = block.gainBlock(
        state,
        source.id,
        source.id,
        6,
        CardId("test.block"),
        DiceCorruption{},
        true
    );
    check(blockResult.modifiedBlock == 8, "dexterity must increase gained block");
    check(source.block == 8, "gained block must be stored on the target entity");
    const CombatLogEntry& blockLog = state.log.entries().back();
    check(blockLog.type == CombatLogEntryType::BlockGained, "block must create a structured journal entry");
    check(blockLog.variables.contains("modifiers") && !blockLog.variables.at("modifiers").empty(),
          "block journal entry must explain modifier-driven value changes");
}


void testStatusesOwnCardNumberScaling() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damage(modifiers);
    BlockSystem block(modifiers);

    CombatState state;
    state.players.push_back(makeEntity(1, EntityType::Player, "tester", 80));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "target", 80));

    CombatEntity& source = state.players.front();
    CombatEntity& target = state.enemies.front();
    source.statuses.set("strength", 2);
    source.statuses.set("dexterity", 2);
    source.statuses.set("ferocity", 2);

    const DamageResult cardDamage = damage.dealDamage(
        state,
        source.id,
        target.id,
        10,
        CardId("test.card_attack"),
        DiceCorruption{},
        true
    );
    check(cardDamage.modifiedDamage == 13, "Strength and Ferocity must modify card attack damage");

    const BlockResult cardBlock = block.gainBlock(
        state,
        source.id,
        source.id,
        6,
        CardId("test.card_block"),
        DiceCorruption{},
        true
    );
    check(cardBlock.modifiedBlock == 8, "Dexterity must modify block granted by cards");

    const DamageResult relicDamage = damage.dealDamage(
        state,
        source.id,
        target.id,
        10,
        CardId("relic.test_damage"),
        DiceCorruption{},
        false
    );
    check(relicDamage.modifiedDamage == 10, "Actor statuses must not modify direct relic damage");

    const BlockResult relicBlock = block.gainBlock(
        state,
        source.id,
        source.id,
        6,
        CardId("relic.test_block"),
        DiceCorruption{},
        false
    );
    check(relicBlock.modifiedBlock == 6, "Dexterity must not modify direct relic block");
}

void testDamageOverTimeAndDurations() {
    StatusDatabase statuses = makeStatusDatabase();
    GameEventBus eventBus;
    std::vector<GameEvent> events;
    eventBus.subscribe([&events](const GameEvent& event) { events.push_back(event); });
    StatusSystem statusSystem(statuses, &eventBus);

    CombatState state;
    state.players.push_back(makeEntity(1, EntityType::Player, "source", 40));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "burn_target", 20));
    state.enemies.push_back(makeEntity(3, EntityType::Enemy, "poison_target", 20));

    statusSystem.applyStatus(state, state.enemies[0].id, "burn", 3, state.players[0].id);
    statusSystem.applyStatus(state, state.enemies[1].id, "poison", 2, state.players[0].id);
    statusSystem.applyStatus(state, state.enemies[0].id, "weak", 2, state.players[0].id);
    statusSystem.onTurnEndedForSide(state, EntityType::Enemy);

    check(state.enemies[0].health.current() == 17, "burn must deal damage equal to its current stacks");
    check(state.enemies[0].statuses.stacks("burn") == 2, "burn must decrease after triggering");
    check(state.enemies[1].health.current() == 18, "poison must still deal damage after the shared refactor");
    check(state.enemies[1].statuses.stacks("poison") == 1, "poison must still decrease after triggering");
    check(state.enemies[0].statuses.stacks("weak") == 1, "owner-turn durations must decrease once");

    int burnDamageEvents = 0;
    int poisonDamageEvents = 0;
    for (const GameEvent& event : events) {
        if (event.type != GameEventType::DamageTaken) {
            continue;
        }
        if (event.statusId == "burn") {
            ++burnDamageEvents;
        }
        if (event.statusId == "poison") {
            ++poisonDamageEvents;
        }
    }
    check(burnDamageEvents == 1, "burn must emit a DamageTaken event for feedback and relics");
    check(poisonDamageEvents == 1, "poison must continue emitting a DamageTaken event");
}

void testMultiEnemyOutcomeAndIntentCleanup() {
    CombatState state;
    state.players.push_back(makeEntity(1, EntityType::Player, "player", 30));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "enemy_a", 10));
    state.enemies.push_back(makeEntity(3, EntityType::Enemy, "enemy_b", 10));

    EnemyIntentState firstIntent;
    firstIntent.enemyId = state.enemies[0].id;
    EnemyIntentState secondIntent;
    secondIntent.enemyId = state.enemies[1].id;
    state.enemyIntents = {firstIntent, secondIntent};

    CombatController controller;
    state.enemies[0].health.setCurrent(0);
    const CombatResult ongoing = controller.updateAfterAction(state);
    check(ongoing.outcome == CombatOutcome::Ongoing, "combat must continue while at least one enemy is alive");
    check(state.enemyIntents.size() == 1, "dead enemies must lose their stored intents");
    check(state.enemyIntents.front().enemyId == state.enemies[1].id, "the living enemy intent must be preserved");

    state.enemies[1].health.setCurrent(0);
    const CombatResult victory = controller.updateAfterAction(state);
    check(victory.outcome == CombatOutcome::Victory, "combat must end only after every enemy dies");
    check(victory.enemiesKilled == 2, "combat result must count all enemies in a group");
    check(victory.killedEnemyIds.size() == 2, "combat result must retain every killed enemy definition id");
    check(state.phase == CombatPhase::Won, "victory must transition combat into the Won phase");
}

void testMultiEnemyTargeting() {
    CombatState state;
    state.players.push_back(makeEntity(1, EntityType::Player, "player", 30));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "enemy_a", 10));
    state.enemies.push_back(makeEntity(3, EntityType::Enemy, "enemy_b", 10));
    state.enemies.push_back(makeEntity(4, EntityType::Enemy, "enemy_c", 10));
    state.enemies[1].health.setCurrent(0);

    Targeting targeting;
    EffectContext context;
    context.source = state.players.front().id;
    context.explicitEnemyTarget = state.enemies[2].id;
    Random random(7);
    context.random = &random;

    const std::vector<EntityId> all = targeting.resolveTargets(state, EffectTarget::AllEnemies, context);
    check(all.size() == 2, "all-enemy targeting must exclude dead enemies");

    const std::vector<EntityId> single = targeting.resolveTargets(state, EffectTarget::SingleEnemy, context);
    check(single.size() == 1 && single.front() == state.enemies[2].id, "single-enemy targeting must preserve the selected living target");

    state.enemies[2].health.setCurrent(0);
    const std::vector<EntityId> deadExplicitTarget = targeting.resolveTargets(state, EffectTarget::SingleEnemy, context);
    check(
        deadExplicitTarget.empty(),
        "remaining effects and repeated hits must skip an explicitly selected enemy killed by an earlier effect"
    );
    state.enemies[2].health.setCurrent(10);

    for (int i = 0; i < 20; ++i) {
        const std::vector<EntityId> randomTarget = targeting.resolveTargets(state, EffectTarget::RandomEnemy, context);
        check(randomTarget.size() == 1, "random-enemy targeting must return one target while enemies are alive");
        check(randomTarget.front() != state.enemies[1].id, "random-enemy targeting must never select a dead enemy");
    }

    context.source = state.enemies.front().id;
    const std::vector<EntityId> enemyTeam = targeting.resolveTargets(state, EffectTarget::AllEnemies, context);
    check(enemyTeam.size() == 2, "enemy support effects must target the living enemy team");
    check(
        enemyTeam[0] == state.enemies[0].id && enemyTeam[1] == state.enemies[2].id,
        "enemy support targeting must preserve the living enemy formation"
    );
}


void testExplicitAndAutomaticSingleTargetSafety() {
    CombatState state;
    state.players.push_back(makeEntity(11, EntityType::Player, "player_a", 30));
    state.players.push_back(makeEntity(12, EntityType::Player, "player_b", 30));
    state.players.push_back(makeEntity(13, EntityType::Player, "player_c", 30));
    state.enemies.push_back(makeEntity(21, EntityType::Enemy, "enemy_a", 10));
    state.enemies.push_back(makeEntity(22, EntityType::Enemy, "enemy_b", 10));

    Targeting targeting;
    EffectContext context;
    context.source = state.players[0].id;

    bool missingEnemyTargetRejected = false;
    try {
        (void)targeting.resolveTargets(state, EffectTarget::SingleEnemy, context);
    } catch (const std::runtime_error&) {
        missingEnemyTargetRejected = true;
    }
    check(
        missingEnemyTargetRejected,
        "single-enemy effects must require a choice while several enemies are alive"
    );

    state.enemies[1].health.setCurrent(0);
    const std::vector<EntityId> automaticEnemy = targeting.resolveTargets(
        state,
        EffectTarget::SingleEnemy,
        context
    );
    check(
        automaticEnemy.size() == 1u && automaticEnemy.front() == state.enemies[0].id,
        "single-enemy effects must auto-target the only living enemy"
    );

    context.explicitEnemyTarget = state.enemies[0].id;
    state.enemies[0].health.setCurrent(0);
    check(
        targeting.resolveTargets(state, EffectTarget::SingleEnemy, context).empty(),
        "a selected enemy killed earlier in the effect chain must not be replaced by another target"
    );

    context.explicitEnemyTarget.reset();
    state.enemies[0].health.setCurrent(10);
    bool missingAllyTargetRejected = false;
    try {
        (void)targeting.resolveTargets(state, EffectTarget::Ally, context);
    } catch (const std::runtime_error&) {
        missingAllyTargetRejected = true;
    }
    check(
        missingAllyTargetRejected,
        "single-ally effects must not silently affect every available ally"
    );

    context.explicitAllyTarget = state.players[1].id;
    const std::vector<EntityId> selectedAlly = targeting.resolveTargets(state, EffectTarget::Ally, context);
    check(
        selectedAlly.size() == 1u && selectedAlly.front() == state.players[1].id,
        "single-ally effects must preserve the explicitly selected living ally"
    );

    state.players[1].health.setCurrent(0);
    check(
        targeting.resolveTargets(state, EffectTarget::Ally, context).empty(),
        "a selected ally defeated earlier in the chain must make later effects miss"
    );

    context.explicitAllyTarget.reset();
    const std::vector<EntityId> automaticAlly = targeting.resolveTargets(state, EffectTarget::Ally, context);
    check(
        automaticAlly.size() == 1u && automaticAlly.front() == state.players[2].id,
        "single-ally effects may auto-target only when exactly one valid ally remains"
    );
}

void testEffectChainSkipsTargetsKilledByReactions() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
    ModifierSystem modifiers(localization, statuses);
    GameEventBus eventBus;
    DamageSystem damageSystem(modifiers, &eventBus);
    BlockSystem blockSystem(modifiers, &eventBus);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver resolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses, &eventBus);
    DroneDatabase drones;
    DroneSystem droneSystem(
        drones,
        resolver,
        targeting,
        damageSystem,
        blockSystem,
        energySystem,
        drawSystem,
        statusSystem
    );
    EffectSystem effects(
        resolver,
        targeting,
        damageSystem,
        blockSystem,
        energySystem,
        drawSystem,
        statusSystem,
        droneSystem,
        &eventBus
    );

    CombatState state;
    state.players.push_back(makeEntity(31, EntityType::Player, "player", 30));
    state.players.front().stress = 20;
    state.enemies.push_back(makeEntity(41, EntityType::Enemy, "enemy_a", 10));
    state.enemies.push_back(makeEntity(42, EntityType::Enemy, "enemy_b", 10));

    int damageEvents = 0;
    eventBus.subscribe([&](const GameEvent& event) {
        if (event.type != GameEventType::DamageDealt) {
            return;
        }
        ++damageEvents;
        if (event.target == state.enemies[0].id) {
            state.enemies[1].health.setCurrent(0);
        }
    });

    EffectDefinition sweep;
    sweep.type = EffectType::Damage;
    sweep.target = EffectTarget::AllEnemies;
    sweep.value = EffectValue::fixed(3);

    EffectContext context;
    context.source = state.players.front().id;
    Random random(99u);
    context.random = &random;
    effects.applyEffect(state, sweep, context);

    check(damageEvents == 1, "targets killed by a reaction must not receive later effects from a stale target snapshot");
    check(state.enemies[0].health.current() == 7, "the first living group target must still receive damage");
    check(state.enemies[1].health.current() == 0, "the reactively killed target must remain defeated without duplicate damage");

    EffectDefinition stressStrike;
    stressStrike.type = EffectType::SpendStressDamage;
    stressStrike.target = EffectTarget::SingleEnemy;
    stressStrike.value = EffectValue::fixed(5);
    stressStrike.outputAmount = 12;
    context.explicitEnemyTarget = state.enemies[1].id;
    const int beforeStress = state.players.front().stress;
    effects.applyEffect(state, stressStrike, context);
    check(
        state.players.front().stress == beforeStress,
        "stress conversion effects must not spend their resource after the selected target has died"
    );
}


void testConditionalEnemyAi() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
    ModifierSystem modifiers(localization, statuses);
    EnemyMoveSelector selector(modifiers);

    CombatState state;
    state.turn = 1;
    state.players.push_back(makeEntity(1, EntityType::Player, "player", 40));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "tactician", 30));

    EnemyDefinition definition;
    definition.id = EnemyId("tactician");

    EnemyActionDefinition solo = makeEnemyAction("solo", 1, 0, 1);
    solo.condition.maxAliveEnemies = 1;

    EnemyActionDefinition protect = makeEnemyAction("protect", 50, 1, 1);
    protect.intentType = EnemyIntentType::Block;
    protect.condition.minAliveEnemies = 2;
    protect.condition.anyOtherEnemyHpBelowPercent = 80;

    EnemyActionDefinition setup = makeEnemyAction("setup", 1, 0, 1);
    setup.intentType = EnemyIntentType::Debuff;
    setup.condition.forbiddenPlayerStatuses = {"vulnerable"};

    EnemyActionDefinition payoff = makeEnemyAction("payoff", 50, 2, 1);
    payoff.condition.requiredPlayerStatuses = {"vulnerable"};

    definition.actions = {solo, protect, setup, payoff};

    Random random(17);
    check(
        selector.selectAction(state, definition, state.enemies[0].id, random).id != "protect",
        "group support actions must stay unavailable without a living ally"
    );

    state.enemies.push_back(makeEntity(3, EntityType::Enemy, "ally", 20));
    state.enemies.back().health.setCurrent(10);
    check(
        selector.selectAction(state, definition, state.enemies[0].id, random).id == "protect",
        "support actions must react to a wounded living ally"
    );

    state.players[0].statuses.set("vulnerable", 1);
    check(
        selector.selectAction(state, definition, state.enemies[0].id, random).id == "payoff",
        "status payoff actions must react to player statuses"
    );

    EnemyAiState aiState;
    aiState.enemyId = state.enemies[0].id;
    aiState.lastActionId = "payoff";
    aiState.consecutiveUses = 1;
    aiState.cooldowns.push_back({"payoff", 4});
    state.enemyAiStates = {aiState};
    state.turn = 2;
    check(
        selector.selectAction(state, definition, state.enemies[0].id, random).id != "payoff",
        "cooldowns and consecutive-use limits must prevent immediate repetition"
    );

    EnemyDefinition weighted;
    weighted.id = EnemyId("weighted");
    weighted.actions = {
        makeEnemyAction("rare", 1),
        makeEnemyAction("common", 9)
    };
    int commonSelections = 0;
    Random weightedRandom(42);
    for (int i = 0; i < 500; ++i) {
        if (selector.selectAction(state, weighted, state.enemies[0].id, weightedRandom).id == "common") {
            ++commonSelections;
        }
    }
    check(commonSelections > 400, "weighted selection must materially favor high-weight actions");

    EnemyDatabase database;
    EnemyDefinition recorded;
    recorded.id = EnemyId("tactician");
    recorded.actions = {
        makeEnemyAction("first", 100, 1, 1),
        makeEnemyAction("second", 1, 0, 1)
    };
    database.add(recorded);
    state.enemyAiStates.clear();
    state.enemies.resize(1);
    state.turn = 1;
    selector.refreshIntents(state, database, random);
    check(state.enemyAiStates.size() == 1, "intent refresh must create persistent enemy AI history");
    check(
        state.enemyAiStates[0].lastActionId == state.enemyIntents[0].actionId,
        "AI history must record the action displayed as the enemy intent"
    );
    check(
        !state.enemyAiStates[0].cooldowns.empty(),
        "intent refresh must record action cooldown availability"
    );
}


void testActiveItemChargeAndUse() {
    ActiveItemDefinition item;
    item.id = ActiveItemId("field_kit");
    item.maxCharge = 5;
    item.chargeCost = 3;
    item.startingCharge = 1;
    item.combatCharge = 1;
    item.eliteCharge = 3;
    item.bossCharge = 5;
    item.useContexts = {ActiveItemUseContext::Map};
    item.effects = {{ActiveItemEffectType::HealParty, 8}};

    RunState run;
    RunActorState actor;
    actor.definitionId = "tester";
    actor.currentHp = 5;
    actor.maxHp = 20;
    run.actorStates.push_back(actor);

    ActiveItemSystem::equip(run, item);
    check(run.activeItem.itemId == "field_kit", "equipping must fill the single active-item slot");
    check(run.activeItem.charge == 1, "new active items must use their data-driven starting charge");
    check(
        ActiveItemSystem::normalCombatRoomsUntilUsable(run.activeItem, item) == 2,
        "charge forecasting must report how many normal combats remain before use"
    );

    check(
        ActiveItemSystem::addCombatRoomCharge(run, item, RunMapNodeType::Combat) == 1,
        "normal enemy rooms must use the item-specific charge rate"
    );
    check(
        ActiveItemSystem::addCombatRoomCharge(run, item, RunMapNodeType::Elite) == 3,
        "elite rooms must use the item-specific charge rate"
    );
    check(run.activeItem.charge == 5, "active-item charge must clamp to max_charge");
    check(run.stats.activeItemChargeGained == 4, "actual charge gain must be recorded in run stats");
    check(
        ActiveItemSystem::normalCombatRoomsUntilUsable(run.activeItem, item) == 0,
        "fully usable items must report zero remaining rooms"
    );

    check(
        !ActiveItemSystem::canUse(run.activeItem, item, ActiveItemUseContext::Shop),
        "active items must reject contexts not listed in their definition"
    );

    const ActiveItemUseResult result = ActiveItemSystem::use(run, item, ActiveItemUseContext::Map);
    check(result.used(), "an item at its use threshold must be usable in an allowed context");
    check(run.actorStates[0].currentHp == 13, "field kit effect must heal living run actors");
    check(run.activeItem.charge == 2, "using an item must spend only charge_cost and preserve banked charge");
    check(run.stats.activeItemsUsed == 1, "successful item uses must be recorded");

    const ActiveItemUseResult insufficientCharge = ActiveItemSystem::use(run, item, ActiveItemUseContext::Map);
    check(
        insufficientCharge.status == ActiveItemUseStatus::NotEnoughCharge,
        "an item below its use threshold must not fire"
    );
}

void testRerollDieOffers() {
    CardDatabase cards;
    for (int index = 1; index <= 8; ++index) {
        CardDefinition card;
        card.id = CardId("card_" + std::to_string(index));
        card.type = CardType::Attack;
        card.rarity = index == 8 ? CardRarity::Rare : CardRarity::Common;
        card.rewardPoolId = "tester";
        card.goldCost = 15 + index;
        cards.add(std::move(card));
    }

    RelicDatabase relics;
    for (int index = 1; index <= 5; ++index) {
        RelicDefinition relic;
        relic.id = RelicId("relic_" + std::to_string(index));
        relic.rarity = RelicRarity::Common;
        relics.add(std::move(relic));
    }

    ConsumableDatabase consumables;
    for (int index = 1; index <= 4; ++index) {
        ConsumableDefinition consumable;
        consumable.id = ConsumableId("potion_" + std::to_string(index));
        consumable.goldCost = 20 + index;
        consumables.add(std::move(consumable));
    }

    ActiveItemDatabase activeItems;
    for (const std::string& id : {std::string("field_kit"), std::string("reroll_die"), std::string("compass")}) {
        ActiveItemDefinition item;
        item.id = ActiveItemId(id);
        item.maxCharge = 4;
        item.chargeCost = 4;
        item.shopPrice = 120;
        item.canAppearInRewards = true;
        item.canAppearInShop = true;
        item.useContexts = {ActiveItemUseContext::Reward, ActiveItemUseContext::Shop};
        item.effects = {{ActiveItemEffectType::RerollOffers, 1}};
        activeItems.add(std::move(item));
    }

    RunState run;
    run.actorDefinitionIds = {"tester"};
    run.rewardCardPoolIds = {"tester"};
    run.relicIds = {"relic_5"};

    RewardState reward;
    reward.sourceNodeType = RunMapNodeType::Elite;
    reward.options.push_back(RewardOption::goldReward(35));
    reward.options.push_back(RewardOption::cardChoice({
        CardRewardOption{CardId("card_1")},
        CardRewardOption{CardId("card_2")},
        CardRewardOption{CardId("card_3")}
    }));
    reward.options.push_back(RewardOption::consumable("potion_1"));
    reward.options.push_back(RewardOption::relic("relic_1"));

    NodeRewardTuning rewardTuning;
    rewardTuning.minimumCardRarity = CardRarity::Common;
    Random rewardRandom(17u);
    const ActiveItemRerollResult rewardResult = ActiveItemRerollSystem::rerollReward(
        reward,
        run,
        cards,
        relics,
        consumables,
        activeItems,
        rewardTuning,
        rewardRandom
    );

    check(rewardResult.cardOffersChanged == 1, "the die must reroll the card reward group");
    check(rewardResult.relicOffersChanged == 1, "the die must reroll relic rewards");
    check(rewardResult.consumableOffersChanged == 1, "the die must reroll consumable rewards");
    check(reward.options[0].gold == 35, "the die must not alter earned gold");
    for (const CardRewardOption& option : reward.options[1].cardOptions) {
        check(
            option.cardId.value != "card_1" && option.cardId.value != "card_2" && option.cardId.value != "card_3",
            "rerolled card rewards must not reuse the old offer set"
        );
    }
    check(reward.options[2].consumableId != "potion_1", "rerolled consumables must change");
    check(reward.options[3].relicId != "relic_1", "rerolled relics must change");
    check(reward.options[3].relicId != "relic_5", "rerolled relics must not be already owned");

    ShopState shop;
    shop.cardRemovalPrice = 75;
    shop.offers = {
        ShopOffer{ShopOfferType::Card, "card_1", 21, false},
        ShopOffer{ShopOfferType::Card, "card_2", 22, false},
        ShopOffer{ShopOfferType::Relic, "relic_1", 150, false},
        ShopOffer{ShopOfferType::Consumable, "potion_1", 25, false},
        ShopOffer{ShopOfferType::CardRemoval, "", 75, false}
    };

    Random shopRandom(23u);
    const ActiveItemRerollResult shopResult = ActiveItemRerollSystem::rerollShop(
        shop,
        run,
        cards,
        relics,
        consumables,
        activeItems,
        20,
        25,
        [](RelicRarity) { return 160; },
        shopRandom
    );

    check(shopResult.cardOffersChanged == 2, "the die must reroll every remaining shop card");
    check(shopResult.relicOffersChanged == 1, "the die must reroll the remaining shop relic");
    check(shopResult.consumableOffersChanged == 1, "the die must reroll the remaining shop consumable");
    check(shop.offers.back().type == ShopOfferType::CardRemoval, "the die must preserve card removal service");
    check(shop.offers.back().price == 75, "the die must preserve card removal pricing");
    check(shop.offers[0].contentId != "card_1" && shop.offers[0].contentId != "card_2", "shop card rerolls must be new");
    check(shop.offers[1].contentId != "card_1" && shop.offers[1].contentId != "card_2", "shop card rerolls must be new");
    check(shop.offers[2].contentId != "relic_1" && shop.offers[2].contentId != "relic_5", "shop relic rerolls must be new and unowned");
    check(shop.offers[3].contentId != "potion_1", "shop consumable rerolls must be new");

    ActiveItemDefinition die;
    die.id = ActiveItemId("reroll_die");
    die.maxCharge = 6;
    die.chargeCost = 4;
    die.startingCharge = 2;
    die.useContexts = {ActiveItemUseContext::Reward, ActiveItemUseContext::Shop, ActiveItemUseContext::Chest};
    die.effects = {{ActiveItemEffectType::RerollOffers, 1}};
    ActiveItemSystem::equip(run, die, 4);
    check(ActiveItemSystem::hasEffect(die, ActiveItemEffectType::RerollOffers), "the die must advertise its scene-owned reroll effect");
    check(ActiveItemSystem::spendCharge(run, die, ActiveItemUseContext::Reward), "a successful reroll must spend charge through the shared active-item system");
    check(run.activeItem.charge == 0, "a die use must consume its full charge");
}

void testActiveItemAcquisition() {
    ActiveItemDatabase items;
    ActiveItemDefinition fieldKit;
    fieldKit.id = ActiveItemId("field_kit");
    fieldKit.maxCharge = 5;
    fieldKit.chargeCost = 3;
    fieldKit.startingCharge = 1;
    fieldKit.shopPrice = 140;
    fieldKit.canAppearInRewards = true;
    fieldKit.canAppearInShop = true;
    items.add(std::move(fieldKit));

    ActiveItemDefinition die;
    die.id = ActiveItemId("reroll_die");
    die.maxCharge = 6;
    die.chargeCost = 4;
    die.startingCharge = 2;
    die.shopPrice = 180;
    die.canAppearInRewards = true;
    die.canAppearInShop = true;
    items.add(std::move(die));

    Random random(41u);
    const std::optional<ActiveItemId> reward = ActiveItemAcquisitionSystem::chooseReward(items, "field_kit", random);
    check(reward.has_value() && reward->value == "reroll_die", "active item rewards must exclude the equipped item");

    RunState run;
    run.activeItem.itemId = "field_kit";
    run.activeItem.charge = 3;
    check(ActiveItemAcquisitionSystem::equipReplacement(run, items, ActiveItemId("reroll_die")), "replacement must equip a valid different item");
    check(run.activeItem.itemId == "reroll_die" && run.activeItem.charge == 2, "replacement must use the new item's starting charge");
    check(run.stats.activeItemsGained == 1 && run.stats.activeItemsReplaced == 1, "replacement must update active item statistics");
}

void testExpandedActiveItemContexts() {
    check(activeItemEffectTypeFromString("skip_enemy_turn") == ActiveItemEffectType::SkipEnemyTurn, "hourglass effect must parse");
    check(activeItemEffectTypeFromString("create_consumable") == ActiveItemEffectType::CreateConsumable, "flask effect must parse");
    check(activeItemEffectTypeFromString("reroll_map_choices") == ActiveItemEffectType::RerollMapChoices, "compass effect must parse");
    check(activeItemEffectTypeFromString("copy_card") == ActiveItemEffectType::CopyCard, "mirror effect must parse");
    check(activeItemEffectTypeFromString("stabilize_stress") == ActiveItemEffectType::StabilizeStress, "stress stabilizer effect must parse");

    ConsumableDatabase consumables;
    ConsumableDefinition potionA;
    potionA.id = ConsumableId("potion_a");
    consumables.add(std::move(potionA));
    ConsumableDefinition potionB;
    potionB.id = ConsumableId("potion_b");
    consumables.add(std::move(potionB));

    RunState run;
    run.maxConsumables = 1;
    Random consumableRandom(71u);
    const std::optional<std::string> created = ActiveItemContextSystem::createRandomConsumable(run, consumables, consumableRandom);
    check(created.has_value(), "the alchemist flask must create a consumable when a slot is free");
    check(run.consumableIds.size() == 1u && run.stats.consumablesGained == 1, "created consumables must enter inventory and statistics");
    check(!ActiveItemContextSystem::createRandomConsumable(run, consumables, consumableRandom).has_value(), "the flask must not overfill consumable slots");

    RunMapNode combat;
    combat.id = 1;
    combat.type = RunMapNodeType::Combat;
    combat.state = RunMapNodeState::Available;
    RunMapNode event;
    event.id = 2;
    event.type = RunMapNodeType::Event;
    event.state = RunMapNodeState::Available;
    RunMapNode elite;
    elite.id = 3;
    elite.type = RunMapNodeType::Elite;
    elite.state = RunMapNodeState::Available;
    RunMapNode locked;
    locked.id = 4;
    locked.type = RunMapNodeType::Shop;
    locked.state = RunMapNodeState::Locked;
    run.map.nodes = {combat, event, elite, locked};

    Random mapRandom(72u);
    const int changed = ActiveItemContextSystem::rerollAvailableMapNodes(run, mapRandom);
    check(changed == 2, "the compass must change every available ordinary room");
    check(run.map.nodes[0].type != RunMapNodeType::Combat && run.map.nodes[1].type != RunMapNodeType::Event, "compass results must differ from the old room types");
    check(run.map.nodes[2].type == RunMapNodeType::Elite, "the compass must preserve elite rooms");
    check(run.map.nodes[3].type == RunMapNodeType::Shop, "the compass must preserve locked future rooms");

    CardDatabase cards;
    CardDefinition shared;
    shared.id = CardId("shared_card");
    cards.add(std::move(shared));
    CardDefinition personal;
    personal.id = CardId("personal_card");
    personal.ownerActorId = "missing_actor";
    cards.add(std::move(personal));

    const std::size_t beforeCards = run.deckCardIds.size();
    check(ActiveItemContextSystem::copyCard(run, cards, CardId("shared_card")), "the mirror must copy an eligible selected card");
    check(run.deckCardIds.size() == beforeCards + 1u && run.deckCardIds.back().value == "shared_card", "the mirrored card must be appended to the deck");
    check(!ActiveItemContextSystem::copyCard(run, cards, CardId("personal_card")), "the mirror must respect actor card eligibility");
}

void testStressDamageAndConversions() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
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

    CombatState state;
    state.players.push_back(makeEntity(81, EntityType::Player, "lost_psychopath", 50));
    state.enemies.push_back(makeEntity(82, EntityType::Enemy, "stress_attacker", 50));
    state.players.front().maxStress = StressRules::MaximumStress;
    state.players.front().block = 6;

    EffectDefinition attack;
    attack.type = EffectType::Damage;
    attack.target = EffectTarget::AllAllies;
    attack.value = EffectValue::fixed(9);
    EffectContext enemyContext;
    enemyContext.source = state.enemies.front().id;
    enemyContext.usesActorStats = true;
    effectSystem.applyEffect(state, attack, enemyContext);
    check(state.players.front().health.current() == 47, "enemy damage must reach HP after block");
    check(state.players.front().stress == 1, "three HP damage must generate one stress");

    state.players.front().block = 20;
    effectSystem.applyEffect(state, attack, enemyContext);
    check(state.players.front().stress == 1, "fully blocked damage must not generate stress");

    state.players.front().stress = 160;
    EffectDefinition conversion;
    conversion.type = EffectType::SpendStressDamage;
    conversion.target = EffectTarget::SingleEnemy;
    conversion.value = EffectValue::fixed(20);
    conversion.outputAmount = 15;
    EffectContext playerContext;
    playerContext.source = state.players.front().id;
    playerContext.usesActorStats = true;
    playerContext.explicitEnemyTarget = state.enemies.front().id;
    effectSystem.applyEffect(state, conversion, playerContext);
    check(state.players.front().stress == 140, "stress conversion must spend its fixed cost");
    check(state.enemies.front().health.current() == 32, "stress conversion damage must use the post-spend stress band bonus");

    EffectDefinition prime;
    prime.type = EffectType::PrimeStressBreakdown;
    prime.target = EffectTarget::AllAllies;
    prime.statusId = "energy";
    effectSystem.applyEffect(state, prime, enemyContext);
    check(state.primedStressBreakdown(state.players.front().id) == std::optional<std::string>("energy"),
          "enemy effects must be able to prime a specific future breakdown");
}

void testStressCardPlayValidation() {
    CombatState state;
    state.phase = CombatPhase::PlayerTurn;
    state.players.push_back(makeEntity(80, EntityType::Player, "lost_psychopath", 50));
    state.players.front().stress = 29;
    state.resources.setMaxEnergy(3);
    state.resources.resetEnergy();

    CardInstance instance;
    instance.instanceId = CardInstanceId{9001};
    instance.definitionId = CardId("stress_conversion_test");
    state.hand.add(instance);

    CardDefinition card;
    card.id = instance.definitionId;
    card.ownerActorId = "lost_psychopath";
    card.energyCost = 0;
    card.stressCost = 5;
    EffectDefinition conversion;
    conversion.type = EffectType::SpendStressEnergy;
    conversion.target = EffectTarget::Self;
    conversion.value = EffectValue::fixed(15);
    conversion.outputAmount = 1;
    conversion.repeatCount = 2;
    card.effects.push_back(conversion);

    CardPlayValidator validator;
    const CardPlayValidationResult insufficient = validator.validate(state, card, instance, state.players.front().id);
    check(!insufficient.valid && insufficient.failureReason == CardPlayFailureReason::NotEnoughStress,
          "repeated stress conversions must validate their full stress cost");

    check(CardStressCost::totalCost(card) == 35,
          "direct card stress cost and conversion costs must be paid atomically");

    state.players.front().stress = 34;
    const CardPlayValidationResult directCostStillMissing =
        validator.validate(state, card, instance, state.players.front().id);
    check(!directCostStillMissing.valid &&
              directCostStillMissing.failureReason == CardPlayFailureReason::NotEnoughStress,
          "direct card stress cost must be included in validation");

    state.players.front().stress = 35;
    const CardPlayValidationResult enough = validator.validate(state, card, instance, state.players.front().id);
    check(enough.valid, "a card must become playable when its full stress cost is available");
}

void testStressEconomyRules() {
    check(StressEconomyRules::stressFromHpDamage(0) == 0, "blocked damage must not generate stress");
    check(StressEconomyRules::stressFromHpDamage(1) == 1, "one HP damage must generate one stress");
    check(StressEconomyRules::stressFromHpDamage(3) == 1, "three HP damage must generate one stress");
    check(StressEconomyRules::stressFromHpDamage(4) == 2, "stress gain must round HP damage upward by thirds");
    check(StressEconomyRules::stressFromHpDamage(100) == 12, "stress from a single hit must be capped");
    check(StressEconomyRules::RestCalmAmount == 60, "rest calm must remove sixty stress");

    check(effectTypeFromString("spend_stress_damage") == EffectType::SpendStressDamage, "stress damage conversion must parse");
    check(effectTypeFromString("spend_stress_block") == EffectType::SpendStressBlock, "stress block conversion must parse");
    check(effectTypeFromString("spend_stress_energy") == EffectType::SpendStressEnergy, "stress energy conversion must parse");
    check(effectTypeFromString("spend_stress_draw") == EffectType::SpendStressDraw, "stress draw conversion must parse");
    check(isStressConversionEffect(EffectType::SpendStressDamage), "stress damage must be classified as a conversion");
    check(!isStressConversionEffect(EffectType::GainStress), "ordinary stress gain must not be classified as a conversion");

    RelicDefinition genericRelic;
    genericRelic.mechanicId = "default";
    check(RewardPoolRules::matchesRunMechanic(genericRelic, "stress_psychopath"),
          "generic relics must remain available to every run mechanic");

    RelicDefinition psychopathRelic;
    psychopathRelic.mechanicId = "stress_psychopath";
    check(RewardPoolRules::matchesRunMechanic(psychopathRelic, "stress_psychopath"),
          "psychopath relics must be available to the psychopath");
    check(!RewardPoolRules::matchesRunMechanic(psychopathRelic, "replicant_drones"),
          "psychopath relics must not leak into ordinary stress economies");
}

void testStressBandsAndPsychopathEffects() {
    check(StressRules::bandForStress(0) == StressRules::StressBand::Calm, "0 stress must be Calm");
    check(StressRules::bandForStress(39) == StressRules::StressBand::Calm, "39 stress must remain Calm");
    check(StressRules::bandForStress(40) == StressRules::StressBand::Tense, "40 stress must enter Tense");
    check(StressRules::bandForStress(80) == StressRules::StressBand::Pressured, "80 stress must enter Pressured");
    check(StressRules::bandForStress(120) == StressRules::StressBand::Panicked, "120 stress must enter Panicked");
    check(StressRules::bandForStress(160) == StressRules::StressBand::Breaking, "160 stress must enter Breaking");
    check(StressRules::bandForStress(200) == StressRules::StressBand::Collapsed, "200 stress must collapse");

    check(StressPsychopathRules::damageBonusForStress(39) == 0, "Calm stress must not add damage");
    check(StressPsychopathRules::damageBonusForStress(40) == 1, "Tense stress must add one damage");
    check(StressPsychopathRules::damageBonusForStress(80) == 2, "Pressured stress must add two damage");
    check(StressPsychopathRules::damageBonusForStress(120) == 3, "Panicked stress must add three damage");
    check(StressPsychopathRules::damageBonusForStress(160) == 4, "Breaking stress must add four damage");
    check(StressPsychopathRules::startTurnEnergyBonus(159) == 0, "energy surge must not start before Breaking");
    check(StressPsychopathRules::startTurnEnergyBonus(160) == 1, "Breaking must grant one start-of-turn energy");

    check(StressPsychopathRules::breakdownSeverity(120, true, false) == 1, "Panicked breakdown must use severity one");
    check(StressPsychopathRules::breakdownSeverity(160, true, false) == 2, "Breaking breakdown must use severity two");
    check(StressPsychopathRules::startTurnDiscardCount(160, false, false) == 1, "unresolved Breaking stress must retain its mild discard");
    check(StressPsychopathRules::startTurnDiscardCount(160, false, true) == 0, "resolve must suppress unresolved Breaking discard");
    check(StressPsychopathRules::startTurnDiscardCount(160, true, false) == 0, "a full breakdown must use a random penalty instead of fixed discard");

    CombatEntity actor = makeEntity(99, EntityType::Player, "lost_psychopath", 40);
    actor.stress = 79;
    actor.maxStress = StressRules::MaximumStress;
    const StressRules::StressAdjustmentResult bandResult = StressRules::applyDelta(actor, 1, nullptr);
    check(bandResult.bandChanged, "crossing 80 stress must report a band change");
    check(bandResult.beforeBand == StressRules::StressBand::Tense, "band transition must retain the previous band");
    check(bandResult.afterBand == StressRules::StressBand::Pressured, "band transition must report the new band");

    actor.stress = 99;
    actor.resolveCheckTriggered = false;
    actor.traitIds.clear();
    const StressRules::StressAdjustmentResult resolveResult = StressRules::applyDelta(actor, 1, nullptr, true);
    check(resolveResult.resolveCheckTriggered, "the psychopath crossing 100 stress must trigger the resolve check");
    check(resolveResult.resolveOutcome == StressRules::ResolveOutcome::Breakdown, "a resolve check without RNG must take the deterministic breakdown branch");
    check(StressRules::hasTrait(actor, StressRules::BreakdownTraitId), "breakdown outcome must add the breakdown trait");

    CombatEntity ordinaryActor = makeEntity(100, EntityType::Player, "herbalist", 40);
    ordinaryActor.stress = 99;
    ordinaryActor.maxStress = StressRules::MaximumStress;
    ordinaryActor.resolveCheckTriggered = true;
    ordinaryActor.traitIds = {StressRules::BreakdownTraitId, StressRules::ResolveTraitId};
    const StressRules::StressAdjustmentResult ordinaryResult =
        StressRules::applyDelta(ordinaryActor, 1, nullptr, false);
    check(!ordinaryResult.resolveCheckTriggered,
          "ordinary characters must not receive the psychopath resolve check");
    check(ordinaryResult.resolveOutcome == StressRules::ResolveOutcome::None,
          "ordinary characters must not receive positive or negative stress traits");
    check(!StressRules::hasTrait(ordinaryActor, StressRules::BreakdownTraitId) &&
              !StressRules::hasTrait(ordinaryActor, StressRules::ResolveTraitId),
          "legacy psychopath stress traits must be cleared from ordinary characters");
    check(ordinaryActor.stress == 100 && ordinaryActor.isAlive(),
          "ordinary characters must keep stress as a resource until the lethal cap");
}

void fillTestDeck(CombatState& state, CardDatabase& cards, const int count) {
    for (int index = 0; index < count; ++index) {
        const std::string id = "stress_test_card_" + std::to_string(index);
        CardDefinition definition;
        definition.id = CardId(id);
        cards.add(std::move(definition));

        CardInstance instance;
        instance.instanceId = CardInstanceId{static_cast<std::uint64_t>(index + 1)};
        instance.definitionId = CardId(id);
        state.deck.drawPile.addTop(std::move(instance));
    }
}

void addStressStatusCards(CardDatabase& cards) {
    CardDefinition wound;
    wound.id = CardId("wound_status");
    wound.type = CardType::Status;
    wound.keywords.push_back(CardKeyword::Unplayable);
    cards.add(std::move(wound));

    CardDefinition burn;
    burn.id = CardId("burn_status");
    burn.type = CardType::Status;
    burn.keywords.push_back(CardKeyword::Unplayable);
    cards.add(std::move(burn));
}

void testStressStartOfTurnEffects() {
    DrawSystem drawSystem;
    Random random(991u);

    CardDatabase resolveCards;
    CombatState resolveState;
    resolveState.players.push_back(makeEntity(2, EntityType::Player, "lost_psychopath", 50));
    resolveState.players.front().stress = 160;
    resolveState.players.front().traitIds.push_back(StressRules::ResolveTraitId);
    resolveState.resources.setMaxEnergy(3);
    fillTestDeck(resolveState, resolveCards, 5);
    PlayerTurnSystem resolveTurns(drawSystem, resolveCards);

    resolveTurns.startTurn(resolveState, 5, random);
    check(resolveState.resources.energy() == 4, "resolve must preserve the Breaking energy bonus");
    check(resolveState.hand.size() == 5u, "resolve must prevent high-stress penalties");

    CardDatabase cards;
    addStressStatusCards(cards);
    CardDefinition attack;
    attack.id = CardId("stress_attack");
    attack.type = CardType::Attack;
    attack.energyCost = 1;
    cards.add(std::move(attack));
    PlayerTurnSystem playerTurns(drawSystem, cards);

    CombatState discardState;
    discardState.players.push_back(makeEntity(10, EntityType::Player, "lost_psychopath", 50));
    discardState.players.front().stress = 160;
    for (int index = 0; index < 4; ++index) {
        CardInstance card;
        card.instanceId = CardInstanceId{static_cast<std::uint64_t>(100 + index)};
        card.definitionId = CardId("stress_attack");
        discardState.hand.add(std::move(card));
    }
    playerTurns.applyStressBreakdown(discardState, discardState.players.front().id, StressBreakdownRules::BreakdownType::Discard, random);
    check(discardState.hand.size() == 2u, "severity-two discard breakdown must remove two cards");
    check(discardState.deck.discardPile.size() == 2u, "discard breakdown cards must enter the discard pile");

    CombatState energyState;
    energyState.players.push_back(makeEntity(11, EntityType::Player, "lost_psychopath", 50));
    energyState.players.front().stress = 160;
    energyState.resources.setMaxEnergy(3);
    energyState.resources.resetEnergy();
    playerTurns.applyStressBreakdown(energyState, energyState.players.front().id, StressBreakdownRules::BreakdownType::EnergyCrash, random);
    check(energyState.resources.energy() == 1, "severity-two energy crash must remove two energy");

    CombatState statusState;
    statusState.players.push_back(makeEntity(12, EntityType::Player, "lost_psychopath", 50));
    statusState.players.front().stress = 160;
    playerTurns.applyStressBreakdown(statusState, statusState.players.front().id, StressBreakdownRules::BreakdownType::IntrusiveThoughts, random);
    check(statusState.hand.size() == 2u, "severity-two intrusive thoughts must add two status cards");
    check(statusState.hand.cards()[0].temporary && statusState.hand.cards()[1].temporary, "breakdown status cards must be combat-temporary");

    CombatState costState;
    costState.players.push_back(makeEntity(13, EntityType::Player, "lost_psychopath", 50));
    costState.players.front().stress = 160;
    playerTurns.applyStressBreakdown(costState, costState.players.front().id, StressBreakdownRules::BreakdownType::CostSpike, random);
    check(CardCost::effectiveEnergyCost(costState, costState.players.front().id, cards.get(CardId("stress_attack"))) == 3,
          "severity-two cost spike must add two energy to card costs");

    CombatState frenzyState;
    frenzyState.players.push_back(makeEntity(14, EntityType::Player, "lost_psychopath", 50));
    frenzyState.players.front().stress = 160;
    frenzyState.enemies.push_back(makeEntity(15, EntityType::Enemy, "target", 50));
    frenzyState.resources.setMaxEnergy(3);
    frenzyState.resources.resetEnergy();
    CardInstance frenzyCard;
    frenzyCard.instanceId = CardInstanceId{500};
    frenzyCard.definitionId = CardId("stress_attack");
    frenzyState.hand.add(std::move(frenzyCard));
    playerTurns.applyStressBreakdown(frenzyState, frenzyState.players.front().id, StressBreakdownRules::BreakdownType::Frenzy, random);
    check(frenzyState.pendingForcedCardPlay.has_value(), "frenzy breakdown must queue an uncontrolled attack");
    check(frenzyState.pendingForcedCardPlay->cardInstanceId == CardInstanceId{500}, "frenzy must queue the eligible attack card");
}



void testStressBreakdownPrimingAndGuard() {
    DrawSystem drawSystem;
    CardDatabase cards;
    addStressStatusCards(cards);
    Random random(1201u);

    std::vector<GameEvent> events;
    GameEventBus eventBus;
    eventBus.subscribe([&events](const GameEvent& event) { events.push_back(event); });
    PlayerTurnSystem turns(drawSystem, cards, &eventBus);

    CombatState primedState;
    primedState.players.push_back(makeEntity(301, EntityType::Player, "lost_psychopath", 50));
    primedState.players.front().stress = 160;
    primedState.players.front().traitIds.push_back(StressRules::BreakdownTraitId);
    primedState.resources.setMaxEnergy(3);
    primedState.primeStressBreakdown(primedState.players.front().id, "energy");

    turns.startTurn(primedState, 0, random);
    check(primedState.resources.energy() == 2, "a primed severe energy crash must remove two energy after the Breaking bonus");
    check(!primedState.primedStressBreakdown(primedState.players.front().id).has_value(), "a primed breakdown must be consumed");
    check(events.size() == 1u && events.front().type == GameEventType::StressBreakdownTriggered,
          "stress breakdowns must emit a gameplay event");
    check(events.front().breakdownType == "energy" && events.front().breakdownSeverity == 2,
          "breakdown events must expose their type and severity");

    CombatState guardedState;
    guardedState.players.push_back(makeEntity(302, EntityType::Player, "lost_psychopath", 50));
    guardedState.players.front().stress = 160;
    guardedState.players.front().traitIds.push_back(StressRules::BreakdownTraitId);
    guardedState.resources.setMaxEnergy(3);
    guardedState.primeStressBreakdown(guardedState.players.front().id, "frenzy");
    guardedState.armStressBreakdownGuard(guardedState.players.front().id);
    const std::size_t beforeEvents = events.size();

    turns.startTurn(guardedState, 0, random);
    check(!guardedState.hasStressBreakdownGuard(guardedState.players.front().id), "breakdown guard must be consumed by the next eligible breakdown");
    check(!guardedState.primedStressBreakdown(guardedState.players.front().id).has_value(), "preventing a breakdown must also clear enemy priming");
    check(!guardedState.pendingForcedCardPlay.has_value(), "a guarded frenzy must not force a card play");
    check(events.size() == beforeEvents, "prevented breakdowns must not trigger breakdown relic events");
}

void testStressEventRequirements() {
    RunState run;
    run.archetypeMechanicId = "stress_psychopath";
    RunActorState actor;
    actor.definitionId = "lost_psychopath";
    actor.stress = 130;
    actor.traitIds.push_back(StressRules::BreakdownTraitId);
    run.actorStates.push_back(actor);

    RunEventChoiceRequirements available;
    available.minStress = 120;
    available.requiredTraitIds.push_back(StressRules::BreakdownTraitId);
    check(evaluateRunEventChoiceRequirements(available, run).available,
          "stress events must unlock choices from stress thresholds and traits");

    RunEventChoiceRequirements mechanicLocked;
    mechanicLocked.requiredMechanicId = "stress_psychopath";
    check(evaluateRunEventChoiceRequirements(mechanicLocked, run).available,
          "psychopath events must be available to the matching run mechanic");
    run.archetypeMechanicId = "herbalist_brews";
    const RunEventChoiceAvailability wrongMechanic =
        evaluateRunEventChoiceRequirements(mechanicLocked, run);
    check(!wrongMechanic.available &&
              wrongMechanic.reasons.front().type == RunEventChoiceBlockReasonType::WrongRunMechanic,
          "psychopath events must not leak into ordinary stress economies");
    run.archetypeMechanicId = "stress_psychopath";

    RunEventChoiceRequirements tooCalm;
    tooCalm.maxStress = 100;
    const RunEventChoiceAvailability blocked = evaluateRunEventChoiceRequirements(tooCalm, run);
    check(!blocked.available && blocked.reasons.front().type == RunEventChoiceBlockReasonType::StressTooHigh,
          "stress events must block choices above their maximum stress threshold");

    run.eventFlags.push_back("chain.test.started");
    RunEventChoiceRequirements flagRequirements;
    flagRequirements.requiredEventFlags.push_back("chain.test.started");
    flagRequirements.forbiddenEventFlags.push_back("chain.test.finished");
    check(evaluateRunEventChoiceRequirements(flagRequirements, run).available,
          "event flags must unlock later chain steps");

    flagRequirements.requiredEventFlags.push_back("chain.test.missing");
    const RunEventChoiceAvailability missingFlag = evaluateRunEventChoiceRequirements(flagRequirements, run);
    check(!missingFlag.available &&
              std::any_of(missingFlag.reasons.begin(), missingFlag.reasons.end(), [](const RunEventChoiceBlockReason& reason) {
                  return reason.type == RunEventChoiceBlockReasonType::MissingRequiredEventFlag;
              }),
          "missing event flags must block chain choices");

    RunEventDefinition ordinaryEvent;
    ordinaryEvent.id = "ordinary";
    RunEventDefinition chainEvent;
    chainEvent.id = "chain";
    chainEvent.requirements.requiredEventFlags.push_back("chain.test.next");
    std::vector<const RunEventDefinition*> pool{&ordinaryEvent, &chainEvent};
    check(availableRunEvents(pool, run).size() == 1,
          "event selection must omit chain steps whose flags are missing");
    run.eventFlags.push_back("chain.test.next");
    check(availableRunEvents(pool, run).size() == 2,
          "event selection must include chain steps after their flag is set");
}

void testEnemyIntentPresentation() {
    EnemyIntent intent;
    EnemyIntentEffectSummary partyAttack;
    partyAttack.type = EffectType::Damage;
    partyAttack.target = EffectTarget::AllAllies;
    intent.effectSummaries.push_back(partyAttack);

    EnemyIntentPresentation presentation = summarizeEnemyIntent(intent);
    check(presentation.affectsAllPlayers, "all-allies enemy effects must be presented as party-wide intents");
    check(!presentation.affectsEnemyTeam, "party-wide attacks must not be presented as enemy-team effects");
    check(presentation.affectsMultipleTargets, "party-wide attacks must be marked as multi-target intents");

    EnemyIntentEffectSummary teamBlock;
    teamBlock.type = EffectType::Block;
    teamBlock.target = EffectTarget::AllEnemies;
    intent.effectSummaries.push_back(teamBlock);

    presentation = summarizeEnemyIntent(intent);
    check(presentation.affectsAllPlayers, "mixed intents must retain their party-wide scope");
    check(presentation.affectsEnemyTeam, "all-enemies enemy effects must be presented as enemy-team effects");
}

void testBossPhaseTransitionsAndSummons() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
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
    minion.id = EnemyId("phase_minion");
    minion.nameTextId = TextId("enemy.phase_minion.name");
    minion.maxHp = 20;
    minion.role = EnemyRole::Minion;
    minion.actions.push_back(makeEnemyAction("minion_attack"));

    EnemyDefinition boss;
    boss.id = EnemyId("phase_boss");
    boss.nameTextId = TextId("enemy.phase_boss.name");
    boss.maxHp = 100;
    boss.role = EnemyRole::Boss;
    boss.actions.push_back(makeEnemyAction("opening_attack"));
    boss.actions.push_back(makeEnemyAction("enraged_attack"));

    EnemyPhaseDefinition opening;
    opening.id = "opening";
    opening.nameTextId = TextId("enemy.phase.phase_boss.opening");
    opening.activateBelowHpPercent = 100;
    opening.actionIds = {"opening_attack"};
    EffectDefinition openingBlock;
    openingBlock.type = EffectType::Block;
    openingBlock.target = EffectTarget::Self;
    openingBlock.value = EffectValue::fixed(5);
    opening.onEnterEffects.push_back(openingBlock);

    EnemyPhaseDefinition enraged;
    enraged.id = "enraged";
    enraged.nameTextId = TextId("enemy.phase.phase_boss.enraged");
    enraged.activateBelowHpPercent = 50;
    enraged.actionIds = {"enraged_attack"};
    enraged.summonEnemyIds = {"phase_minion", "phase_minion", "phase_minion"};
    enraged.maximumAliveEnemies = 3;
    EffectDefinition strength;
    strength.type = EffectType::ApplyStatus;
    strength.target = EffectTarget::Self;
    strength.value = EffectValue::fixed(2);
    strength.statusId = "strength";
    enraged.onEnterEffects.push_back(strength);
    EffectDefinition stress;
    stress.type = EffectType::GainStress;
    stress.target = EffectTarget::AllAllies;
    stress.value = EffectValue::fixed(3);
    enraged.playerTurnEffects.push_back(stress);
    boss.phases = {opening, enraged};

    EnemyDatabase enemies;
    enemies.add(std::move(minion));
    enemies.add(boss);

    CombatState state;
    state.turn = 1;
    state.enemyHpMultiplier = 1.5f;
    state.players.push_back(makeEntity(1, EntityType::Player, "tester", 50));
    state.enemies.push_back(makeEntity(2, EntityType::Enemy, "phase_boss", 100));

    BossPhaseSystem phases(enemies, effectSystem);
    EnemyMoveSelector selector(modifiers);
    Random random(91u);

    check(phases.synchronizePhases(state, random), "the opening boss phase must activate at combat start");
    check(state.enemies.front().block == 5, "opening phase effects must apply once");
    check(state.enemyAiStates.front().activePhaseId == "opening", "combat state must remember the active boss phase");
    check(selector.selectAction(state, enemies.get(EnemyId("phase_boss")), state.enemies.front().id, random).id == "opening_attack",
          "boss AI must use only actions from the active phase");

    state.enemies.front().health.setCurrent(40);
    check(phases.synchronizePhases(state, random), "crossing a boss HP threshold must activate the next phase");
    check(state.enemies.front().statuses.stacks("strength") == 2, "phase-entry statuses must affect the boss");
    check(state.aliveEnemyCount() == 3u, "phase summons must respect the configured three-enemy cap");
    check(state.enemies[1].health.maximum() == 30, "summoned enemies must inherit the run HP multiplier");
    check(selector.selectAction(state, enemies.get(EnemyId("phase_boss")), state.enemies.front().id, random).id == "enraged_attack",
          "boss AI must switch action pools after a phase transition");

    phases.applyPlayerTurnEffects(state, random);
    phases.applyPlayerTurnEffects(state, random);
    check(state.players.front().stress == 3, "arena effects must trigger at most once per player turn");
    ++state.turn;
    phases.applyPlayerTurnEffects(state, random);
    check(state.players.front().stress == 6, "arena effects must trigger again on the next turn");

    state.enemies.front().health.setCurrent(100);
    check(!phases.synchronizePhases(state, random), "boss phases must never regress after healing");
    check(state.enemyAiStates.front().activePhaseId == "enraged", "healing must not restore an earlier action pool");
}


void testCardLifecycleAndOpeningHand() {
    DrawSystem drawSystem;
    Random random(1701u);

    Hand limitedHand;
    for (std::uint64_t id = 1; id <= Hand::MaximumSize; ++id) {
        CardInstance card;
        card.instanceId = CardInstanceId{id};
        card.definitionId = CardId("limit_card");
        limitedHand.add(std::move(card));
    }
    check(limitedHand.full(), "a hand must be full at ten cards");
    check(limitedHand.remainingCapacity() == 0u, "a full hand must report no remaining capacity");

    CardInstance overflow;
    overflow.instanceId = CardInstanceId{99};
    overflow.definitionId = CardId("overflow");
    check(!limitedHand.tryAdd(std::move(overflow)), "cards beyond the ten-card hand limit must be rejected");

    Deck cappedDeck;
    for (std::uint64_t id = 100; id < 103; ++id) {
        CardInstance card;
        card.instanceId = CardInstanceId{id};
        card.definitionId = CardId("draw_card");
        cappedDeck.drawPile.addTop(std::move(card));
    }
    check(drawSystem.drawCards(cappedDeck, limitedHand, 3, random) == 0u,
          "drawing with a full hand must not remove cards from the draw pile");
    check(cappedDeck.drawPile.size() == 3u, "blocked draws must leave cards in the draw pile");

    CardDatabase openingCards;
    for (int index = 0; index < 3; ++index) {
        CardDefinition innate;
        innate.id = CardId("innate_" + std::to_string(index));
        innate.keywords.push_back(CardKeyword::Innate);
        openingCards.add(std::move(innate));
    }
    for (int index = 0; index < 5; ++index) {
        CardDefinition normal;
        normal.id = CardId("normal_" + std::to_string(index));
        openingCards.add(std::move(normal));
    }

    Deck openingDeck;
    std::uint64_t nextId = 200;
    for (const CardDefinition* definition : openingCards.all()) {
        CardInstance card;
        card.instanceId = CardInstanceId{nextId++};
        card.definitionId = definition->id;
        openingDeck.drawPile.addTop(std::move(card));
    }

    Hand openingHand;
    drawSystem.drawOpeningHand(openingDeck, openingHand, openingCards, 5, random);
    check(openingHand.size() == 5u, "the opening hand must be filled to the normal hand size after innate cards");
    for (int index = 0; index < 3; ++index) {
        const CardId expected("innate_" + std::to_string(index));
        check(std::any_of(openingHand.cards().begin(), openingHand.cards().end(), [&expected](const CardInstance& card) {
            return card.definitionId == expected;
        }), "every innate card must appear in the opening hand");
    }

    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    EnergySystem energySystem;
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

    CardDatabase lifecycleCards;
    CardDefinition burn;
    burn.id = CardId("burn_test");
    burn.type = CardType::Status;
    burn.keywords = {CardKeyword::Unplayable, CardKeyword::Ethereal};
    EffectDefinition burnDamage;
    burnDamage.type = EffectType::Damage;
    burnDamage.target = EffectTarget::Self;
    burnDamage.value = EffectValue::fixed(2);
    burn.effects.push_back(burnDamage);
    lifecycleCards.add(std::move(burn));

    CardDefinition wound;
    wound.id = CardId("wound_test");
    wound.type = CardType::Status;
    wound.keywords = {CardKeyword::Unplayable};
    lifecycleCards.add(std::move(wound));

    CardDefinition etherealRetain;
    etherealRetain.id = CardId("ethereal_retain_test");
    etherealRetain.type = CardType::Skill;
    etherealRetain.keywords = {CardKeyword::Retain, CardKeyword::Ethereal};
    lifecycleCards.add(std::move(etherealRetain));

    CardDefinition temporaryPlayable;
    temporaryPlayable.id = CardId("temporary_play_test");
    temporaryPlayable.type = CardType::Skill;
    temporaryPlayable.energyCost = 0;
    lifecycleCards.add(std::move(temporaryPlayable));

    CombatState endState;
    endState.phase = CombatPhase::PlayerTurn;
    endState.players.push_back(makeEntity(500, EntityType::Player, "tester", 20));
    endState.activePlayerIndex = 0;

    const auto addToEndHand = [&endState](const std::uint64_t id, const char* definitionId, const bool temporary = false) {
        CardInstance card;
        card.instanceId = CardInstanceId{id};
        card.definitionId = CardId(definitionId);
        card.temporary = temporary;
        endState.hand.add(std::move(card));
    };
    addToEndHand(501, "burn_test");
    addToEndHand(502, "wound_test");
    addToEndHand(503, "ethereal_retain_test");
    addToEndHand(504, "temporary_play_test", true);

    PlayerTurnSystem turns(drawSystem, lifecycleCards, nullptr, &effectSystem);
    turns.endTurn(endState, random);
    check(endState.players.front().health.current() == 18,
          "an unplayable burn status must deal its end-of-turn damage");
    check(endState.deck.exhaustPile.size() == 3u,
          "burn, ethereal-retain and unplayed temporary cards must leave the combat cycle");
    check(endState.deck.discardPile.size() == 1u &&
          endState.deck.discardPile.cards().front().definitionId == CardId("wound_test"),
          "a wound must clog the hand and then enter the discard pile without dealing hidden damage");

    CombatState playState;
    playState.phase = CombatPhase::PlayerTurn;
    playState.players.push_back(makeEntity(600, EntityType::Player, "tester", 20));
    playState.activePlayerIndex = 0;
    playState.resources.setMaxEnergy(playState.players.front().id, 3);
    playState.resources.resetEnergy();
    CardInstance temporaryCard;
    temporaryCard.instanceId = CardInstanceId{601};
    temporaryCard.definitionId = CardId("temporary_play_test");
    temporaryCard.temporary = true;
    playState.hand.add(std::move(temporaryCard));

    CardPlayValidator validator;
    CardPlaySystem cardPlay(lifecycleCards, validator, energySystem, effectSystem);
    const PlayCardResult played = cardPlay.playCard(
        playState,
        PlayCardRequest{CardInstanceId{601}, playState.players.front().id, std::nullopt},
        random
    );
    check(played.played, "a playable temporary card must still be playable");
    check(playState.deck.exhaustPile.size() == 1u && playState.deck.discardPile.empty(),
          "a played temporary card must exhaust instead of entering the discard pile");
}

void testDataDrivenStatusDefinitions() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();

    StatusDefinition customPower;
    customPower.id = StatusId("custom_power");
    customPower.nameTextId = TextId("status.custom_power.name");
    customPower.descriptionTextId = TextId("status.custom_power.description");
    StatusModifierDefinition customModifier;
    customModifier.effectType = EffectType::Damage;
    customModifier.entity = StatusModifierEntity::Source;
    customModifier.operation = StatusModifierOperation::AddPerStack;
    customModifier.addAmount = 3;
    customModifier.descriptionTextId = TextId("modifier.custom_power");
    customPower.modifiers.push_back(customModifier);
    statuses.add(std::move(customPower));

    StatusDefinition firstMode;
    firstMode.id = StatusId("mode_one");
    firstMode.nameTextId = TextId("status.mode_one.name");
    firstMode.descriptionTextId = TextId("status.mode_one.description");
    firstMode.exclusiveGroup = "test_mode";
    statuses.add(std::move(firstMode));

    StatusDefinition secondMode;
    secondMode.id = StatusId("mode_two");
    secondMode.nameTextId = TextId("status.mode_two.name");
    secondMode.descriptionTextId = TextId("status.mode_two.description");
    secondMode.exclusiveGroup = "test_mode";
    statuses.add(std::move(secondMode));

    StatusDefinition bleeding;
    bleeding.id = StatusId("test_bleeding");
    bleeding.nameTextId = TextId("status.test_bleeding.name");
    bleeding.descriptionTextId = TextId("status.test_bleeding.description");
    bleeding.durationRule = StatusDurationRule::Custom;
    StatusTriggerDefinition trigger;
    trigger.effect = StatusTriggeredEffect::DamageHp;
    trigger.valuePerStack = 2;
    trigger.removeStacks = 2;
    bleeding.triggers.push_back(trigger);
    statuses.add(std::move(bleeding));

    ModifierSystem modifiers(localization, statuses);
    DamageSystem damage(modifiers);
    StatusSystem statusSystem(statuses);

    CombatState state;
    state.players.push_back(makeEntity(700, EntityType::Player, "tester", 40));
    state.enemies.push_back(makeEntity(701, EntityType::Enemy, "target", 40));
    state.players.front().statuses.set("custom_power", 2);

    const DamageResult result = damage.dealDamage(
        state,
        state.players.front().id,
        state.enemies.front().id,
        10,
        CardId("test.data_driven"),
        DiceCorruption{},
        true
    );
    check(result.modifiedDamage == 16,
          "a status unknown to C++ must modify card damage entirely from its definition");

    statusSystem.applyStatus(state, state.players.front().id, "mode_one", 1);
    statusSystem.applyStatus(state, state.players.front().id, "mode_two", 1);
    check(!state.players.front().statuses.has("mode_one") && state.players.front().statuses.has("mode_two"),
          "exclusive status groups must be enforced from data instead of stance ids");

    statusSystem.applyStatus(state, state.enemies.front().id, "test_bleeding", 3, state.players.front().id);
    const int hpBefore = state.enemies.front().health.current();
    statusSystem.onTurnEndedForEntity(state, state.enemies.front().id);
    check(state.enemies.front().health.current() == hpBefore - 6,
          "a custom end-turn trigger must calculate its value from status stacks");
    check(state.enemies.front().statuses.stacks("test_bleeding") == 1,
          "a custom end-turn trigger must remove the configured number of stacks");
}

void testCombatTelemetryCounters() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damage(modifiers);
    BlockSystem block(modifiers);

    CombatState state;
    state.phase = CombatPhase::PlayerTurn;
    state.players.push_back(makeEntity(800, EntityType::Player, "telemetry_player", 40));
    state.enemies.push_back(makeEntity(801, EntityType::Enemy, "telemetry_enemy", 40));

    block.gainBlock(
        state,
        state.players.front().id,
        state.players.front().id,
        4,
        CardId("telemetry_block"),
        DiceCorruption{},
        true
    );
    check(state.telemetry.blockGainedByPlayers == 4,
          "player block gained must be recorded in combat telemetry");

    damage.dealDamage(
        state,
        state.players.front().id,
        state.enemies.front().id,
        7,
        CardId("telemetry_attack"),
        DiceCorruption{},
        true
    );
    check(state.telemetry.damageDealtToEnemies == 7 && state.telemetry.maximumSingleHit == 7,
          "enemy health damage and maximum hit must be recorded");

    damage.dealDamage(
        state,
        state.enemies.front().id,
        state.players.front().id,
        10,
        CardId{},
        DiceCorruption{},
        true
    );
    check(state.telemetry.damageTakenByPlayers == 6,
          "only health damage after block must count as damage taken");
    check(state.telemetry.damageBlockedByPlayers == 4,
          "prevented damage must be recorded separately");

    CardDatabase cards;
    CardDefinition attack;
    attack.id = CardId("telemetry_card");
    attack.type = CardType::Attack;
    attack.energyCost = 1;
    EffectDefinition attackEffect;
    attackEffect.type = EffectType::Damage;
    attackEffect.target = EffectTarget::SingleEnemy;
    attackEffect.value = EffectValue::fixed(3);
    attack.effects.push_back(attackEffect);
    cards.add(std::move(attack));

    CardInstance instance;
    instance.instanceId = CardInstanceId{802};
    instance.definitionId = CardId("telemetry_card");
    state.hand.add(std::move(instance));
    state.resources.setMaxEnergy(state.players.front().id, 3);
    state.resources.resetEnergy();

    EnergySystem energy;
    EffectResolver resolver;
    DrawSystem draw;
    Targeting targeting;
    DroneDatabase drones;
    StatusSystem statusSystem(statuses);
    DroneSystem droneSystem(
        drones,
        resolver,
        targeting,
        damage,
        block,
        energy,
        draw,
        statusSystem
    );
    EffectSystem effects(
        resolver,
        targeting,
        damage,
        block,
        energy,
        draw,
        statusSystem,
        droneSystem
    );
    CardPlayValidator validator;
    CardPlaySystem play(cards, validator, energy, effects);
    Random random(800u);
    const PlayCardResult result = play.playCard(
        state,
        PlayCardRequest{CardInstanceId{802}, state.players.front().id, state.enemies.front().id},
        random
    );
    check(result.played, "telemetry test card must be playable");
    check(state.telemetry.cardsPlayed == 1 && state.telemetry.energySpentOnCards == 1,
          "played cards and their energy cost must be recorded");

    CombatController controller;
    const CombatResult combatResult = controller.buildResult(state);
    check(combatResult.telemetry.damageDealtToEnemies == 10,
          "combat results must preserve accumulated telemetry");
}

void testEnemyRoles() {
    check(enemyRoleFromString("striker") == EnemyRole::Striker, "striker role must parse");
    check(enemyRoleFromString("defender") == EnemyRole::Defender, "defender role must parse");
    check(enemyRoleFromString("support") == EnemyRole::Support, "support role must parse");
    check(enemyRoleFromString("controller") == EnemyRole::Controller, "controller role must parse");
    check(enemyRoleFromString("bruiser") == EnemyRole::Bruiser, "bruiser role must parse");
    check(enemyRoleFromString("minion") == EnemyRole::Minion, "minion role must parse");
    check(enemyRoleFromString("boss") == EnemyRole::Boss, "boss role must parse");
    check(toString(EnemyRole::Support) == "support", "enemy roles must serialize to stable data ids");
}

} // namespace


void testEnemyDamageDifficultyMultiplier() {
    LocalizationManager localization;
    StatusDatabase statuses;
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damage(modifiers);

    CombatState state;
    state.enemyDamageMultiplier = 1.5f;
    state.enemies.push_back(makeEntity(1, EntityType::Enemy, "enemy", 20));
    state.players.push_back(makeEntity(2, EntityType::Player, "player", 30));

    const DamageResult result = damage.dealDamage(
        state,
        state.enemies.front().id,
        state.players.front().id,
        10,
        CardId(),
        DiceCorruption{},
        false
    );

    check(result.modifiedDamage == 15, "hard difficulty enemy damage multiplier must affect real combat damage");
    check(state.players.front().health.current() == 15, "difficulty-scaled damage must be applied to player health");
}


void testEffectScalingAndDiscardRecovery() {
    CombatState state;
    state.players.push_back(makeEntity(901, EntityType::Player, "scaler", 40));
    state.enemies.push_back(makeEntity(902, EntityType::Enemy, "marked", 40));
    state.enemies.front().statuses.set("poison", 3);

    for (int i = 0; i < 4; ++i) {
        CardInstance card;
        card.instanceId = CardInstanceId{9100u + static_cast<std::uint64_t>(i)};
        card.definitionId = CardId("hand_card");
        state.hand.add(card);
    }
    for (int i = 0; i < 3; ++i) {
        CardInstance card;
        card.instanceId = CardInstanceId{9200u + static_cast<std::uint64_t>(i)};
        card.definitionId = CardId("discard_card");
        state.deck.discardPile.addTop(card);
    }

    EffectDefinition scaled;
    scaled.type = EffectType::Damage;
    scaled.target = EffectTarget::SingleEnemy;
    scaled.value = EffectValue::fixed(5);
    scaled.scaling.statusId = "poison";
    scaled.scaling.statusOwner = EffectScalingStatusOwner::Target;
    scaled.scaling.bonusIfStatusPresent = 2;
    scaled.scaling.bonusPerStatusStack = 2;
    scaled.scaling.bonusPerCardInHand = 1;
    scaled.scaling.bonusPerCardInDiscard = 1;
    scaled.scaling.maximumBonus = 10;
    check(effectScalingBonus(state, scaled, state.players.front().id, state.enemies.front().id) == 10,
          "effect scaling must combine status, hand, and discard bonuses and respect the cap");
    check(scaledEffectAmount(state, scaled, state.players.front().id, state.enemies.front().id, 5) == 15,
          "scaled effect amount must add the computed dynamic bonus");

    LocalizationManager localization;
    StatusDatabase statuses = makeStatusDatabase();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver resolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses);
    DroneDatabase drones;
    DroneSystem droneSystem(
        drones, resolver, targeting, damageSystem, blockSystem,
        energySystem, drawSystem, statusSystem
    );
    EffectSystem effects(
        resolver, targeting, damageSystem, blockSystem,
        energySystem, drawSystem, statusSystem, droneSystem
    );

    EffectDefinition recover;
    recover.type = EffectType::RecoverCards;
    recover.target = EffectTarget::Self;
    recover.value = EffectValue::fixed(2);
    EffectContext context;
    context.source = state.players.front().id;
    effects.applyEffect(state, recover, context);
    check(state.hand.size() == 6 && state.deck.discardPile.size() == 1,
          "recover cards must move cards from discard into the hand");

    while (!state.hand.full()) {
        CardInstance card;
        card.instanceId = CardInstanceId{9300u + static_cast<std::uint64_t>(state.hand.size())};
        card.definitionId = CardId("filler");
        state.hand.add(card);
    }
    effects.applyEffect(state, recover, context);
    check(state.hand.size() == Hand::MaximumSize && state.deck.discardPile.size() == 1,
          "recover cards must respect the hand size limit without consuming discard cards");
}

void testCombatLogSequenceAndCapacity() {
    CombatLog log;
    for (int index = 0; index < 300; ++index) {
        log.addText("entry " + std::to_string(index));
    }

    check(log.entries().size() == 256u, "combat log must keep a bounded recent history");
    check(log.entries().front().sequence == 45u, "combat log must evict the oldest entries first");
    check(log.entries().back().sequence == 300u, "combat log sequence must preserve event order");

    log.clear();
    log.addText("fresh");
    check(log.entries().size() == 1u && log.entries().front().sequence == 1u,
          "clearing the combat log must reset journal sequencing");
}


void testShopEconomyScaling() {
    check(ShopEconomy::scaledPrice(100, 1, 5) == 100, "first-floor prices must use their base value");
    check(ShopEconomy::scaledPrice(100, 3, 5) == 110, "floor price growth must be linear and predictable");
    check(
        ShopEconomy::cardRemovalPrice(75, 3, 2, 10, 25) == 145,
        "card removal must scale with both floor and previous removals"
    );
    check(
        ShopEconomy::affordableCardPriceCap(100, 80, 20) == 80,
        "affordable card cap must preserve part of the player's gold"
    );
    check(
        ShopEconomy::affordableCardPriceCap(15, 80, 20) == 15,
        "affordability must not invent gold below the minimum card price"
    );
}

void testCardRewardQuality() {
    auto makeCard = [](const std::string& id, const EffectType type, const std::optional<std::string> status = std::nullopt) {
        CardDefinition card;
        card.id = CardId(id);
        card.rarity = CardRarity::Common;
        EffectDefinition effect;
        effect.type = type;
        effect.statusId = status;
        card.effects.push_back(effect);
        return card;
    };

    CardDefinition poisonSetup = makeCard("poison_setup", EffectType::ApplyStatus, std::string("poison"));
    CardDefinition poisonPayoff = makeCard("poison_payoff", EffectType::Damage);
    poisonPayoff.effects.front().scaling.statusId = "poison";
    CardDefinition plainAttack = makeCard("plain_attack", EffectType::Damage);
    CardDefinition block = makeCard("block", EffectType::Block);
    CardDefinition heal = makeCard("heal", EffectType::Heal);
    CardDefinition orphanDrone = makeCard("orphan_drone", EffectType::UseDrone);

    const std::vector<const CardDefinition*> deck{&poisonSetup, &plainAttack, &plainAttack};
    check(
        CardRewardQuality::relevanceScore(poisonPayoff, deck) > CardRewardQuality::relevanceScore(orphanDrone, deck),
        "a supported status payoff must outrank an unusable drone payoff"
    );
    check(
        CardRewardQuality::relevanceScore(plainAttack, deck) < CardRewardQuality::relevanceScore(block, deck),
        "duplicate attacks must be penalized when the deck lacks block"
    );

    Random random(42u);
    const std::vector<const CardDefinition*> offers = CardRewardQuality::chooseOffers(
        {&poisonPayoff, &plainAttack, &block, &heal, &orphanDrone},
        deck,
        3,
        random
    );
    check(offers.size() == 3u, "quality generator must preserve the requested offer count");
    check(offers.front()->id == poisonPayoff.id, "the first offer must represent the strongest existing synergy");
    check(
        std::any_of(offers.begin(), offers.end(), [&block](const CardDefinition* card) { return card != nullptr && card->id == block.id; }),
        "a diverse offer must cover a missing defensive role"
    );
}
int main() {
    testCardRewardQuality();
    testShopEconomyScaling();
    testCombatLogSequenceAndCapacity();
    testEnemyDamageDifficultyMultiplier();
    testEffectScalingAndDiscardRecovery();
    testCardLifecycleAndOpeningHand();
    testDamageAndBlockModifiers();
    testDataDrivenStatusDefinitions();
    testStatusesOwnCardNumberScaling();
    testDamageOverTimeAndDurations();
    testMultiEnemyOutcomeAndIntentCleanup();
    testMultiEnemyTargeting();
    testExplicitAndAutomaticSingleTargetSafety();
    testEffectChainSkipsTargetsKilledByReactions();
    testEnemyRoles();
    testCombatTelemetryCounters();
    testBossPhaseTransitionsAndSummons();
    testConditionalEnemyAi();
    testEnemyIntentPresentation();
    testStressDamageAndConversions();
    testStressCardPlayValidation();
    testStressEconomyRules();
    testStressBandsAndPsychopathEffects();
    testStressStartOfTurnEffects();
    testStressBreakdownPrimingAndGuard();
    testStressEventRequirements();
    testActiveItemChargeAndUse();
    testActiveItemAcquisition();
    testRerollDieOffers();
    testExpandedActiveItemContexts();

    if (failures != 0) {
        std::cerr << failures << " combat core test(s) failed\n";
        return 1;
    }

    std::cout << "All combat core tests passed\n";
    return 0;
}
