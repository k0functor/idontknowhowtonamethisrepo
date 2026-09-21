#include "ActiveItemAcquisitionSystem.hpp"

#include <algorithm>
#include <vector>

namespace {
template <typename Predicate>
std::optional<ActiveItemId> choose(
    const ActiveItemDatabase& items,
    const std::string& equippedItemId,
    Predicate predicate,
    Random& random
) {
    std::vector<const ActiveItemDefinition*> candidates;
    for (const ActiveItemDefinition* item : items.all()) {
        if (item != nullptr && item->id.value != equippedItemId && predicate(*item)) {
            candidates.push_back(item);
        }
    }
    if (candidates.empty()) {
        return std::nullopt;
    }
    const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(index)]->id;
}
}

std::optional<ActiveItemId> ActiveItemAcquisitionSystem::chooseReward(
    const ActiveItemDatabase& items,
    const std::string& equippedItemId,
    Random& random
) {
    return choose(items, equippedItemId, [](const ActiveItemDefinition& item) {
        return item.canAppearInRewards;
    }, random);
}

std::optional<ActiveItemId> ActiveItemAcquisitionSystem::chooseShopOffer(
    const ActiveItemDatabase& items,
    const std::string& equippedItemId,
    Random& random
) {
    return choose(items, equippedItemId, [](const ActiveItemDefinition& item) {
        return item.canAppearInShop && item.shopPrice > 0;
    }, random);
}

bool ActiveItemAcquisitionSystem::equipReplacement(
    RunState& run,
    const ActiveItemDatabase& items,
    const ActiveItemId& itemId
) {
    if (!items.contains(itemId) || run.activeItem.itemId == itemId.value) {
        return false;
    }
    const bool replaced = !run.activeItem.empty();
    const ActiveItemDefinition& definition = items.get(itemId);
    run.activeItem.itemId = itemId.value;
    run.activeItem.charge = std::clamp(definition.startingCharge, 0, definition.maxCharge);
    ++run.stats.activeItemsGained;
    if (replaced) {
        ++run.stats.activeItemsReplaced;
    }
    return true;
}
