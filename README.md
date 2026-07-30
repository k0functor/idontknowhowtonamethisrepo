# I Don't Know How To Name This Game

Data-driven card roguelike prototype written in C++ with raylib and JSON content files.

The project is intentionally split into engine/gameplay code under `src/` and editable content under `data/`. Runtime text is localized through `data/localization/<locale>/*.json`; do not add old root-level files like `data/localization/ru.json` or `data/localization/en.json`.

## Requirements

- C++20 compiler, tested with GCC/MSYS2 on Windows.
- CMake 3.28 or newer.
- Python 3 for validation tools.
- raylib is fetched by CMake when configuring the project. FetchContent pins raylib to an exact commit and verifies the nlohmann/json archive with SHA-256.

## Configure and build

```bash
cmake --preset gcc-debug
cmake --build --preset gcc-debug
```

The debug executable is produced under `build/gcc-debug/bin/`.

Dependencies fetched by CMake are shared through `.cache/fetchcontent/`. After one successful online configure, the same cache can be reused for Debug and Release builds. For a deliberately offline configure, pre-populate that directory first and add `-DFETCHCONTENT_FULLY_DISCONNECTED=ON`; CMake then refuses to contact the network instead of quietly improvising a new dependency version.

The gameplay rules are compiled once into the shared `game_core` library and linked by both the application and the C++ tests. The dependency-free preset keeps gameplay iteration fast:

```bash
cmake --preset core-debug
cmake --build --preset core-debug
ctest --preset core-debug
```

JSON persistence is isolated in `game_persistence`. Its real serializer and filesystem recovery tests can run without building raylib:

```bash
cmake --preset persistence-debug
cmake --build --preset persistence-debug
ctest --preset persistence-debug
```

## Build a clean Windows release

Run the packaging script from PowerShell inside the same MSYS2 UCRT64 environment used for the build:

```powershell
pwsh -File tools/package_windows_release.ps1
```

The script forces static GNU runtime linkage, disables debug tools in the packaged config, rejects non-system DLL dependencies, and validates both the staging directory and final ZIP. Local files under `saves/`, logs, symbols, source files, and developer settings are never distributable runtime content. The game creates `saves/settings.json` on first launch from the safe defaults in `config/app.json`.

A package can also be checked independently:

```bash
python tools/validate_release_package.py dist/idontknowhowtonamethisrepo-windows-x64.zip
```

## Run dependency-free gameplay tests

The core and integration gameplay tests do not fetch raylib or JSON dependencies. They cover individual combat rules plus complete multi-enemy turns, relic/status interaction, boss phases, summons, arena effects, and persistent room-state contracts:

```bash
cmake -S . -B build/core-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGAME_BUILD_APP=OFF -DGAME_BUILD_PERSISTENCE=OFF -DBUILD_TESTING=ON
cmake --build build/core-tests
ctest --test-dir build/core-tests --output-on-failure
```

## Run validators

Run these before making a patch or adding content:

```bash
python tools/validate_project.py
python tools/validate_localization.py
python tools/validate_cmake_sources.py
python tools/validate_floor_definitions.py
python tools/validate_floor_vertical_slices.py
python tools/validate_run_save_files.py
python tests/test_enemy_ai_contract.py
python tests/test_active_item_contract.py
python tests/test_reroll_die_contract.py
python tests/test_active_item_acquisition_contract.py
python tests/test_active_item_expansion_contract.py
python tests/test_stress_band_contract.py
python tests/test_stress_economy_contract.py
python tests/test_stress_breakdown_contract.py
python tests/test_stress_content_contract.py
python tests/test_boss_phase_contract.py
python tests/test_status_driven_relics_contract.py
python tests/test_card_lifecycle_contract.py
python tests/test_data_driven_status_contract.py
python tests/test_run_save_room_integration.py
```

The broad content validator checks JSON parsing, cross-references, cards, relics, consumables, enemies, encounters, events, floors, and deprecated data layout files.

## Content layout

- `data/actors/` - combat actors used by playable archetypes.
- `data/archetypes/` - playable archetype definitions and starting decks.
- `data/cards/` - real card definitions. Starter cards live in their archetype card files and are referenced from archetype starting decks; do not recreate `starter_cards.json`.
- `data/enemies/` and `data/encounters/` - enemy definitions and encounter pools.
  Encounter entries use an `enemies` array with one to three ids. Repeated ids are allowed for swarm encounters.
  Enemy actions use weighted AI metadata: `weight`, `cooldown`, `max_consecutive_uses`, and an optional `conditions` object. Conditions can inspect the turn, HP thresholds, living enemy count, wounded allies, and statuses on the acting enemy or players.
  Boss enemies define ordered `phases` with descending HP thresholds, phase-specific action pools, entry effects, optional once-per-player-turn arena effects, and optional summons. Phase progression never reverses after healing, and the living enemy cap remains three.
- `data/events/` - map event definitions.
- `data/localization/ru/` and `data/localization/en/` - split localization bundles.
- `data/relics/`, `data/consumables/`, `data/statuses/`, `data/drones/` - other gameplay content.

### Data-driven statuses

Status definitions can declare numeric card modifiers, mutually exclusive groups, and end-of-turn triggers directly in `data/statuses/`. A modifier selects the affected effect type (`damage` or `block`), whether the status belongs to the source or target, its operation (`add_per_stack`, `add_fixed`, `multiply_per_stack`, or `multiply_fixed`), priority, and localized breakdown text. `exclusive_group` replaces hard-coded stance ids. End-turn `triggers` can calculate damage, healing, or block from a flat value and current stacks, then remove a configured number of stacks.

Strength, Dexterity, Ferocity, Weak, Vulnerable, the three Monk stances, Pleasure, Pain, Poison, and Burn all use this definition format. The only remaining built-in numeric modifier is the Lost Psychopath stress band, because stress is an archetype resource rather than a status.
- `data/active_items/` - charged active-item definitions. A run has one slot; normal, elite, and boss rooms grant 1, 2, and 3 charge respectively. Definitions declare `max_charge`, `charge_cost`, allowed `use_contexts`, and effects. Scene-owned effects such as `reroll_offers` are applied to persistent pending reward/shop state so saving and loading cannot restore the old offers.
- `data/run/` - floors, acts, map generation and difficulties.

## Continuous integration

`.github/workflows/ci.yml` runs dependency-free gameplay tests and validators, real JSON save round-trip/recovery tests, complete Debug/Release Linux builds with a headless startup check, and a statically linked Windows portable package validated by `tools/validate_release_package.py`.

## Patch discipline

Prefer small, reviewable patches. Add or update validators when changing data formats, because relying on humans to remember invisible contracts is how projects turn into folklore.

## Active-item debug commands

Use the F1 debug panel to equip and charge the foundation item while its normal acquisition sources are still being built:

```text
give active field_kit 3
give active reroll_die 4
give active hourglass 5
give active alchemist_flask 3
give active compass 4
give active mirror 5
give active grounding_chime 4
charge active 5
```

Press Space in an allowed room context to use the equipped item. The Reroll Die changes unclaimed reward offers, remaining normal-shop stock, or a chest relic. It cannot be used after part of a reward has already been claimed, and it never rerolls earned gold or card-removal service. The Hourglass arms a one-use skip for the next enemy action phase while still allowing end-of-turn statuses to tick. The Alchemist Flask creates a random consumable when inventory space exists. The Compass rerolls only currently available ordinary map nodes and preserves elite, rest, boss, locked, completed, and current nodes. The Mirror copies the explicitly selected reward or shop card directly into the run deck without claiming or purchasing the original offer.


## Active-item acquisition

Active items can appear as elite or boss rewards, replace a chest relic, or occupy a dedicated normal-shop offer. Selecting one opens a comparison panel for the equipped and offered items. Keeping the old item leaves the offer untouched; equipping the new item destroys the old one and starts the replacement at zero charge. Shop replacement prices come from the active-item definition, while reward and shop eligibility are controlled by `reward_eligible` and `shop_eligible`.

## Lost Psychopath stress bands

The Lost Psychopath uses five 40-point stress bands:

- `0-39` Calm: no attack bonus.
- `40-79` Tense: `+1` attack damage.
- `80-119` Pressured: `+2` attack damage. Crossing 100 stress still performs the resolve check.
- `120-159` Panicked: `+3` attack damage. Stress Breakdown triggers one random severity-1 penalty after the hand is drawn.
- `160-199` Breaking: `+4` attack damage and `+1` energy at the start of turn. Stress Breakdown triggers one random severity-2 penalty; an unresolved actor without either trait still discards one card, while Stress Resolve prevents the penalty.
- `200` Collapsed: the actor dies from stress.

The combat panel shows the current band, a segmented stress bar, the attack bonus, the next threshold, and any start-of-turn risk.

## Stress economy

Enemy attacks generate stress only from damage that reaches player HP: one stress per three HP damage, rounded up, with a maximum of 12 stress from one hit. Fully blocked hits generate no stress. Selected enemy actions can also apply direct party stress, so controller and support enemies can pressure the resource without relying only on raw damage.

Lost Psychopath cards now include explicit stress generators, recovery cards, and four fixed-cost conversions: stress into damage, block, energy, or card draw. A conversion card is unplayable unless its actor can pay the complete stress cost, including repeated effects. The normal rest action heals only; **Calm down** is the separate option that removes 60 stress without healing.

## Stress breakdowns

At Panicked or Breaking stress, an actor with the Stress Breakdown trait receives one random start-of-turn penalty. Possible outcomes are random discard, energy loss, temporary status cards in hand, increased card costs for the turn, or an uncontrolled attack card played against a random living enemy. Breaking uses severity 2, increasing the size of discard, energy, status-card, and cost penalties. Stress Resolve suppresses these random breakdown outcomes.

## Stress-reactive content

Stress breakdowns now emit a dedicated `stress_breakdown_triggered` game event with the breakdown type and severity. Relic triggers can filter that event through `breakdown_type` and `min_breakdown_severity`, allowing content to respond to discard, energy loss, status-card, cost-spike, or frenzy breakdowns without hard-coded relic ids.

Four relics use these filters: Grounding Bead softens the first breakdown of a combat, Cracked Metronome compensates energy crashes, Ash Filter turns status-card breakdowns into recovery, and Red Thread Spool rewards severe frenzy. The Grounding Chime active item removes stress and prevents the next breakdown for the most stressed living actor.

Enemy actions may use `prime_stress_breakdown` to determine the next breakdown type before adding stress. Event choices can require `min_stress`, `max_stress`, `has_trait`, or `missing_trait`; three new stress events use those requirements to offer different outcomes according to the run's current mental state.

## Boss phases and arena rules

Every boss has three ordered phases. The first phase activates at 100% HP; later phases activate when the boss crosses their configured thresholds. A transition immediately clears the boss action history and cooldowns, applies phase-entry effects, refreshes its intent, and may summon non-boss enemies up to the configured living-enemy cap. Summons inherit the run enemy-HP multiplier.

Each phase owns an `action_ids` pool, so conditional enemy AI selects only actions valid for the current phase. Optional `player_turn_effects` act as arena rules and trigger at most once at the start of each player turn. Healing never returns a boss to an earlier phase. Combat logs and the enemy panel expose the active localized phase name, summons, and arena-rule activations.


## Balance simulation and local telemetry

Run deterministic static simulations for starter decks and encounter pressure:

```powershell
cmake --build build/core-tests --target simulate_balance
```

The command writes `reports/balance_simulation.json` and
`reports/balance_simulation.csv`. The model is intentionally approximate: it
compares starter-deck output with weighted enemy action pressure and is meant to
identify outliers before manual playtesting, not replace playtests.

Completed runs are appended locally to
`saves/telemetry/run_history.jsonl`. No network requests are made. Summarize the
file with:

```powershell
cmake --build build/core-tests --target report_run_telemetry
```

## Balance targets

`data/balance/progression_targets.json` defines the expected growth of player offense and defense between floors and the target health-loss bands for ordinary, elite, and boss encounters. `tools/simulate_balance.py` uses these projections instead of comparing late-floor enemies directly with an untouched starter deck. The report marks encounter outliers and records their weights and layer windows so tuning can change frequency before changing enemy identity.

Hard difficulty now applies both configured multipliers in live combat: enemy maximum health and enemy outgoing damage.

### Conditional card scaling

Card effects may define a capped `scaling` object. The bonus can depend on a source or target status, the number of cards in hand, or the number of cards in discard. `recover_cards` moves cards from the top of the discard pile into the hand and respects the ten-card hand limit.
