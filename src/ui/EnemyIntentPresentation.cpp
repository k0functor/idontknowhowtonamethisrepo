#include "EnemyIntentPresentation.hpp"

EnemyIntentPresentation summarizeEnemyIntent(const EnemyIntent& intent) {
    EnemyIntentPresentation presentation;

    for (const EnemyIntentEffectSummary& effect : intent.effectSummaries) {
        if (effect.target == EffectTarget::AllAllies) {
            presentation.affectsAllPlayers = true;
            presentation.affectsMultipleTargets = true;
        }

        if (effect.target == EffectTarget::AllEnemies) {
            presentation.affectsEnemyTeam = true;
            presentation.affectsMultipleTargets = true;
        }
    }

    return presentation;
}
