#!/usr/bin/env python3
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"


@dataclass(frozen=True)
class CardInfo:
    id: str
    owner_actor: str
    reward_pool: str
    rarity: str
    has_upgrade: bool
    source: str


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def load_cards() -> dict[str, CardInfo]:
    cards: dict[str, CardInfo] = {}
    for path in sorted((DATA_DIR / "cards").glob("*.json")):
        data = load_json(path)
        if not isinstance(data, list):
            continue
        for item in data:
            if not isinstance(item, dict):
                continue
            card_id = item.get("id")
            if not isinstance(card_id, str) or not card_id:
                continue
            owner_actor = item.get("owner_actor", "") if isinstance(item.get("owner_actor", ""), str) else ""
            reward_pool = item.get("card_pool", item.get("reward_pool", owner_actor))
            if not isinstance(reward_pool, str):
                reward_pool = owner_actor
            cards[card_id] = CardInfo(
                id=card_id,
                owner_actor=owner_actor,
                reward_pool=reward_pool,
                rarity=item.get("rarity", "") if isinstance(item.get("rarity", ""), str) else "",
                has_upgrade=isinstance(item.get("upgrade"), dict) and bool(item.get("upgrade")),
                source=path.relative_to(ROOT).as_posix(),
            )
    return cards


def load_archetypes() -> list[dict[str, Any]]:
    path = DATA_DIR / "archetypes" / "playable_archetypes.json"
    data = load_json(path)
    return data if isinstance(data, list) else []


def ratio(upgraded: int, total: int) -> str:
    if total <= 0:
        return "0/0 (n/a)"
    return f"{upgraded}/{total} ({upgraded / total * 100:.1f}%)"


def card_has_upgrade(cards: dict[str, CardInfo], card_id: str) -> bool:
    card = cards.get(card_id)
    return card is not None and card.has_upgrade


def reward_pool_cards(cards: dict[str, CardInfo], pools: list[str]) -> list[CardInfo]:
    return [
        card for card in cards.values()
        if card.reward_pool in pools and card.rarity not in {"starter", "status", "curse"}
    ]


def main() -> int:
    cards = load_cards()
    archetypes = load_archetypes()

    print("Card upgrade coverage")
    print("=====================")
    print(f"All cards: {ratio(sum(1 for card in cards.values() if card.has_upgrade), len(cards))}")
    print()

    by_source: dict[str, list[CardInfo]] = {}
    for card in cards.values():
        by_source.setdefault(card.source, []).append(card)

    print("By card file:")
    for source, source_cards in sorted(by_source.items()):
        upgraded = sum(1 for card in source_cards if card.has_upgrade)
        print(f" - {source}: {ratio(upgraded, len(source_cards))}")
    print()

    print("By archetype:")
    for archetype in archetypes:
        archetype_id = archetype.get("id", "<unknown>")
        available = bool(archetype.get("is_available", False))
        starting_deck = [card_id for card_id in archetype.get("starting_deck", []) if isinstance(card_id, str)]
        starting_upgrades = sum(1 for card_id in starting_deck if card_has_upgrade(cards, card_id))
        pools = archetype.get("reward_card_pools")
        if not isinstance(pools, list) or not all(isinstance(item, str) for item in pools):
            pools = archetype.get("actors", [])
        pools = [item for item in pools if isinstance(item, str)]
        reward_cards = reward_pool_cards(cards, pools)
        reward_upgrades = sum(1 for card in reward_cards if card.has_upgrade)
        status = "available" if available else "locked"
        print(
            f" - {archetype_id} [{status}]: "
            f"starting deck {ratio(starting_upgrades, len(starting_deck))}; "
            f"reward pool {ratio(reward_upgrades, len(reward_cards))}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
