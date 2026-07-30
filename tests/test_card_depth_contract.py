#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    'herbalist_autopsy', 'herbalist_surgical_flurry',
    'rusted_rallying_plate', 'rusted_sweeping_arc',
    'merchant_black_market_map', 'merchant_debt_collector',
    'monk_flame_wheel', 'monk_ash_mirror', 'monk_smoke_afterimage',
    'replicant_precision_ping', 'replicant_hard_reset',
    'lost_psychopath_blind_spot', 'lost_psychopath_storm_of_hands',
    'sadist_symphony_of_knives', 'masochist_blood_smile',
}

cards = {}
for path in (ROOT / 'data/cards').glob('*.json'):
    payload = json.loads(path.read_text(encoding='utf-8'))
    entries = payload if isinstance(payload, list) else payload.get('cards', [])
    cards.update({entry['id']: entry for entry in entries})

missing = EXPECTED - cards.keys()
assert not missing, f'missing redesigned cards: {sorted(missing)}'

scaled = set()
recovering = set()
for card_id in EXPECTED:
    card = cards[card_id]
    for version in (card.get('effects', []), card.get('upgrade', {}).get('effects', [])):
        for effect in version:
            if isinstance(effect.get('scaling'), dict):
                scaled.add(card_id)
            if effect.get('type') == 'recover_cards':
                recovering.add(card_id)

assert len(scaled) >= 13, f'expected broad conditional scaling, found {len(scaled)} cards'
assert recovering == {'merchant_black_market_map', 'masochist_blood_smile'}, recovering

for card_id in scaled:
    for effect in cards[card_id]['effects']:
        scaling = effect.get('scaling')
        if not isinstance(scaling, dict):
            continue
        assert any(key in scaling for key in (
            'bonus_if_status_present', 'bonus_per_status_stack',
            'bonus_per_card_in_hand', 'bonus_per_card_in_discard'
        )), f'{card_id}: scaling has no source'
        maximum = scaling.get('maximum_bonus', -1)
        assert isinstance(maximum, int) and maximum >= 0, f'{card_id}: scaling must be capped'

source = (ROOT / 'src/combat/EffectScaling.cpp').read_text(encoding='utf-8')
assert 'bonusPerCardInHand' in source and 'bonusPerCardInDiscard' in source
assert 'bonusPerStatusStack' in source and 'bonusIfStatusPresent' in source
assert 'maximumBonus' in source

effect_system = (ROOT / 'src/combat/EffectSystem.cpp').read_text(encoding='utf-8')
assert 'recoverCardsFromDiscard' in effect_system
assert 'state.hand.full()' in effect_system

print(f'Card depth contract passed: {len(scaled)} scaled cards, {len(recovering)} recovery cards')
