#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def load(path): return json.loads((ROOT/path).read_text(encoding='utf-8'))
def main():
    events=load('data/events/run_events.json'); by={e['id']:e for e in events}
    chains={
      'mapmaker':['chain_mapmaker_first_mark','chain_mapmaker_crossroads','chain_mapmaker_vault'],
      'traveler':['chain_traveler_wound','chain_traveler_return','chain_traveler_last_camp'],
      'bell':['chain_bell_buried','chain_bell_tower','chain_bell_last_chime'],
    }
    assert len(events)==97, f'expected 97 events, got {len(events)}'
    for name, ids in chains.items():
        assert all(i in by for i in ids), name
        assert not by[ids[0]].get('requirements',{}).get('has_flag')
        assert by[ids[1]].get('requirements',{}).get('has_flag')
        assert by[ids[2]].get('requirements',{}).get('has_flag')
        for eid in ids:
            effects=[fx for c in by[eid]['choices'] for fx in c.get('effects',[])]
            assert any(fx.get('type')=='set_flag' for fx in effects), f'{eid} does not persist progress'
    source=(ROOT/'src/save/RunStateSerializer.cpp').read_text(encoding='utf-8')
    assert '{"version", 4}' in source
    assert '"event_flags"' in source
    selector=(ROOT/'src/events/RunEventSelector.cpp').read_text(encoding='utf-8')
    assert 'evaluateRunEventChoiceRequirements(event->requirements, run)' in selector
    flow=(ROOT/'src/flow/GameFlowController.cpp').read_text(encoding='utf-8')
    assert 'chooseAvailableRunEvent' in flow
    parser=(ROOT/'src/data/parsers/RunEventDefinitionParser.cpp').read_text(encoding='utf-8')
    for token in ('set_flag','clear_flag','has_flag','missing_flag'):
        assert token in parser
    for lang in ('ru','en'):
        run=load(f'data/localization/{lang}/run.json')
        req=load(f'data/localization/{lang}/event_requirements.json')
        for ids in chains.values():
            for eid in ids:
                assert run.get(f'event.{eid}.title')
                assert run.get(f'event.{eid}.description')
        assert req.get('event.choice.unavailable.required_event_flag')
    print('Event chain contract passed: 3 chains, 9 events, persistent save flags')
    return 0
if __name__=='__main__': raise SystemExit(main())
