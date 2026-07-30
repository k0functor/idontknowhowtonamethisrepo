#include "ActiveItemRerollSystem.hpp"

#include "rewards/RewardPoolRules.hpp"
#include "run/RunCardEligibility.hpp"

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace {
int rarityRank(const CardRarity rarity) {
    switch (rarity) {
        case CardRarity::Starter: return 0;
        case CardRarity::Common: return 1;
        case CardRarity::Uncommon: return 2;
        case CardRarity::Rare: return 3;
        case CardRarity::Special: return 4;
    }
    return 0;
}

bool meetsMinimumRarity(const CardDefinition& card, const std::optional<CardRarity>& minimum) {
    return !minimum.has_value() || rarityRank(card.rarity) >= rarityRank(*minimum);
}

template <typename T>
const T* takeRandom(std::vector<const T*>& candidates, Random& random) {
    if (candidates.empty()) {
        return nullptr;
    }

    const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    const T* picked = candidates[static_cast<std::size_t>(index)];
    candidates.erase(candidates.begin() + index);
    return picked;
}

std::set<std::string> rewardCardIds(const RewardOption& option) {
    std::set<std::string> result;
    for (const CardRewardOption& card : option.cardOptions) {
        result.insert(card.cardId.value);
    }
    return result;
}

std::set<std::string> currentShopIds(const ShopState& shop, const ShopOfferType type) {
    std::set<std::string> result;
    for (const ShopOffer& offer : shop.offers) {
        if (offer.type == type && !offer.contentId.empty()) {
            result.insert(offer.contentId);
        }
    }
    return result;
}
}

ActiveItemRerollResult ActiveItemRerollSystem::rerollReward(
    RewardState& reward,
    const RunState& run,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const ActiveItemDatabase& activeItems,
    const NodeRewardTuning& tuning,
    Random& random
) {
    ActiveItemRerollResult result;
    std::set<std::string> reservedRelics(run.relicIds.begin(), run.relicIds.end());

    for (const RewardOption& option : reward.options) {
        if (option.type == RewardOptionType::Relic && !option.relicId.empty()) {
            reservedRelics.insert(option.relicId);
        }
    }

    for (RewardOption& option : reward.options) {
        switch (option.type) {
            case RewardOptionType::Gold:
                break;

            case RewardOptionType::CardChoice: {
                const std::size_t requested = option.cardOptions.size();
                if (requested == 0u) {
                    break;
                }

                const std::set<std::string> oldIds = rewardCardIds(option);
                std::vector<const CardDefinition*> candidates;
                for (const CardDefinition* card : cards.all()) {
                    if (card != nullptr &&
                        !oldIds.contains(card->id.value) &&
                        RewardPoolRules::canAppearAsCardReward(*card) &&
                        runCanReceiveArchetypeRewardCard(run, *card) &&
                        meetsMinimumRarity(*card, tuning.minimumCardRarity)) {
                        candidates.push_back(card);
                    }
                }

                if (candidates.size() < requested) {
                    break;
                }

                std::vector<CardRewardOption> replacement;
                replacement.reserve(requested);
                for (std::size_t i = 0; i < requested; ++i) {
                    const CardDefinition* picked = takeRandom(candidates, random);
                    if (picked == nullptr) {
                        replacement.clear();
                        break;
                    }
                    replacement.push_back(CardRewardOption{picked->id});
                }

                if (replacement.size() == requested) {
                    option.cardOptions = std::move(replacement);
                    ++result.cardOffersChanged;
                }
                break;
            }

            case RewardOptionType::Consumable: {
                std::vector<const ConsumableDefinition*> candidates;
                for (const ConsumableDefinition* consumable : consumables.all()) {
                    if (consumable != nullptr && consumable->id.value != option.consumableId) {
                        candidates.push_back(consumable);
                    }
                }

                const ConsumableDefinition* picked = takeRandom(candidates, random);
                if (picked != nullptr) {
                    option.consumableId = picked->id.value;
                    ++result.consumableOffersChanged;
                }
                break;
            }

            case RewardOptionType::Relic: {
                const std::string oldId = option.relicId;
                reservedRelics.erase(oldId);

                std::vector<const RelicDefinition*> candidates;
                for (const RelicDefinition* relic : relics.all()) {
                    if (relic != nullptr &&
                        relic->id.value != oldId &&
                        !reservedRelics.contains(relic->id.value) &&
                        RewardPoolRules::canAppearAsRelicReward(*relic)) {
                        candidates.push_back(relic);
                    }
                }

                const RelicDefinition* picked = takeRandom(candidates, random);
                if (picked != nullptr) {
                    option.relicId = picked->id.value;
                    reservedRelics.insert(option.relicId);
                    ++result.relicOffersChanged;
                } else if (!oldId.empty()) {
                    reservedRelics.insert(oldId);
                }
                break;
            }

            case RewardOptionType::ActiveItem: {
                std::vector<const ActiveItemDefinition*> candidates;
                for (const ActiveItemDefinition* item : activeItems.all()) {
                    if (item != nullptr && item->canAppearInRewards &&
                        item->id.value != option.activeItemId &&
                        item->id.value != run.activeItem.itemId) {
                        candidates.push_back(item);
                    }
                }
                const ActiveItemDefinition* picked = takeRandom(candidates, random);
                if (picked != nullptr) {
                    option.activeItemId = picked->id.value;
                    ++result.activeItemOffersChanged;
                }
                break;
            }
        }
    }

    return result;
}

ActiveItemRerollResult ActiveItemRerollSystem::rerollShop(
    ShopState& shop,
    const RunState& run,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const ActiveItemDatabase& activeItems,
    const int minimumCardPrice,
    const int minimumConsumablePrice,
    const std::function<int(RelicRarity)>& relicPrice,
    Random& random
) {
    ActiveItemRerollResult result;
    if (shop.isMerchantRest()) {
        return result;
    }

    const std::set<std::string> oldCards = currentShopIds(shop, ShopOfferType::Card);
    const std::set<std::string> oldRelics = currentShopIds(shop, ShopOfferType::Relic);
    const std::set<std::string> oldConsumables = currentShopIds(shop, ShopOfferType::Consumable);
    const std::set<std::string> oldActiveItems = currentShopIds(shop, ShopOfferType::ActiveItem);

    std::set<std::string> reservedCards;
    std::set<std::string> reservedRelics(run.relicIds.begin(), run.relicIds.end());
    std::set<std::string> reservedConsumables;
    std::set<std::string> reservedActiveItems;

    for (ShopOffer& offer : shop.offers) {
        if (offer.purchased || offer.type == ShopOfferType::CardRemoval) {
            continue;
        }

        switch (offer.type) {
            case ShopOfferType::Card: {
                std::vector<const CardDefinition*> candidates;
                for (const CardDefinition* card : cards.all()) {
                    if (card != nullptr &&
                        !oldCards.contains(card->id.value) &&
                        !reservedCards.contains(card->id.value) &&
                        RewardPoolRules::canAppearInShop(*card) &&
                        runCanReceiveCard(run, *card)) {
                        candidates.push_back(card);
                    }
                }

                const CardDefinition* picked = takeRandom(candidates, random);
                if (picked != nullptr) {
                    offer.contentId = picked->id.value;
                    offer.price = std::max(minimumCardPrice, picked->goldCost);
                    reservedCards.insert(offer.contentId);
                    ++result.cardOffersChanged;
                }
                break;
            }

            case ShopOfferType::Relic: {
                std::vector<const RelicDefinition*> candidates;
                for (const RelicDefinition* relic : relics.all()) {
                    if (relic != nullptr &&
                        !oldRelics.contains(relic->id.value) &&
                        !reservedRelics.contains(relic->id.value) &&
                        RewardPoolRules::canAppearInShop(*relic)) {
                        candidates.push_back(relic);
                    }
                }

                const RelicDefinition* picked = takeRandom(candidates, random);
                if (picked != nullptr) {
                    offer.contentId = picked->id.value;
                    offer.price = relicPrice(picked->rarity);
                    reservedRelics.insert(offer.contentId);
                    ++result.relicOffersChanged;
                }
                break;
            }

            case ShopOfferType::Consumable: {
                std::vector<const ConsumableDefinition*> candidates;
                for (const ConsumableDefinition* consumable : consumables.all()) {
                    if (consumable != nullptr &&
                        !oldConsumables.contains(consumable->id.value) &&
                        !reservedConsumables.contains(consumable->id.value)) {
                        candidates.push_back(consumable);
                    }
                }

                const ConsumableDefinition* picked = takeRandom(candidates, random);
                if (picked != nullptr) {
                    offer.contentId = picked->id.value;
                    offer.price = std::max(minimumConsumablePrice, picked->goldCost);
                    reservedConsumables.insert(offer.contentId);
                    ++result.consumableOffersChanged;
                }
                break;
            }

            case ShopOfferType::ActiveItem: {
                std::vector<const ActiveItemDefinition*> candidates;
                for (const ActiveItemDefinition* item : activeItems.all()) {
                    if (item != nullptr && item->canAppearInShop && item->shopPrice > 0 &&
                        !oldActiveItems.contains(item->id.value) &&
                        !reservedActiveItems.contains(item->id.value) &&
                        item->id.value != run.activeItem.itemId) {
                        candidates.push_back(item);
                    }
                }
                const ActiveItemDefinition* picked = takeRandom(candidates, random);
                if (picked != nullptr) {
                    offer.contentId = picked->id.value;
                    offer.price = picked->shopPrice;
                    reservedActiveItems.insert(offer.contentId);
                    ++result.activeItemOffersChanged;
                }
                break;
            }

            case ShopOfferType::CardRemoval:
                break;
        }
    }

    return result;
}
