#!/usr/bin/env python3
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(path: str):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


def main() -> int:
    relics = load("data/relics/test_relics.json")
    by_id = {relic["id"]: relic for relic in relics}
    events = Counter(
        trigger.get("event")
        for relic in relics
        for trigger in relic.get("triggers", [])
    )

    assert events["combat_started"] <= 12, events
    assert events["card_played"] >= 12, events

    old_bell = by_id["old_bell"]["triggers"][0]
    assert old_bell.get("card_type") == "skill"
    assert old_bell.get("previous_card_type") == "attack"

    black_candle = by_id["black_candle"]["triggers"][0]
    assert black_candle.get("card_number_this_turn") == 3
    assert {effect["type"] for effect in black_candle["effects"]} == {"gain_energy", "draw_cards"}

    crown = by_id["crown_of_nails"]["triggers"][0]
    halo = by_id["clockwork_halo"]["triggers"][0]
    assert crown.get("card_number_this_turn") == 4
    assert halo.get("card_number_this_turn") == 5

    duelist = by_id["duelist_coin"]["triggers"]
    pairs = {(t.get("previous_card_type"), t.get("card_type")) for t in duelist}
    assert pairs == {("attack", "skill"), ("skill", "attack")}

    drone = by_id["drone_caliper"]
    assert drone.get("mechanic_id") == "replicant_drones"
    assert drone["triggers"][0].get("min_drones") == 3

    bead = by_id["threefold_bead"]
    assert bead.get("mechanic_id") == "monk_stances"
    stance_filters = {t.get("owner_status") for t in bead["triggers"]}
    assert stance_filters == {"stance_flame", "stance_ash", "stance_smoke"}

    for relic_id in (
        "old_bell", "splintered_banner", "first_wick", "black_candle",
        "brass_locket", "polished_guard", "duelist_coin", "drone_caliper",
        "threefold_bead", "ember_compass", "crown_of_nails", "glass_heart",
        "serpent_standard", "clockwork_halo",
    ):
        assert all(t.get("event") != "combat_started" for t in by_id[relic_id].get("triggers", [])), relic_id

    for lang in ("ru", "en"):
        text = load(f"data/localization/{lang}/relics.json")
        for relic_id in ("old_bell", "duelist_coin", "drone_caliper", "threefold_bead", "clockwork_halo"):
            assert text.get(f"relic.{relic_id}.description")

    print(
        "Build-around relic contract passed: "
        f"combat_started={events['combat_started']}, card_played={events['card_played']}, relics={len(relics)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
