#include "ProfileProgressionService.hpp"

#include "achievements/AchievementDefinition.hpp"
#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "archetypes/PlayableArchetypeId.hpp"
#include "cards/CardDefinition.hpp"
#include "challenges/ChallengeDefinition.hpp"
#include "combat/CombatResult.hpp"
#include "cards/CardId.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "consumables/ConsumableId.hpp"
#include "data/ContentRegistry.hpp"
#include "enemies/EnemyDefinition.hpp"
#include "enemies/EnemyId.hpp"
#include "events/RunEventChoiceResult.hpp"
#include "localization/LocalizationManager.hpp"
#include "profile/ProfileManager.hpp"
#include "progression/AchievementEvaluator.hpp"
#include "progression/ChallengeEvaluator.hpp"
#include "progression/UnlockEvaluator.hpp"
#include "relics/RelicDefinition.hpp"
#include "relics/RelicId.hpp"
#include "rewards/RewardSelection.hpp"
#include "run/RunState.hpp"
#include "shop/ShopState.hpp"
#include "statuses/StatusDefinition.hpp"
#include "statuses/StatusId.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

ProfileProgressionService::ProfileProgressionService(
    const ContentRegistry& content,
    const LocalizationManager& localization,
    ProfileManager& profileManager
)
    : content_(content),
      localization_(localization),
      profileManager_(profileManager) {
}

void ProfileProgressionService::recordCombatStats(const CombatResult& result) {
    for (const std::string& cardId : result.playedCardIds) {
        if (content_.cards().contains(CardId(cardId))) {
            (void)profileManager_.recordSelectedCardPlayed(cardId);
        }
    }
}

void ProfileProgressionService::discoverCombatContent(const CombatResult& result) {
    for (const std::string& enemyId : result.encounteredEnemyIds) {
        if (content_.enemies().contains(EnemyId(enemyId))) {
            discoverEnemyAndToast(enemyId);
        }
    }

    for (const std::string& statusId : result.statusIdsSeen) {
        if (content_.statuses().contains(StatusId(statusId))) {
            discoverStatusAndToast(statusId);
        }
    }
}

void ProfileProgressionService::synchronizeRunContent(const RunState& run) {
    // Loaded runs are synchronized silently. Reward and encounter paths use toast helpers.
    for (const CardId& cardId : run.deckCardIds) {
        if (content_.cards().contains(cardId)) {
            profileManager_.unlockSelectedCard(cardId.value);
        }
    }

    for (const std::string& relicId : run.relicIds) {
        if (content_.relics().contains(RelicId(relicId))) {
            profileManager_.unlockSelectedRelic(relicId);
        }
    }

    for (const RunActorState& actor : run.actorStates) {
        for (const std::string& relicId : actor.relicIds) {
            if (content_.relics().contains(RelicId(relicId))) {
                profileManager_.unlockSelectedRelic(relicId);
            }
        }
    }

    for (const std::string& consumableId : run.consumableIds) {
        if (content_.consumables().contains(ConsumableId(consumableId))) {
            profileManager_.discoverSelectedConsumable(consumableId);
        }
    }
}

void ProfileProgressionService::applyRewardSelection(const RewardSelection& selection) {
    for (const CardId& cardId : selection.selectedCardIds) {
        if (content_.cards().contains(cardId)) {
            unlockCardAndToast(cardId.value);
        }
    }

    for (const std::string& consumableId : selection.selectedConsumableIds) {
        if (content_.consumables().contains(ConsumableId(consumableId))) {
            discoverConsumableAndToast(consumableId);
        }
    }

    for (const std::string& relicId : selection.selectedRelicIds) {
        if (content_.relics().contains(RelicId(relicId))) {
            unlockRelicAndToast(relicId);
            (void)profileManager_.recordSelectedRelicTaken(relicId);
        }
    }

    for (const RelicRewardSelection& relic : selection.selectedRelics) {
        if (content_.relics().contains(RelicId(relic.relicId))) {
            unlockRelicAndToast(relic.relicId);
            (void)profileManager_.recordSelectedRelicTaken(relic.relicId);
        }
    }
}

void ProfileProgressionService::applyShopPurchase(const ShopPurchase& purchase) {
    if (purchase.type == ShopOfferType::Card && content_.cards().contains(CardId(purchase.contentId))) {
        unlockCardAndToast(purchase.contentId);
        return;
    }

    if (purchase.type == ShopOfferType::Relic && content_.relics().contains(RelicId(purchase.contentId))) {
        unlockRelicAndToast(purchase.contentId);
        (void)profileManager_.recordSelectedRelicTaken(purchase.contentId);
        return;
    }

    if (purchase.type == ShopOfferType::Consumable && content_.consumables().contains(ConsumableId(purchase.contentId))) {
        discoverConsumableAndToast(purchase.contentId);
    }
}

void ProfileProgressionService::applyEventOutcome(const RunEventChoiceResult& result) {
    for (const RunEventOutcomeEntry& outcome : result.outcomes) {
        if (outcome.type == RunEventOutcomeType::CardGained && content_.cards().contains(CardId(outcome.contentId))) {
            unlockCardAndToast(outcome.contentId);
            continue;
        }

        if (outcome.type == RunEventOutcomeType::RelicGained && content_.relics().contains(RelicId(outcome.contentId))) {
            unlockRelicAndToast(outcome.contentId);
            (void)profileManager_.recordSelectedRelicTaken(outcome.contentId);
            continue;
        }

        if (outcome.type == RunEventOutcomeType::ConsumableGained &&
            content_.consumables().contains(ConsumableId(outcome.contentId))) {
            discoverConsumableAndToast(outcome.contentId);
        }
    }
}

void ProfileProgressionService::grantFloorCompletionUnlocks(const RunState& run) {
    if (run.currentFloorId == "floor1") {
        (void)unlockArchetypeAndToast("replicant");
        (void)unlockArchetypeAndToast("monk");
    }

    if (run.currentFloorId == "floor2") {
        (void)unlockArchetypeAndToast("merchant");
        (void)unlockArchetypeAndToast("lost_psychopath");
    }

    if (run.currentFloorId == "floor3") {
        (void)unlockArchetypeAndToast("sadist_masochist");
    }

    completeEligibleChallenges(run);
    completeEligibleAchievements(&run);
}

bool ProfileProgressionService::unlockArchetypeAndToast(const std::string& archetypeId) {
    if (!content_.archetypes().contains(PlayableArchetypeId(archetypeId))) {
        return false;
    }

    if (!profileManager_.unlockSelectedArchetype(archetypeId)) {
        return false;
    }

    const PlayableArchetypeDefinition& archetype = content_.archetypes().get(PlayableArchetypeId(archetypeId));
    pushToast(localization_.format(
        TextId("profile_toast.archetype_unlocked"),
        {{"name", localization_.get(archetype.nameTextId)}}
    ));
    return true;
}

bool ProfileProgressionService::unlockCardAndToast(const std::string& cardId) {
    if (!content_.cards().contains(CardId(cardId))) {
        return false;
    }

    if (!profileManager_.unlockSelectedCard(cardId)) {
        return false;
    }

    const CardDefinition& card = content_.cards().get(CardId(cardId));
    pushToast(localization_.format(
        TextId("profile_toast.card_unlocked"),
        {{"name", localization_.get(card.nameTextId)}}
    ));
    return true;
}

bool ProfileProgressionService::unlockRelicAndToast(const std::string& relicId) {
    if (!content_.relics().contains(RelicId(relicId))) {
        return false;
    }

    if (!profileManager_.unlockSelectedRelic(relicId)) {
        return false;
    }

    const RelicDefinition& relic = content_.relics().get(RelicId(relicId));
    pushToast(localization_.format(
        TextId("profile_toast.relic_unlocked"),
        {{"name", localization_.get(relic.nameTextId)}}
    ));
    return true;
}

bool ProfileProgressionService::discoverEnemyAndToast(const std::string& enemyId) {
    if (!content_.enemies().contains(EnemyId(enemyId))) {
        return false;
    }

    if (!profileManager_.discoverSelectedEnemy(enemyId)) {
        return false;
    }

    const EnemyDefinition& enemy = content_.enemies().get(EnemyId(enemyId));
    pushToast(localization_.format(
        TextId("profile_toast.enemy_discovered"),
        {{"name", localization_.get(enemy.nameTextId)}}
    ));
    return true;
}

bool ProfileProgressionService::discoverStatusAndToast(const std::string& statusId) {
    if (!content_.statuses().contains(StatusId(statusId))) {
        return false;
    }

    if (!profileManager_.discoverSelectedStatus(statusId)) {
        return false;
    }

    const StatusDefinition& status = content_.statuses().get(StatusId(statusId));
    pushToast(localization_.format(
        TextId("profile_toast.status_discovered"),
        {{"name", localization_.get(status.nameTextId)}}
    ));
    return true;
}

bool ProfileProgressionService::discoverConsumableAndToast(const std::string& consumableId) {
    if (!content_.consumables().contains(ConsumableId(consumableId))) {
        return false;
    }

    if (!profileManager_.discoverSelectedConsumable(consumableId)) {
        return false;
    }

    const ConsumableDefinition& consumable = content_.consumables().get(ConsumableId(consumableId));
    pushToast(localization_.format(
        TextId("profile_toast.consumable_discovered"),
        {{"name", localization_.get(consumable.nameTextId)}}
    ));
    return true;
}

void ProfileProgressionService::applyUnlockRewardAndToast(const UnlockReward& reward) {
    for (const std::string& archetypeId : reward.archetypeIds) {
        (void)unlockArchetypeAndToast(archetypeId);
    }
    for (const std::string& cardId : reward.cardIds) {
        (void)unlockCardAndToast(cardId);
    }
    for (const std::string& relicId : reward.relicIds) {
        (void)unlockRelicAndToast(relicId);
    }
}

void ProfileProgressionService::pushToast(std::string message) {
    if (message.empty()) {
        return;
    }

    constexpr std::size_t maxToasts = 6u;
    toasts_.push_back(ProfileToast{std::move(message), 4.8f});
    if (toasts_.size() > maxToasts) {
        toasts_.erase(
            toasts_.begin(),
            toasts_.begin() + static_cast<std::ptrdiff_t>(toasts_.size() - maxToasts)
        );
    }
}

void ProfileProgressionService::update(const float deltaSeconds) {
    for (ProfileToast& toast : toasts_) {
        toast.remainingSeconds -= deltaSeconds;
    }

    toasts_.erase(
        std::remove_if(toasts_.begin(), toasts_.end(), [](const ProfileToast& toast) {
            return toast.remainingSeconds <= 0.f;
        }),
        toasts_.end()
    );
}

const std::vector<ProfileToast>& ProfileProgressionService::toasts() const {
    return toasts_;
}

void ProfileProgressionService::completeEligibleChallenges(const RunState& run) {
    const ProfileData* profile = profileManager_.selectedProfile();
    if (profile == nullptr) {
        return;
    }

    const std::vector<std::string> completedIds = ChallengeEvaluator::findNewlyCompleted(
        content_.challenges(),
        *profile,
        run
    );
    const std::vector<std::string> newlyCompleted = profileManager_.completeSelectedChallenges(completedIds);

    for (const std::string& challengeId : newlyCompleted) {
        if (!content_.challenges().contains(challengeId)) {
            continue;
        }
        const ChallengeDefinition& challenge = content_.challenges().get(challengeId);
        pushToast(localization_.format(
            TextId("profile_toast.challenge_completed"),
            {{"name", localization_.get(challenge.nameTextId)}}
        ));
    }

    applyUnlockRewardAndToast(UnlockEvaluator::collectChallengeRewards(content_.challenges(), newlyCompleted));
}

void ProfileProgressionService::completeEligibleAchievements(const RunState* run) {
    const ProfileData* profile = profileManager_.selectedProfile();
    if (profile == nullptr) {
        return;
    }

    const std::vector<std::string> completedIds = AchievementEvaluator::findNewlyCompleted(
        content_.achievements(),
        *profile,
        run
    );
    const std::vector<std::string> newlyCompleted = profileManager_.completeSelectedAchievements(completedIds);

    for (const std::string& achievementId : newlyCompleted) {
        if (!content_.achievements().contains(achievementId)) {
            continue;
        }
        const AchievementDefinition& achievement = content_.achievements().get(achievementId);
        pushToast(localization_.format(
            TextId("profile_toast.achievement_completed"),
            {{"name", localization_.get(achievement.nameTextId)}}
        ));
    }

    applyUnlockRewardAndToast(UnlockEvaluator::collectAchievementRewards(content_.achievements(), newlyCompleted));
}
