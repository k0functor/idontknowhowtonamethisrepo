#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def load(path: str): return json.loads((ROOT/path).read_text(encoding='utf-8'))

def main() -> int:
    relics=load('data/relics/test_relics.json')
    by_id={r['id']:r for r in relics}
    expected={
        'polished_guard','butcher_hook','venom_vial','ash_thread','iron_spur','prayer_strip',
        'cracked_prism','scar_talisman','duelist_coin','toxic_censer','drone_caliper',
        'threefold_bead','blood_receipt','smoke_clasp','ember_compass','collector_badge',
        'crown_of_nails','glass_heart','serpent_standard','clockwork_halo'
    }
    assert len(relics) == 49, f'expected 49 relics, got {len(relics)}'
    assert expected <= by_id.keys()
    statuses={s['id'] for s in load('data/statuses/combat_statuses.json')}
    status_relics=0
    for relic in relics:
        applies=[]
        for trigger in relic.get('triggers',[]):
            for effect in trigger.get('effects',[]):
                if effect.get('type')=='apply_status':
                    assert effect.get('status') in statuses, f"{relic['id']} references unknown status"
                    applies.append(effect['status'])
        status_relics += bool(applies)
    assert status_relics >= 36, f'expected at least 36 status relics, got {status_relics}'
    assert any(e.get('target')=='all_enemies' and e.get('status')=='poison' for t in by_id['serpent_standard']['triggers'] for e in t['effects'])
    assert any(e.get('status')=='vulnerable' and e.get('target')=='self' for t in by_id['glass_heart']['triggers'] for e in t['effects'])
    for lang in ('ru','en'):
        text=load(f'data/localization/{lang}/relics.json')
        for rid in expected:
            assert text.get(f'relic.{rid}.name')
            assert text.get(f'relic.{rid}.description')
    print(f'Relic expansion contract passed: {len(relics)} relics, {status_relics} status-driven')
    return 0
if __name__=='__main__': raise SystemExit(main())
