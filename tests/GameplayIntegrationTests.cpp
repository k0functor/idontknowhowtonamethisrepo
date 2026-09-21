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
#include "preview/CardPreviewSystem.hpp"
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
#include "events/RunEventRequirement.hpp"
#include "events/RunEventSelector.hpp"
#include "game/GameEventBus.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicSystem.hpp"
#include "rewards/RewardTuning.hpp"
#include "run/RunPacingTracker.hpp"
#include "run/RunState.hpp"
#include "run/SadistMasochistRules.hpp"
#include "run/StressPsychopathRules.hpp"
#include "run/StressRules.hpp"
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

void DroneDatabase::clear() {
    drones_.clear();
}

void DroneDatabase::add(DroneDefinition definition) {
    drones_.emplace(definition.id.value, std::move(definition));
}

bool DroneDatabase::contains(const DroneId& id) const {
    return drones_.contains(id.value);
}

const DroneDefinition& DroneDatabase::get(const DroneId& id) const {
    const auto iterator = drones_.find(id.value);
    if (iterator == drones_.end()) {
        throw std::runtime_error("unknown integration-test drone");
    }
    return iterator->second;
}

std::vector<const DroneDefinition*> DroneDatabase::all() const {
    std::vector<const DroneDefinition*> result;
    result.reserve(drones_.size());
    for (const auto& [_, definition] : drones_) {
        result.push_back(&definition);
    }
    return result;
}

std::size_t DroneDatabase::size() const {
    return drones_.size();
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

    for (const char* stanceId : {"stance_flame", "stance_ash", "stance_smoke"}) {
        StatusDefinition stance;
        stance.id = StatusId(stanceId);
        stance.nameTextId = TextId(std::string("status.") + stanceId + ".name");
        stance.descriptionTextId = TextId(std::string("status.") + stanceId + ".description");
        stance.type = StatusType::Buff;
        stance.durationRule = StatusDurationRule::PersistentCombat;
        stance.exclusiveGroup = "monk_stance";
        add(std::move(stance));
    }

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

    RelicTriggerDefinition thirdCardTrigger;
    thirdCardTrigger.eventType = GameEventType::CardPlayed;
    thirdCardTrigger.cardNumberThisTurn = 3;
    thirdCardTrigger.sourceSide = "player";
    thirdCardTrigger.effects.push_back(fixedEffect(EffectType::ApplyStatus, EffectTarget::Self, 1, "strength"));
    relic.triggers.push_back(std::move(thirdCardTrigger));

    RelicTriggerDefinition alternatingTrigger;
    alternatingTrigger.eventType = GameEventType::CardPlayed;
    alternatingTrigger.cardType = CardType::Skill;
    alternatingTrigger.previousCardType = CardType::Attack;
    alternatingTrigger.sourceSide = "player";
    alternatingTrigger.effects.push_back(fixedEffect(EffectType::ApplyStatus, EffectTarget::Self, 1, "dexterity"));
    relic.triggers.push_back(std::move(alternatingTrigger));

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

    check(state.players.front().statuses.stacks("strength") == 2,
          "the third-card relic condition must trigger after the third card resolves");
    check(state.players.front().statuses.stacks("dexterity") == 2,
          "a Skill played immediately after an Attack must satisfy the sequence relic condition");
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
    state.enemies.front().boss = true;
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

    state.enemies.front().health.setCurrent(0);
    CombatController controller;
    const CombatResult result = controller.updateAfterAction(state);
    check(result.outcome == CombatOutcome::Victory,
          "defeating a phased boss must also finish its summoned entourage");
    check(state.aliveEnemyCount() == 0u,
          "summoned boss minions must die immediately with their boss");
}

void testExactRepeatedDamagePreview() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatuses();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    CardPlayValidator validator;
    EffectResolver resolver;

    CardDefinition repeatedStrike = makeCard(
        "preview_repeated_strike",
        CardType::Attack,
        1,
        {fixedEffect(EffectType::Damage, EffectTarget::SingleEnemy, 10)}
    );
    repeatedStrike.effects.front().repeatCount = 2;

    CardDatabase cards;
    cards.add(repeatedStrike);

    CombatState state;
    state.phase = CombatPhase::PlayerTurn;
    state.players.push_back(makeEntity(201, EntityType::Player, "preview_source", 30));
    state.enemies.push_back(makeEntity(202, EntityType::Enemy, "preview_target", 40));
    state.players.front().statuses.set("strength", 2);
    state.enemies.front().block = 5;
    state.resources.setMaxEnergy(state.players.front().id, 3);
    state.resources.gainEnergy(state.players.front().id, 3);

    CardInstance instance;
    instance.instanceId = CardInstanceId{7001};
    instance.definitionId = repeatedStrike.id;
    state.hand.add(instance);

    CardPreviewSystem previews(cards, validator, resolver, damageSystem, blockSystem);
    const CardPreview preview = previews.previewCard(
        state,
        instance.instanceId,
        state.players.front().id,
        state.enemies.front().id
    );

    check(preview.outcome.modifiedDamage.minimum == 24 && preview.outcome.modifiedDamage.maximum == 24,
          "card preview must include strength and every repeated hit");
    check(preview.outcome.hpDamage.minimum == 19 && preview.outcome.hpDamage.maximum == 19,
          "card preview must consume target block only once across repeated hits");
    check(!preview.outcome.modifierLabels.empty(),
          "card preview must expose the modifier that changed the printed value");
}


void testRunPacingTracksRoomsFloorsAndPhases() {
    RunState run;
    run.currentFloorId = "floor1";
    run.currentFloorIndex = 1;
    run.phase = RunPhase::Map;

    RunPacingTracker::tick(run, 2.f);
    check(run.pacing.activeSeconds == 0.25f && run.pacing.mapSeconds == 0.25f,
          "pacing must clamp a single frame and attribute it to the active phase");

    RunPacingTracker::beginRoom(run, 7, RunMapNodeType::Combat);
    run.phase = RunPhase::Combat;
    RunPacingTracker::tick(run, 0.10f);
    RunPacingTracker::tick(run, 0.15f);
    RunPacingTracker::finishRoom(run, 7);

    check(run.pacing.completedRooms.size() == 1,
          "pacing must store a completed room sample");
    check(run.pacing.completedRooms.front().nodeId == 7 &&
              run.pacing.completedRooms.front().activeSeconds > 0.24f,
          "pacing must retain the node id and active room duration");
    check(run.pacing.combatSeconds > 0.24f,
          "pacing must attribute room time to combat");

    run.stats.nodesCompleted = 1;
    RunPacingTracker::finishFloor(run);
    check(run.pacing.completedFloors.size() == 1,
          "pacing must store a completed floor sample");
    check(run.pacing.completedFloors.front().roomsCompleted == 1,
          "floor pacing must include completed room count");

    run.currentFloorId = "floor2";
    run.currentFloorIndex = 2;
    RunPacingTracker::beginFloor(run);
    check(run.pacing.floorActive && run.pacing.currentFloorSeconds == 0.f,
          "starting a floor must reset only the current floor timer");
}

void testEventStateRequirementsAndUnseenSelection() {
    RunState run;
    RunActorState actor;
    actor.definitionId = "event_tester";
    actor.currentHp = 20;
    actor.maxHp = 40;
    run.actorStates.push_back(actor);
    for (int index = 0; index < 12; ++index) {
        run.deckCardIds.push_back(CardId("event_card_" + std::to_string(index)));
    }
    run.upgradedDeckIndices = {0, 1, 2};

    RunEventChoiceRequirements requirements;
    requirements.minDeckSize = 8;
    requirements.maxDeckSize = 12;
    requirements.minMissingHp = 15;
    requirements.minUpgradedCards = 3;
    check(evaluateRunEventChoiceRequirements(requirements, run).available,
          "event requirements must accept a matching run state");

    run.deckCardIds.push_back(CardId("event_card_extra"));
    RunEventChoiceAvailability unavailable = evaluateRunEventChoiceRequirements(requirements, run);
    check(!unavailable.available && std::any_of(
              unavailable.reasons.begin(), unavailable.reasons.end(),
              [](const RunEventChoiceBlockReason& reason) {
                  return reason.type == RunEventChoiceBlockReasonType::TooManyCards;
              }),
          "event requirements must enforce maximum deck size");
    run.deckCardIds.pop_back();

    run.upgradedDeckIndices = {0, 1};
    unavailable = evaluateRunEventChoiceRequirements(requirements, run);
    check(!unavailable.available && std::any_of(
              unavailable.reasons.begin(), unavailable.reasons.end(),
              [](const RunEventChoiceBlockReason& reason) {
                  return reason.type == RunEventChoiceBlockReasonType::NotEnoughUpgradedCards;
              }),
          "event requirements must inspect upgraded-card count");
    run.upgradedDeckIndices = {0, 1, 2};

    run.actorStates.front().currentHp = 30;
    unavailable = evaluateRunEventChoiceRequirements(requirements, run);
    check(!unavailable.available && std::any_of(
              unavailable.reasons.begin(), unavailable.reasons.end(),
              [](const RunEventChoiceBlockReason& reason) {
                  return reason.type == RunEventChoiceBlockReasonType::NotWoundedEnough;
              }),
          "event requirements must inspect missing party HP");
    run.actorStates.front().currentHp = 20;

    RunEventDefinition seen;
    seen.id = "seen";
    RunEventDefinition unseen;
    unseen.id = "unseen";
    std::vector<const RunEventDefinition*> pool{&seen, &unseen};
    run.eventFlags.push_back("event.seen.seen");
    Random random(3401u);
    check(chooseAvailableRunEvent(pool, run, random).id == "unseen",
          "event selection must prefer an unseen event");

    run.eventFlags.push_back("event.seen.unseen");
    const std::string fallbackId = chooseAvailableRunEvent(pool, run, random).id;
    check(fallbackId == "seen" || fallbackId == "unseen",
          "event selection must fall back safely after exhausting a pool");
}

void testBossDefeatClearsMinions() {
    CombatController controller;
    CombatState state;
    state.phase = CombatPhase::PlayerTurn;
    state.players.push_back(makeEntity(300, EntityType::Player, "tester", 30));

    CombatEntity boss = makeEntity(301, EntityType::Enemy, "boss", 20);
    boss.boss = true;
    boss.health.setCurrent(0);
    CombatEntity minion = makeEntity(302, EntityType::Enemy, "minion", 10);
    CombatEntity secondBoss = makeEntity(303, EntityType::Enemy, "second_boss", 12);
    secondBoss.boss = true;

    state.enemies.push_back(std::move(boss));
    state.enemies.push_back(std::move(minion));
    state.enemies.push_back(std::move(secondBoss));

    controller.updateAfterAction(state);

    check(!state.enemies[1].isAlive(), "defeating a boss must immediately defeat its non-boss entourage");
    check(state.enemies[2].isAlive(), "defeating one boss must not automatically kill another boss");
    check(state.phase == CombatPhase::PlayerTurn,
          "combat must continue when another boss remains alive");
}


void testStressCollapseAndPsychopathIsolation() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatuses();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver resolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses);
    DroneDatabase drones;
    DroneSystem droneSystem(drones, resolver, targeting, damageSystem, blockSystem, energySystem, drawSystem, statusSystem);
    EffectSystem effects(resolver, targeting, damageSystem, blockSystem, energySystem, drawSystem, statusSystem, droneSystem);

    CombatState collapse;
    collapse.phase = CombatPhase::PlayerTurn;
    collapse.players.push_back(makeEntity(400, EntityType::Player, "rusted_knight", 30));
    collapse.enemies.push_back(makeEntity(401, EntityType::Enemy, "stress_dummy", 30));
    collapse.players.front().stress = 195;
    collapse.players.front().maxStress = StressRules::MaximumStress;

    EffectContext stressContext;
    stressContext.source = collapse.players.front().id;
    Random random(3701u);
    stressContext.random = &random;
    effects.applyEffects(collapse, {fixedEffect(EffectType::GainStress, EffectTarget::Self, 10)}, stressContext);

    check(collapse.players.front().stress == StressRules::MaximumStress,
          "stress gain must clamp at the configured maximum");
    check(!collapse.players.front().isAlive(),
          "a normal actor reaching maximum stress must collapse and die");
    CombatController combatController;
    check(combatController.updateAfterAction(collapse).outcome == CombatOutcome::Defeat,
          "stress collapse of the final actor must end combat in defeat");

    CombatState normal;
    normal.players.push_back(makeEntity(410, EntityType::Player, "rusted_knight", 30));
    normal.enemies.push_back(makeEntity(411, EntityType::Enemy, "damage_dummy", 30));
    normal.players.front().stress = StressRules::PanickedThreshold;
    const DamageResult normalDamage = damageSystem.dealDamage(
        normal, normal.players.front().id, normal.enemies.front().id, 5,
        CardId("qa_attack"), DiceCorruption{}, true
    );

    CombatState psychopath;
    psychopath.players.push_back(makeEntity(420, EntityType::Player, "lost_psychopath", 30));
    psychopath.enemies.push_back(makeEntity(421, EntityType::Enemy, "damage_dummy", 30));
    psychopath.players.front().stress = StressRules::PanickedThreshold;
    const DamageResult psychopathDamage = damageSystem.dealDamage(
        psychopath, psychopath.players.front().id, psychopath.enemies.front().id, 5,
        CardId("qa_attack"), DiceCorruption{}, true
    );

    check(normalDamage.modifiedDamage == 5,
          "high stress must not grant a direct damage bonus to ordinary actors");
    check(psychopathDamage.modifiedDamage == 8,
          "high stress must grant the psychopath-only damage bonus");
}

void testMonkStanceCycleAndReward() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatuses();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver resolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses);
    DroneDatabase drones;
    DroneSystem droneSystem(drones, resolver, targeting, damageSystem, blockSystem, energySystem, drawSystem, statusSystem);
    EffectSystem effects(resolver, targeting, damageSystem, blockSystem, energySystem, drawSystem, statusSystem, droneSystem);

    CombatState state;
    state.players.push_back(makeEntity(500, EntityType::Player, "monk", 30));
    const EntityId monkId = state.players.front().id;
    state.resources.setMaxEnergy(monkId, 3);
    state.resources.spendEnergy(monkId, 3);
    for (std::uint64_t id = 1; id <= 4; ++id) {
        CardInstance card;
        card.instanceId = CardInstanceId{5000 + id};
        card.definitionId = CardId("qa_draw_card");
        state.deck.drawPile.addTop(std::move(card));
    }

    statusSystem.applyStatus(state, monkId, "stance_flame", 1, monkId);
    EffectContext context;
    context.source = monkId;
    Random random(3702u);
    context.random = &random;
    effects.applyEffects(state, {fixedEffect(EffectType::EnterStance, EffectTarget::Self, 0, "stance_ash")}, context);

    check(!state.players.front().statuses.has("stance_flame") && state.players.front().statuses.has("stance_ash"),
          "entering a stance must replace the previous stance in the exclusive group");
    check(state.resources.energyFor(monkId) == 1 && state.hand.size() == 2u,
          "a real monk stance shift must grant one energy and draw two cards");

    effects.applyEffects(state, {fixedEffect(EffectType::EnterStance, EffectTarget::Self, 0, "stance_smoke")}, context);
    check(!state.players.front().statuses.has("stance_ash") && state.players.front().statuses.has("stance_smoke"),
          "the stance cycle must keep exactly one active stance");
    check(state.resources.energyFor(monkId) == 2 && state.hand.size() == 4u,
          "successive stance shifts must each trigger the monk reward");

    CombatState ordinary;
    ordinary.players.push_back(makeEntity(510, EntityType::Player, "rusted_knight", 30));
    const EntityId ordinaryId = ordinary.players.front().id;
    ordinary.resources.setMaxEnergy(ordinaryId, 3);
    ordinary.resources.spendEnergy(ordinaryId, 3);
    statusSystem.applyStatus(ordinary, ordinaryId, "stance_flame", 1, ordinaryId);
    EffectContext ordinaryContext;
    ordinaryContext.source = ordinaryId;
    ordinaryContext.random = &random;
    effects.applyEffects(ordinary, {fixedEffect(EffectType::EnterStance, EffectTarget::Self, 0, "stance_ash")}, ordinaryContext);
    check(ordinary.resources.energyFor(ordinaryId) == 0 && ordinary.hand.empty(),
          "stance-shift rewards must remain exclusive to the monk actor");
}

void testDroneSlotOverflowPassiveAndConsumption() {
    LocalizationManager localization;
    StatusDatabase statuses = makeStatuses();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver resolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses);
    DroneDatabase drones;

    const auto addDrone = [&drones](const char* id, const int damage, const int passiveBlock) {
        DroneDefinition drone;
        drone.id = DroneId(id);
        DroneActionDefinition active;
        active.effects.push_back(fixedEffect(EffectType::Damage, EffectTarget::SingleEnemy, damage));
        drone.activeAction = active;
        if (passiveBlock > 0) {
            DroneActionDefinition passive;
            passive.effects.push_back(fixedEffect(EffectType::Block, EffectTarget::Self, passiveBlock));
            drone.passiveAction = passive;
        }
        drones.add(std::move(drone));
    };
    addDrone("qa_drone_a", 1, 0);
    addDrone("qa_drone_b", 3, 2);
    addDrone("qa_drone_c", 4, 0);
    addDrone("qa_drone_d", 5, 0);

    DroneSystem droneSystem(drones, resolver, targeting, damageSystem, blockSystem, energySystem, drawSystem, statusSystem);
    CombatState state;
    state.players.push_back(makeEntity(600, EntityType::Player, "replicant", 30));
    state.enemies.push_back(makeEntity(601, EntityType::Enemy, "drone_target", 30));
    state.maxDroneSlots = 3;
    const EntityId owner = state.players.front().id;

    droneSystem.summonDrone(state, "qa_drone_a", owner);
    droneSystem.summonDrone(state, "qa_drone_b", owner);
    droneSystem.summonDrone(state, "qa_drone_c", owner);
    droneSystem.summonDrone(state, "qa_drone_d", owner);
    check(state.droneSlots.size() == 3u,
          "summoning beyond the drone cap must keep exactly three slots");
    check(state.droneSlots[0].droneId == "qa_drone_b" && state.droneSlots[2].droneId == "qa_drone_d",
          "overflow must evict the oldest drone, not a random or newest slot");

    Random random(3703u);
    droneSystem.processEndOfPlayerTurn(state, random);
    check(state.players.front().block == 2,
          "passive drones must trigger at the end of the player turn");
    droneSystem.useOldestDrone(state, &random);
    check(state.enemies.front().health.current() == 27,
          "using the oldest drone must execute its active effect");
    check(state.droneSlots.size() == 2u && state.droneSlots.front().droneId == "qa_drone_c",
          "an activated drone must be consumed from its exact slot");
}

void testSadistMasochistEngineAndSequentialTurns() {
    CombatState state;
    state.phase = CombatPhase::PlayerTurn;
    state.turn = 1;
    state.useSequentialPlayerTurns = true;
    state.players.push_back(makeEntity(700, EntityType::Player, "sadist", 30));
    state.players.push_back(makeEntity(701, EntityType::Player, "masochist", 30));
    state.enemies.push_back(makeEntity(702, EntityType::Enemy, "turn_dummy", 30));

    GameEvent event;
    event.type = GameEventType::DamageDealt;
    event.source = state.players[0].id;
    event.target = state.players[1].id;
    event.amount = 4;
    SadistMasochistRules::handleEvent(state, event);
    check(state.players[0].statuses.stacks(SadistMasochistRules::PleasureStatusId) == 1 &&
              state.players[1].statuses.stacks(SadistMasochistRules::PainStatusId) == 1,
          "sadist damage to masochist must arm both sides of the duo engine");

    LocalizationManager localization;
    StatusDatabase statuses = makeStatuses();
    ModifierSystem modifiers(localization, statuses);
    DamageSystem damageSystem(modifiers);
    BlockSystem blockSystem(modifiers);
    EnergySystem energySystem;
    DrawSystem drawSystem;
    EffectResolver resolver;
    Targeting targeting;
    StatusSystem statusSystem(statuses);
    DroneDatabase drones;
    DroneSystem droneSystem(drones, resolver, targeting, damageSystem, blockSystem, energySystem, drawSystem, statusSystem);
    EffectSystem effectSystem(resolver, targeting, damageSystem, blockSystem, energySystem, drawSystem, statusSystem, droneSystem);
    CardDatabase cards;
    PlayerTurnSystem playerTurns(drawSystem, cards, nullptr, &effectSystem);
    EnemyDefinition enemy;
    enemy.id = EnemyId("turn_dummy");
    enemy.maxHp = 30;
    enemy.actions.push_back(attackAction("turn_dummy.wait", 0));
    EnemyDatabase enemies;
    enemies.add(std::move(enemy));
    EnemyMoveSelector selector(modifiers);
    EnemyTurnSystem enemyTurns(selector, effectSystem);
    BossPhaseSystem phases(enemies, effectSystem);
    CombatController controller;
    TurnSystem turns(enemies, playerTurns, enemyTurns, selector, phases, statusSystem, droneSystem, controller, 5);

    Random random(3704u);
    turns.endPlayerTurn(state, random, true);
    check(state.turn == 1 && state.activePlayerIndex == 1 && state.activePlayerId() == state.players[1].id,
          "ending Sadist's subturn must pass the same round to Masochist");

    state.players[0].health.setCurrent(0);
    turns.endPlayerTurn(state, random, true);
    check(state.turn == 2 && state.activePlayerId() == state.players[1].id,
          "a dead duo member must be skipped when the next round starts");
}

} // namespace

int main() {
    testStressCollapseAndPsychopathIsolation();
    testMonkStanceCycleAndReward();
    testDroneSlotOverflowPassiveAndConsumption();
    testSadistMasochistEngineAndSequentialTurns();
    testRunPacingTracksRoomsFloorsAndPhases();
    testEventStateRequirementsAndUnseenSelection();
    testBossDefeatClearsMinions();
    testExactRepeatedDamagePreview();
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
