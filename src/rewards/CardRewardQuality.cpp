#include "rewards/CardRewardQuality.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardKeyword.hpp"
#include "core/Random.hpp"
#include "effects/EffectType.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace {
constexpr std::uint32_t damageTheme = 1u << 0u;
constexpr std::uint32_t blockTheme = 1u << 1u;
constexpr std::uint32_t sustainTheme = 1u << 2u;
constexpr std::uint32_t drawTheme = 1u << 3u;
constexpr std::uint32_t discardTheme = 1u << 4u;
constexpr std::uint32_t energyTheme = 1u << 5u;
constexpr std::uint32_t stressTheme = 1u << 6u;
constexpr std::uint32_t statusTheme = 1u << 7u;
constexpr std::uint32_t stanceTheme = 1u << 8u;
constexpr std::uint32_t droneTheme = 1u << 9u;
constexpr std::uint32_t exhaustTheme = 1u << 10u;
constexpr std::uint32_t multiHitTheme = 1u << 11u;

bool hasKeyword(const CardDefinition& card, const CardKeyword keyword) {
    return std::find(card.keywords.begin(), card.keywords.end(), keyword) != card.keywords.end();
}

std::set<std::string> statusIds(const CardDefinition& card) {
    std::set<std::string> result;
    for (const EffectDefinition& effect : card.effects) {
        if (effect.statusId.has_value()) {
            result.insert(*effect.statusId);
        }
        if (effect.scaling.statusId.has_value()) {
            result.insert(*effect.scaling.statusId);
        }
    }
    return result;
}

int themeCount(const std::vector<const CardDefinition*>& cards, const std::uint32_t theme) {
    return static_cast<int>(std::count_if(cards.begin(), cards.end(), [theme](const CardDefinition* card) {
        return card != nullptr && (CardRewardQuality::themeMask(*card) & theme) != 0u;
    }));
}

int exactCopies(const CardDefinition& candidate, const std::vector<const CardDefinition*>& deck) {
    return static_cast<int>(std::count_if(deck.begin(), deck.end(), [&candidate](const CardDefinition* card) {
        return card != nullptr && card->id == candidate.id;
    }));
}

int rarityBonus(const CardRarity rarity) {
    switch (rarity) {
        case CardRarity::Starter: return -20;
        case CardRarity::Common: return 0;
        case CardRarity::Uncommon: return 2;
        case CardRarity::Rare: return 4;
        case CardRarity::Special: return -20;
    }
    return 0;
}
}

std::uint32_t CardRewardQuality::themeMask(const CardDefinition& card) {
    std::uint32_t result = 0u;
    for (const EffectDefinition& effect : card.effects) {
        switch (effect.type) {
            case EffectType::Damage:
                result |= damageTheme;
                if (effect.repeatCount > 1) result |= multiHitTheme;
                break;
            case EffectType::Block: result |= blockTheme; break;
            case EffectType::Heal: result |= sustainTheme; break;
            case EffectType::DrawCards:
            case EffectType::RecoverCards: result |= drawTheme; break;
            case EffectType::DiscardCards: result |= discardTheme; break;
            case EffectType::GainEnergy:
            case EffectType::LoseEnergy: result |= energyTheme; break;
            case EffectType::GainStress:
            case EffectType::LoseStress:
            case EffectType::SpendStressDamage:
            case EffectType::SpendStressBlock:
            case EffectType::SpendStressEnergy:
            case EffectType::SpendStressDraw:
            case EffectType::PrimeStressBreakdown: result |= stressTheme; break;
            case EffectType::ApplyStatus: result |= statusTheme; break;
            case EffectType::EnterStance: result |= stanceTheme; break;
            case EffectType::SummonDrone:
            case EffectType::UseDrone: result |= droneTheme; break;
            case EffectType::LoseHp: result |= damageTheme; break;
        }
    }
    if (hasKeyword(card, CardKeyword::Exhaust)) {
        result |= exhaustTheme;
    }
    return result;
}

int CardRewardQuality::relevanceScore(
    const CardDefinition& candidate,
    const std::vector<const CardDefinition*>& deck
) {
    const std::uint32_t candidateThemes = themeMask(candidate);
    int score = rarityBonus(candidate.rarity) - exactCopies(candidate, deck) * 14;

    for (unsigned bit = 0u; bit < 12u; ++bit) {
        const std::uint32_t theme = 1u << bit;
        if ((candidateThemes & theme) == 0u) {
            continue;
        }
        score += std::min(4, themeCount(deck, theme)) * 3;
    }

    const int deckSize = static_cast<int>(deck.size());
    if ((candidateThemes & damageTheme) != 0u && themeCount(deck, damageTheme) * 3 < std::max(1, deckSize)) {
        score += 8;
    }
    if ((candidateThemes & blockTheme) != 0u && themeCount(deck, blockTheme) * 3 < std::max(1, deckSize)) {
        score += 8;
    }
    if ((candidateThemes & sustainTheme) != 0u && themeCount(deck, sustainTheme) == 0) {
        score += 4;
    }

    const std::set<std::string> candidateStatuses = statusIds(candidate);
    for (const CardDefinition* deckCard : deck) {
        if (deckCard == nullptr) {
            continue;
        }
        const std::set<std::string> deckStatuses = statusIds(*deckCard);
        for (const std::string& status : candidateStatuses) {
            if (deckStatuses.contains(status)) {
                score += 7;
            }
        }
    }

    const bool candidateUsesDrone = std::any_of(
        candidate.effects.begin(),
        candidate.effects.end(),
        [](const EffectDefinition& effect) { return effect.type == EffectType::UseDrone; }
    );
    if (candidateUsesDrone && themeCount(deck, droneTheme) == 0) {
        score -= 24;
    }

    return score;
}

std::vector<const CardDefinition*> CardRewardQuality::chooseOffers(
    std::vector<const CardDefinition*> candidates,
    const std::vector<const CardDefinition*>& deck,
    const int count,
    Random& random
) {
    candidates.erase(
        std::remove(candidates.begin(), candidates.end(), nullptr),
        candidates.end()
    );

    std::vector<const CardDefinition*> result;
    result.reserve(static_cast<std::size_t>(std::max(0, count)));

    while (!candidates.empty() && static_cast<int>(result.size()) < count) {
        int bestScore = std::numeric_limits<int>::min();
        std::size_t bestIndex = 0u;

        for (std::size_t index = 0u; index < candidates.size(); ++index) {
            const CardDefinition& candidate = *candidates[index];
            int score = relevanceScore(candidate, deck);

            const std::uint32_t themes = themeMask(candidate);
            for (const CardDefinition* selected : result) {
                if (selected == nullptr) {
                    continue;
                }
                const int overlap = std::popcount(themes & themeMask(*selected));
                score -= overlap * 5;
            }

            if (result.empty()) {
                score *= 2;
            } else {
                score += random.rangeInclusive(0, 10);
            }

            if (score > bestScore) {
                bestScore = score;
                bestIndex = index;
            }
        }

        result.push_back(candidates[bestIndex]);
        candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(bestIndex));
    }

    return result;
}
