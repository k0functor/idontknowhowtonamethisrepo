#include "shop/ShopGenerator.hpp"

#include "active_items/ActiveItemAcquisitionSystem.hpp"
#include "active_items/ActiveItemDatabase.hpp"
#include "cards/CardDefinition.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicDefinition.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "rewards/CardRewardQuality.hpp"
#include "run/RunCardEligibility.hpp"
#include "run/RunState.hpp"
#include "shop/ShopEconomy.hpp"
#include "shop/ShopTuning.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace {
template <typename T>
void pickUniqueRandom(
    std::vector<const T*>& candidates,
    const int count,
    Random& random,
    std::vector<const T*>& out
) {
    for (int index = 0; index < count && !candidates.empty(); ++index) {
        const int pickedIndex = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
        out.push_back(candidates[static_cast<std::size_t>(pickedIndex)]);
        candidates.erase(candidates.begin() + pickedIndex);
    }
}

void ensureAffordableCardOffers(
    ShopState& shop,
    const RunState& run,
    const ShopTuning& tuning
) {
    if (run.gold < tuning.minimumCardPrice() || tuning.affordableCardOfferCount() <= 0) {
        return;
    }

    std::vector<ShopOffer*> cardOffers;
    for (ShopOffer& offer : shop.offers) {
        if (offer.type == ShopOfferType::Card && !offer.purchased) {
            cardOffers.push_back(&offer);
        }
    }

    const int alreadyAffordable = static_cast<int>(std::count_if(
        cardOffers.begin(),
        cardOffers.end(),
        [&run](const ShopOffer* offer) { return offer != nullptr && offer->price <= run.gold; }
    ));
    int needed = std::min<int>(
        tuning.affordableCardOfferCount() - alreadyAffordable,
        static_cast<int>(cardOffers.size())
    );
    if (needed <= 0) {
        return;
    }

    std::sort(cardOffers.begin(), cardOffers.end(), [](const ShopOffer* lhs, const ShopOffer* rhs) {
        return lhs->price < rhs->price;
    });

    const int cap = ShopEconomy::affordableCardPriceCap(
        run.gold,
        tuning.affordableCardPriceCapPercent(),
        tuning.minimumCardPrice()
    );
    for (ShopOffer* offer : cardOffers) {
        if (needed <= 0) {
            break;
        }
        if (offer != nullptr && offer->price > run.gold) {
            offer->price = std::min(offer->price, cap);
            --needed;
        }
    }
}
}

ShopState ShopGenerator::createShop(
    const RunState& run,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const ActiveItemDatabase& activeItems,
    const ShopTuning& tuning,
    Random& random
) {
    ShopState shop;
    shop.cardRemovalPrice = tuning.cardRemovalPrice(run.currentFloorIndex, run.stats.cardsRemoved);

    std::vector<const CardDefinition*> cardCandidates;
    for (const CardDefinition* card : cards.all()) {
        if (card != nullptr && RewardPoolRules::canAppearInShop(*card) && runCanReceiveCard(run, *card)) {
            cardCandidates.push_back(card);
        }
    }

    std::vector<const CardDefinition*> deck;
    deck.reserve(run.deckCardIds.size());
    for (const CardId& cardId : run.deckCardIds) {
        if (cards.contains(cardId)) {
            deck.push_back(&cards.get(cardId));
        }
    }
    const std::vector<const CardDefinition*> pickedCards = CardRewardQuality::chooseOffers(
        std::move(cardCandidates),
        deck,
        tuning.cardOfferCount(),
        random
    );
    for (const CardDefinition* card : pickedCards) {
        ShopOffer offer;
        offer.type = ShopOfferType::Card;
        offer.contentId = card->id.value;
        offer.price = tuning.cardPrice(card->goldCost, run.currentFloorIndex);
        shop.offers.push_back(offer);
    }

    std::vector<const RelicDefinition*> relicCandidates;
    for (const RelicDefinition* relic : relics.all()) {
        if (relic == nullptr || !RewardPoolRules::canAppearInShop(*relic, run.archetypeMechanicId)) {
            continue;
        }

        const bool alreadyOwned = std::find(
            run.relicIds.begin(),
            run.relicIds.end(),
            relic->id.value
        ) != run.relicIds.end();
        if (!alreadyOwned) {
            relicCandidates.push_back(relic);
        }
    }

    std::vector<const RelicDefinition*> pickedRelics;
    pickUniqueRandom(relicCandidates, tuning.relicOfferCount(), random, pickedRelics);
    for (const RelicDefinition* relic : pickedRelics) {
        ShopOffer offer;
        offer.type = ShopOfferType::Relic;
        offer.contentId = relic->id.value;
        offer.price = tuning.relicPrice(relic->rarity, run.currentFloorIndex);
        shop.offers.push_back(offer);
    }

    std::vector<const ConsumableDefinition*> consumableCandidates;
    for (const ConsumableDefinition* consumable : consumables.all()) {
        if (consumable != nullptr) {
            consumableCandidates.push_back(consumable);
        }
    }

    std::vector<const ConsumableDefinition*> pickedConsumables;
    pickUniqueRandom(consumableCandidates, tuning.consumableOfferCount(), random, pickedConsumables);
    for (const ConsumableDefinition* consumable : pickedConsumables) {
        ShopOffer offer;
        offer.type = ShopOfferType::Consumable;
        offer.contentId = consumable->id.value;
        offer.price = tuning.consumablePrice(consumable->goldCost, run.currentFloorIndex);
        shop.offers.push_back(offer);
    }

    if (tuning.activeItemOfferChancePercent() > 0 &&
        random.chance(static_cast<double>(tuning.activeItemOfferChancePercent()) / 100.0)) {
        const std::optional<ActiveItemId> activeItem = ActiveItemAcquisitionSystem::chooseShopOffer(
            activeItems,
            run.activeItem.itemId,
            random
        );
        if (activeItem.has_value()) {
            const ActiveItemDefinition& definition = activeItems.get(*activeItem);
            ShopOffer offer;
            offer.type = ShopOfferType::ActiveItem;
            offer.contentId = definition.id.value;
            offer.price = definition.shopPrice;
            shop.offers.push_back(offer);
        }
    }

    ensureAffordableCardOffers(shop, run, tuning);

    ShopOffer removal;
    removal.type = ShopOfferType::CardRemoval;
    removal.price = shop.cardRemovalPrice;
    shop.offers.push_back(removal);

    return shop;
}

ShopState ShopGenerator::createMerchantRest(
    const RunState& run,
    const CardDatabase& cards,
    const ShopTuning& tuning,
    Random& random
) {
    ShopState state;
    state.mode = ShopStateMode::MerchantRest;
    state.maxCardPurchases = tuning.merchantRestMaxCardPurchases();
    state.cardPurchasesMade = 0;

    std::vector<const CardDefinition*> candidates;
    for (const CardDefinition* card : cards.all()) {
        if (card != nullptr &&
            RewardPoolRules::canAppearAsCardReward(*card) &&
            runCanReceiveArchetypeRewardCard(run, *card)) {
            candidates.push_back(card);
        }
    }

    std::vector<const CardDefinition*> deck;
    deck.reserve(run.deckCardIds.size());
    for (const CardId& cardId : run.deckCardIds) {
        if (cards.contains(cardId)) {
            deck.push_back(&cards.get(cardId));
        }
    }
    const std::vector<const CardDefinition*> pickedCards = CardRewardQuality::chooseOffers(
        std::move(candidates),
        deck,
        tuning.merchantRestCardOfferCount(),
        random
    );
    for (const CardDefinition* card : pickedCards) {
        ShopOffer offer;
        offer.type = ShopOfferType::Card;
        offer.contentId = card->id.value;
        const double scaledPrice = static_cast<double>(
            tuning.cardPrice(card->goldCost, run.currentFloorIndex)
        ) * tuning.merchantRestCardPriceMultiplier();
        offer.price = std::max(tuning.minimumCardPrice(), static_cast<int>(scaledPrice + 0.5));
        state.offers.push_back(offer);
    }

    return state;
}
