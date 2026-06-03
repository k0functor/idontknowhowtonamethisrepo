#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from validate_project import ProjectValidator  # noqa: E402


class CoreReadinessValidator:
    def __init__(self) -> None:
        self.errors: list[str] = []
        self.lines: list[str] = []
        self.project = ProjectValidator()

    def error(self, owner: str, message: str) -> None:
        self.errors.append(f"{owner}: {message}")

    def load_content(self) -> bool:
        self.project.validate_json_parse_smoke()
        self.project.load_indexed_content()
        if self.project.errors:
            self.errors.extend(self.project.errors)
            return False
        return True

    def card_pool_for(self, card: dict[str, Any]) -> str | None:
        value = card.get("card_pool") or card.get("reward_pool") or card.get("owner_actor")
        return value if isinstance(value, str) else None

    def card_can_appear_as_reward(self, card: dict[str, Any]) -> bool:
        return self.project.card_can_appear_as_reward(card)

    def has_upgrade(self, card: dict[str, Any]) -> bool:
        upgrade = card.get("upgrade")
        return isinstance(upgrade, dict) and bool(upgrade)

    def validate_available_archetypes(self) -> None:
        for archetype_id, archetype in sorted(self.project.content.archetypes.items.items()):
            if archetype.get("is_available") is not True:
                continue

            owner = self.project.content.archetypes.paths[archetype_id]
            actors = archetype.get("actors", [])
            starting_deck = archetype.get("starting_deck", [])
            reward_pools = archetype.get("reward_card_pools", [])
            starting_relics = archetype.get("starting_relics", [])
            starting_consumables = archetype.get("starting_consumables", [])

            if not actors:
                self.error(owner, "available archetype has no actors")
            if not starting_deck:
                self.error(owner, "available archetype has empty starting deck")
            if not reward_pools:
                self.error(owner, "available archetype has empty reward_card_pools")

            reward_cards = [
                card
                for card in self.project.content.cards.items.values()
                if self.card_pool_for(card) in reward_pools and self.card_can_appear_as_reward(card)
            ]
            if not reward_cards:
                self.error(owner, "available archetype has no reward cards")

            cards_to_upgrade = [
                self.project.content.cards.items[card_id]
                for card_id in dict.fromkeys(starting_deck)
                if card_id in self.project.content.cards.items
            ] + reward_cards
            missing_upgrade_ids = sorted(
                {
                    card.get("id", "<missing id>")
                    for card in cards_to_upgrade
                    if card.get("type") not in {"status", "curse"}
                    and not self.has_upgrade(card)
                }
            )
            if missing_upgrade_ids:
                self.error(owner, "non-status cards without upgrade: " + ", ".join(missing_upgrade_ids))

            self.lines.append(
                f" - {archetype_id}: actors={len(actors)}, "
                f"starter_cards={len(starting_deck)}, reward_cards={len(reward_cards)}, "
                f"starting_relics={len(starting_relics)}, starting_consumables={len(starting_consumables)}"
            )

    def validate_sadist_masochist_contract(self) -> None:
        archetype_id = "sadist_masochist"
        archetype = self.project.content.archetypes.items.get(archetype_id)
        owner = self.project.content.archetypes.paths.get(archetype_id, "sadist_masochist")
        if archetype is None:
            self.error(owner, "archetype is missing")
            return

        expected_actors = ["sadist", "masochist"]
        if archetype.get("actors") != expected_actors:
            self.error(owner, f"actors must be exactly {expected_actors}")
        if archetype.get("reward_card_pools") != ["sadist_masochist"]:
            self.error(owner, "reward_card_pools must be exactly ['sadist_masochist']")

        for actor_id in expected_actors:
            actor = self.project.content.actors.items.get(actor_id)
            actor_owner = self.project.content.actors.paths.get(actor_id, actor_id)
            if actor is None:
                self.error(actor_owner, "actor is missing")
                continue
            if actor.get("starting_energy") != 2:
                self.error(actor_owner, "Sadist/Masochist actors must each start with 2 energy")
            if not actor.get("starting_relics"):
                self.error(actor_owner, "actor must have its own starting_relics for per-actor relic ownership")

        card_pool_cards = [
            (card_id, card)
            for card_id, card in sorted(self.project.content.cards.items.items())
            if card.get("card_pool") == "sadist_masochist"
        ]
        if not card_pool_cards:
            self.error(owner, "shared card_pool 'sadist_masochist' has no cards")

        for card_id in archetype.get("starting_deck", []):
            card = self.project.content.cards.items.get(card_id)
            card_owner = self.project.content.cards.paths.get(card_id, card_id)
            if card is None:
                continue
            if card.get("card_pool") != "sadist_masochist":
                self.error(card_owner, "starting deck card must belong to shared card_pool 'sadist_masochist'")

        for card_id, card in card_pool_cards:
            card_owner = self.project.content.cards.paths.get(card_id, card_id)
            if "owner_actor" in card:
                self.error(card_owner, "Sadist/Masochist cards must be shared and must not define owner_actor")
            if "energy_pool" in card:
                self.error(card_owner, "Sadist/Masochist cards must be shared and must not define energy_pool")
            if card.get("reward_pool") not in (None, "sadist_masochist"):
                self.error(card_owner, "Sadist/Masochist cards must not use a different reward_pool")
            if card.get("type") not in {"status", "curse"} and not self.has_upgrade(card):
                self.error(card_owner, "shared Sadist/Masochist card is missing upgrade")

        for status_id in ("sadist_pleasure", "masochist_pain"):
            status = self.project.content.statuses.items.get(status_id)
            status_owner = self.project.content.statuses.paths.get(status_id, status_id)
            if status is None:
                self.error(status_owner, "status is missing")
                continue
            if status.get("duration_rule") != "decrease_end_of_owner_turn":
                self.error(status_owner, "temporary Sadist/Masochist status must expire at end of owner turn")

    def validate_no_flat_attack_damage_relics(self) -> None:
        for relic_id, relic in sorted(self.project.content.relics.items.items()):
            owner = self.project.content.relics.paths[relic_id]
            for index, modifier in enumerate(relic.get("modifiers", [])):
                if modifier.get("type") == "outgoing_damage_add":
                    self.error(
                        f"{owner}.modifiers[{index}]",
                        "flat +damage relics should be implemented as combat_started strength instead",
                    )

    def run(self) -> int:
        if self.load_content():
            self.validate_available_archetypes()
            self.validate_sadist_masochist_contract()
            self.validate_no_flat_attack_damage_relics()

        if self.errors:
            print("Core readiness validation failed:")
            for error in self.errors:
                print(f" - {error}")
            return 1

        print("Core readiness OK:")
        for line in self.lines:
            print(line)
        print(" - sadist_masochist: shared cards, fixed Sadist -> Masochist subturns, 2+2 energy contract, temporary pain/pleasure statuses OK")
        print(" - relics: no flat outgoing_damage_add relic modifiers")
        return 0


def main() -> int:
    return CoreReadinessValidator().run()


if __name__ == "__main__":
    sys.exit(main())
