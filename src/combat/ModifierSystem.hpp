#pragma once

#include "cards/CardId.hpp"
#include "dice/DiceCorruption.hpp"
#include "effects/EffectType.hpp"
#include "entities/EntityId.hpp"
#include "statuses/StatusDatabase.hpp"

#include <string>
#include <vector>

class CombatState;
class LocalizationManager;

enum class ModifierOperation {
    Add,
    Multiply
};

struct ValueModifier {
    std::string sourceId;
    std::string description;

    ModifierOperation operation = ModifierOperation::Add;
    int addAmount = 0;
    double multiplier = 1.0;

    int priority = 0;
};

struct ModifierContext {
    EffectType effectType = EffectType::Damage;

    EntityId source;
    EntityId target;
    bool hasTarget = true;

    CardId cardId;
    DiceCorruption diceCorruption;
    bool usesActorStats = false;

    bool preview = false;
};

struct ModifierBreakdownEntry {
    std::string sourceId;
    std::string description;
    int before = 0;
    int after = 0;
};

struct ModifiedValue {
    int base = 0;
    int modified = 0;
    std::vector<ModifierBreakdownEntry> breakdown;
};

struct ModifiedValueRange {
    int baseMin = 0;
    int baseMax = 0;
    int modifiedMin = 0;
    int modifiedMax = 0;
    std::vector<std::string> modifierLabels;
};

class IModifierProvider {
public:
    virtual ~IModifierProvider() = default;

    virtual void collectModifiers(
        const CombatState& state,
        const ModifierContext& context,
        std::vector<ValueModifier>& output
    ) const = 0;
};

class ModifierSystem {
public:
    ModifierSystem(
        const LocalizationManager& localization,
        const StatusDatabase& statusDatabase
    );

    void addProvider(const IModifierProvider& provider);
    void clearProviders();

    ModifiedValue modifyValue(
        const CombatState& state,
        int baseValue,
        const ModifierContext& context
    ) const;

    ModifiedValueRange modifyRange(
        const CombatState& state,
        int baseMin,
        int baseMax,
        const ModifierContext& context
    ) const;

private:
    std::vector<ValueModifier> collectModifiers(
        const CombatState& state,
        const ModifierContext& context
    ) const;

    void collectStatusModifiers(
        const CombatState& state,
        const ModifierContext& context,
        std::vector<ValueModifier>& output
    ) const;

    void collectModifiersFromEntity(
        const CombatState& state,
        const ModifierContext& context,
        EntityId entityId,
        StatusModifierEntity modifierEntity,
        std::vector<ValueModifier>& output
    ) const;

    static int applyModifier(int value, const ValueModifier& modifier);

private:
    const LocalizationManager& localization_;
    const StatusDatabase& statusDatabase_;
    std::vector<const IModifierProvider*> providers_;
};
