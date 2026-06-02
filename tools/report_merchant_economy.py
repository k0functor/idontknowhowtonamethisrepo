#!/usr/bin/env python3
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
from statistics import mean
from typing import Any

ROOT = Path(__file__).resolve().parents[1]


def load_json(relative_path: str) -> Any:
    with (ROOT / relative_path).open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def find_merchant_archetype() -> dict[str, Any]:
    for archetype in load_json("data/archetypes/playable_archetypes.json"):
        if isinstance(archetype, dict) and archetype.get("id") == "merchant":
            return archetype
    raise RuntimeError("merchant archetype was not found")


def base_merchant_cards() -> list[dict[str, Any]]:
    cards = load_json("data/cards/merchant_cards.json")
    result: list[dict[str, Any]] = []
    for card in cards:
        if not isinstance(card, dict):
            continue
        rarity = card.get("rarity")
        if rarity in {"common", "uncommon", "rare"}:
            result.append(card)
    return result


def scaled_price(card: dict[str, Any], multiplier: float, minimum: int) -> int:
    return max(minimum, int(float(card.get("gold_cost", 0)) * multiplier + 0.5))


def print_price_bucket(title: str, values: list[int]) -> None:
    if not values:
        print(f"{title}: none")
        return
    print(
        f"{title}: count={len(values)}, min={min(values)}, avg={mean(values):.1f}, max={max(values)}"
    )


def main() -> int:
    merchant = find_merchant_archetype()
    rewards = load_json("data/rewards/reward_tables.json")
    shop = load_json("data/shop/shop_tables.json")
    cards = base_merchant_cards()

    gold_multiplier = float(rewards.get("merchant_gold_multiplier", 1.0))
    rest_multiplier = float(shop.get("merchant_rest_card_price_multiplier", 1.0))
    minimum_card_price = int(shop.get("minimum_card_price", 0))
    rest_offers = int(shop.get("merchant_rest_card_offers", 0))
    rest_purchases = int(shop.get("merchant_rest_max_card_purchases", 0))

    print("Merchant economy report")
    print(f"Starting gold: {merchant.get('starting_gold')}")
    print(f"Gold multiplier: {gold_multiplier:.2f}x")
    print(f"Merchant rest: {rest_offers} offers, {rest_purchases} purchases, {rest_multiplier:.2f}x card price")
    print()

    nodes = rewards.get("nodes", {}) if isinstance(rewards.get("nodes"), dict) else {}
    for node_type in ("combat", "elite", "boss"):
        node = nodes.get(node_type, {}) if isinstance(nodes.get(node_type), dict) else {}
        base_gold = int(node.get("gold", 0))
        merchant_gold = int(base_gold * gold_multiplier)
        print(f"{node_type.title()} gold: base={base_gold}, merchant={merchant_gold}, bonus={merchant_gold - base_gold}")
    print()

    rarities = Counter(str(card.get("rarity", "unknown")) for card in cards)
    print("Merchant reward card pool:")
    print(", ".join(f"{rarity}={rarities[rarity]}" for rarity in sorted(rarities)))
    print()

    shop_prices = [int(card.get("gold_cost", 0)) for card in cards]
    rest_prices = [scaled_price(card, rest_multiplier, minimum_card_price) for card in cards]
    print_price_bucket("Shop card prices", shop_prices)
    print_price_bucket("Merchant rest card prices", rest_prices)
    print()

    for rarity in ("common", "uncommon", "rare"):
        rarity_cards = [card for card in cards if card.get("rarity") == rarity]
        print_price_bucket(
            f"Merchant rest {rarity} prices",
            [scaled_price(card, rest_multiplier, minimum_card_price) for card in rarity_cards],
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
