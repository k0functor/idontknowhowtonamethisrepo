#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAMEPLAY = (ROOT / "tests" / "GameplayIntegrationTests.cpp").read_text(encoding="utf-8")
PERSISTENCE = (ROOT / "tests" / "RunStatePersistenceTests.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

required_gameplay_scenarios = {
    "testStressCollapseAndPsychopathIsolation": "stress collapse and psychopath isolation",
    "testMonkStanceCycleAndReward": "monk stance cycle",
    "testDroneSlotOverflowPassiveAndConsumption": "drone slot lifecycle",
    "testSadistMasochistEngineAndSequentialTurns": "duo sequential turns",
    "testBossPhaseAndArenaScenario": "boss phase and summoned entourage cleanup",
    "testBossDefeatClearsMinions": "boss entourage cleanup",
}

for symbol, description in required_gameplay_scenarios.items():
    assert f"void {symbol}()" in GAMEPLAY, f"missing critical QA scenario: {description}"
    assert f"    {symbol}();" in GAMEPLAY, f"critical QA scenario is not executed: {description}"

assert "stress collapse of the final actor must end combat in defeat" in GAMEPLAY
assert "high stress must not grant a direct damage bonus to ordinary actors" in GAMEPLAY
assert "high stress must grant the psychopath-only damage bonus" in GAMEPLAY
assert "successive stance shifts must each trigger the monk reward" in GAMEPLAY
assert "overflow must evict the oldest drone" in GAMEPLAY
assert "a dead duo member must be skipped" in GAMEPLAY
assert "summoned boss minions must die immediately with their boss" in GAMEPLAY

for symbol, description in {
    "testPacingStateRoundTrip": "pacing save round trip",
    "testEveryPersistentRoomKindRoundTrips": "all persistent room kinds",
}.items():
    assert f"void {symbol}()" in PERSISTENCE, f"missing persistence QA scenario: {description}"
    assert symbol in PERSISTENCE.split("const std::vector<TestCase> tests", 1)[1], (
        f"persistence QA scenario is not registered: {description}"
    )

core_block = CMAKE.split("set(GAME_CORE_SOURCES", 1)[1].split(")", 1)[0]
app_block = CMAKE.split("set(GAME_APP_SOURCES", 1)[1].split(")", 1)[0]
assert "src/run/SadistMasochistRules.cpp" in core_block, (
    "Sadist/Masochist rules must live in game_core so the real mechanic is integration-testable"
)
assert "src/run/SadistMasochistRules.cpp" not in app_block, (
    "Sadist/Masochist rules must not be compiled twice by app and game_core"
)

print("Critical gameplay QA contract passed")
