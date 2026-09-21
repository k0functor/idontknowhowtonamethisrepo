#include "PlayerTurnSystem.hpp"

#include "cards/CardKeyword.hpp"
#include "cards/CardType.hpp"
#include "cards/CardUpgrade.hpp"
#include "combat/CardCost.hpp"
#include "effects/EffectType.hpp"
#include "run/StressPsychopathRules.hpp"
#include "run/StressRules.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr const char* woundStatusCardId = "wound_status";
constexpr const char* burnStatusCardId = "burn_status";

std::uint64_t nextCardInstanceValue(const CombatState& state) {
    std::uint64_t maximum = 0;
    const auto scan = [&maximum](const std::vector<CardInstance>& cards) {
        for (const CardInstance& card : cards) {
            maximum = std::max(maximum, card.instanceId.value);
        }
    };

    scan(state.hand.cards());
    scan(state.deck.drawPile.cards());
    scan(state.deck.discardPile.cards());
    scan(state.deck.exhaustPile.cards());
    return maximum + 1;
}

int discardRandomCards(
    CombatState& state,
    const int amount,
    Random& random,
    const CombatEntity& player
) {
    int discardedCount = 0;
    while (discardedCount < amount && !state.hand.empty()) {
        const int randomIndex = random.rangeInclusive(0, static_cast<int>(state.hand.size() - 1u));
        std::vector<CardInstance>& cards = state.hand.cards();
        CardInstance discarded = std::move(cards[static_cast<std::size_t>(randomIndex)]);
        cards.erase(cards.begin() + randomIndex);

        const std::string discardedCardId = discarded.definitionId.value;
        if (discarded.temporary) {
            state.deck.exhaustPile.addTop(std::move(discarded));
        } else {
            state.deck.discardPile.addTop(std::move(discarded));
        }
        state.log.add(
            CombatLogEntryType::StressBreakdownDiscard,
            {
                {"actor", player.definitionId},
                {"actor_text_id", player.nameTextId.value},
                {"card", discardedCardId}
            }
        );
        ++discardedCount;
    }
    return discardedCount;
}

bool hasKeyword(const CardDefinition& definition, const CardKeyword keyword) {
    return std::find(definition.keywords.begin(), definition.keywords.end(), keyword) != definition.keywords.end();
}

int requiredStressForCard(const CardDefinition& definition) {
    int result = 0;
    for (const EffectDefinition& effect : definition.effects) {
        if (isStressConversionEffect(effect.type) && effect.value.isFixed()) {
            result += effect.value.fixedAmount() * std::max(1, effect.repeatCount);
        }
    }
    return result;
}

std::vector<CardInstanceId> frenzyCandidates(
    const CombatState& state,
    const CombatEntity& player,
    const CardDatabase& cardDatabase
) {
    std::vector<CardInstanceId> result;
    for (const CardInstance& card : state.hand.cards()) {
        if (!cardDatabase.contains(card.definitionId)) {
            continue;
        }

        const CardDefinition definition = CardUpgrade::effectiveDefinition(
            cardDatabase.get(card.definitionId),
            card.upgraded
        );
        if (definition.type != CardType::Attack || hasKeyword(definition, CardKeyword::Unplayable)) {
            continue;
        }
        if (!definition.ownerActorId.empty() && definition.ownerActorId != player.definitionId) {
            continue;
        }
        if (CardCost::effectiveEnergyCost(state, player.id, definition) > state.resources.energyFor(player.id)) {
            continue;
        }
        if (requiredStressForCard(definition) > player.stress) {
            continue;
        }
        result.push_back(card.instanceId);
    }
    return result;
}
}

PlayerTurnSystem::PlayerTurnSystem(
    const DrawSystem& drawSystem,
    const CardDatabase& cardDatabase,
    const GameEventBus* eventBus,
    const EffectSystem* effectSystem
)
    : drawSystem_(drawSystem),
      cardDatabase_(cardDatabase),
      eventBus_(eventBus),
      effectSystem_(effectSystem) {}

void PlayerTurnSystem::startTurn(
    CombatState& state,
    const std::size_t handSize,
    Random& random
) const {
    state.phase = CombatPhase::PlayerTurn;
    state.pendingForcedCardPlay.reset();
    state.clearTurnCardCostModifiers();
    state.resources.resetEnergy();

    for (CombatEntity& player : state.players) {
        player.block = 0;

        if (!player.isAlive() || !StressPsychopathRules::appliesTo(player.definitionId)) {
            continue;
        }

        const int energyBonus = StressPsychopathRules::startTurnEnergyBonus(player.stress);
        if (energyBonus > 0) {
            state.resources.gainEnergy(player.id, energyBonus);
            state.log.add(CombatLogEntryType::GainEnergy, {{"amount", std::to_string(energyBonus)}});
        }
    }

    if (state.turn == 1) {
        drawSystem_.drawOpeningHand(state.deck, state.hand, cardDatabase_, handSize, random);
    } else {
        drawSystem_.drawCards(state.deck, state.hand, handSize, random);
    }
    state.log.add(CombatLogEntryType::PlayerTurnStarted, {{"turn", std::to_string(state.turn)}});

    applyStartOfTurnTraitEffects(state, random);
}

void PlayerTurnSystem::applyStartOfTurnTraitEffects(CombatState& state, Random& random) const {
    for (const CombatEntity& playerSnapshot : state.players) {
        if (!playerSnapshot.isAlive()) {
            continue;
        }

        const bool hasBreakdown = StressRules::hasTrait(playerSnapshot, StressRules::BreakdownTraitId);
        const bool hasResolve = StressRules::hasTrait(playerSnapshot, StressRules::ResolveTraitId);

        if (!StressPsychopathRules::appliesTo(playerSnapshot.definitionId)) {
            continue;
        }

        if (StressBreakdownRules::canTrigger(playerSnapshot.stress, hasBreakdown, hasResolve)) {
            if (state.consumeStressBreakdownGuard(playerSnapshot.id)) {
                state.consumePrimedStressBreakdown(playerSnapshot.id);
                state.log.add(
                    CombatLogEntryType::StressBreakdownPrevented,
                    {{"actor", playerSnapshot.definitionId}, {"actor_text_id", playerSnapshot.nameTextId.value}}
                );
                continue;
            }

            StressBreakdownRules::BreakdownType breakdownType = StressBreakdownRules::choose(playerSnapshot.stress, random);
            if (const std::optional<std::string> primed = state.consumePrimedStressBreakdown(playerSnapshot.id)) {
                const std::optional<StressBreakdownRules::BreakdownType> parsed = StressBreakdownRules::fromString(*primed);
                if (parsed.has_value() && StressBreakdownRules::isAvailableAtStress(*parsed, playerSnapshot.stress)) {
                    breakdownType = *parsed;
                }
            }

            applyStressBreakdown(state, playerSnapshot.id, breakdownType, random);
            continue;
        }

        const int unstableDiscard = StressPsychopathRules::startTurnDiscardCount(
            playerSnapshot.stress,
            hasBreakdown,
            hasResolve
        );
        if (unstableDiscard > 0) {
            discardRandomCards(state, unstableDiscard, random, playerSnapshot);
        }
    }
}

void PlayerTurnSystem::applyStressBreakdown(
    CombatState& state,
    const EntityId playerId,
    StressBreakdownRules::BreakdownType type,
    Random& random
) const {
    if (!state.hasEntity(playerId) || !state.isPlayer(playerId)) {
        return;
    }

    const CombatEntity playerSnapshot = state.entity(playerId);
    const int severity = StressBreakdownRules::severityForStress(playerSnapshot.stress);
    if (severity <= 0) {
        return;
    }

    const auto emitBreakdownEvent = [this, &state, playerId, type, severity]() {
        if (eventBus_ == nullptr) {
            return;
        }
        GameEvent event;
        event.type = GameEventType::StressBreakdownTriggered;
        event.source = playerId;
        event.breakdownType = StressBreakdownRules::toString(type);
        event.breakdownSeverity = severity;
        event.amount = severity;
        event.turn = state.turn;
        eventBus_->emit(event);
    };

    switch (type) {
        case StressBreakdownRules::BreakdownType::None:
            return;

        case StressBreakdownRules::BreakdownType::Discard:
            discardRandomCards(state, severity, random, playerSnapshot);
            emitBreakdownEvent();
            return;

        case StressBreakdownRules::BreakdownType::EnergyCrash: {
            const int currentEnergy = state.resources.energyFor(playerId);
            const int lostEnergy = std::min(currentEnergy, severity);
            if (lostEnergy > 0) {
                state.resources.spendEnergy(playerId, lostEnergy);
            }
            state.log.add(
                CombatLogEntryType::StressBreakdownEnergy,
                {
                    {"actor", playerSnapshot.definitionId},
                    {"actor_text_id", playerSnapshot.nameTextId.value},
                    {"amount", std::to_string(lostEnergy)}
                }
            );
            emitBreakdownEvent();
            return;
        }

        case StressBreakdownRules::BreakdownType::IntrusiveThoughts: {
            std::vector<std::string> statusCards;
            statusCards.push_back(woundStatusCardId);
            if (severity > 1) {
                statusCards.push_back(burnStatusCardId);
            }

            std::uint64_t nextId = nextCardInstanceValue(state);
            int added = 0;
            for (const std::string& cardId : statusCards) {
                if (!cardDatabase_.contains(CardId(cardId))) {
                    continue;
                }
                CardInstance card;
                card.instanceId = CardInstanceId{nextId++};
                card.definitionId = CardId(cardId);
                card.temporary = true;
                if (state.hand.tryAdd(std::move(card))) {
                    ++added;
                }
            }

            state.log.add(
                CombatLogEntryType::StressBreakdownStatusCards,
                {
                    {"actor", playerSnapshot.definitionId},
                    {"actor_text_id", playerSnapshot.nameTextId.value},
                    {"amount", std::to_string(added)}
                }
            );
            emitBreakdownEvent();
            return;
        }

        case StressBreakdownRules::BreakdownType::CostSpike:
            state.setCardCostModifier(playerId, severity);
            state.log.add(
                CombatLogEntryType::StressBreakdownCost,
                {
                    {"actor", playerSnapshot.definitionId},
                    {"actor_text_id", playerSnapshot.nameTextId.value},
                    {"amount", std::to_string(severity)}
                }
            );
            emitBreakdownEvent();
            return;

        case StressBreakdownRules::BreakdownType::Frenzy: {
            const std::vector<CardInstanceId> candidates = frenzyCandidates(state, playerSnapshot, cardDatabase_);
            const std::vector<EntityId> enemies = state.aliveEnemyIds();
            if (candidates.empty() || enemies.empty()) {
                discardRandomCards(state, severity, random, playerSnapshot);
                emitBreakdownEvent();
                return;
            }

            const CardInstanceId cardId = candidates[static_cast<std::size_t>(
                random.rangeInclusive(0, static_cast<int>(candidates.size() - 1u))
            )];
            const EntityId target = enemies[static_cast<std::size_t>(
                random.rangeInclusive(0, static_cast<int>(enemies.size() - 1u))
            )];
            state.pendingForcedCardPlay = ForcedCardPlay{cardId, playerId, target};
            state.log.add(
                CombatLogEntryType::StressBreakdownForcedCard,
                {
                    {"actor", playerSnapshot.definitionId},
                    {"actor_text_id", playerSnapshot.nameTextId.value},
                    {"card", state.hand.get(cardId).definitionId.value}
                }
            );
            emitBreakdownEvent();
            return;
        }
    }
}

void PlayerTurnSystem::endTurn(CombatState& state, Random& random) const {
    resolveEndOfTurnStatusCards(state, random);
    discardHand(state);
    state.pendingForcedCardPlay.reset();
    state.log.add(CombatLogEntryType::PlayerTurnEnded);
}

void PlayerTurnSystem::resolveEndOfTurnStatusCards(CombatState& state, Random& random) const {
    if (effectSystem_ == nullptr) {
        return;
    }

    std::optional<EntityId> source = state.activePlayerId();
    if (!source.has_value()) {
        const std::vector<EntityId> alivePlayers = state.alivePlayerIds();
        if (!alivePlayers.empty()) {
            source = alivePlayers.front();
        }
    }
    if (!source.has_value()) {
        return;
    }

    const std::vector<CardInstance> cards = state.hand.cards();
    for (const CardInstance& card : cards) {
        if (!cardDatabase_.contains(card.definitionId)) {
            continue;
        }

        const CardDefinition definition = CardUpgrade::effectiveDefinition(
            cardDatabase_.get(card.definitionId),
            card.upgraded
        );
        if (definition.type != CardType::Status ||
            !hasKeyword(definition, CardKeyword::Unplayable) ||
            definition.effects.empty()) {
            continue;
        }

        EffectContext context;
        context.source = *source;
        context.cardInstanceId = card.instanceId;
        context.cardDefinitionId = definition.id;
        context.usesActorStats = false;
        context.random = &random;
        effectSystem_->applyEffects(state, definition.effects, context);
    }
}

void PlayerTurnSystem::discardHand(CombatState& state) const {
    std::vector<CardInstance> cards = std::move(state.hand.cards());
    state.hand.clear();

    for (CardInstance& card : cards) {
        const CardDefinition definition = CardUpgrade::effectiveDefinition(cardDatabase_.get(card.definitionId), card.upgraded);
        const bool retains = hasKeyword(definition, CardKeyword::Retain);
        const bool ethereal = hasKeyword(definition, CardKeyword::Ethereal);

        if (card.temporary || ethereal) {
            state.deck.exhaustPile.addTop(std::move(card));
        } else if (retains) {
            state.hand.add(std::move(card));
        } else {
            state.deck.discardPile.addTop(std::move(card));
        }
    }
}
