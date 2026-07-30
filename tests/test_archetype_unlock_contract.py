#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    profile_manager = read("src/profile/ProfileManager.cpp")
    profile_save = read("src/profile/ProfileSaveSystem.cpp")
    game_flow = read("src/flow/GameFlowController.cpp")

    starter_block = profile_manager.split("starterArchetypeIds{", 1)[1].split("};", 1)[0]
    assert '"rusted_knight"' in starter_block
    assert '"herbalist"' in starter_block
    assert '"sadist_masochist"' not in starter_block, (
        "Sadist and Masochist must not be a starter archetype"
    )

    assert 'run.currentFloorId == "floor3"' in game_flow
    floor_three_block = game_flow.split('run.currentFloorId == "floor3"', 1)[1].split("}", 1)[0]
    assert 'unlockArchetypeAndToast("sadist_masochist")' in floor_three_block

    achievements = json.loads(read("data/achievements/achievements.json"))
    achievement = next(item for item in achievements if item["id"] == "ashen_conservatory_survivor")
    assert "sadist_masochist" in achievement["rewards"]["unlock_archetypes"]
    assert "furnace_heart" in achievement["rewards"]["unlock_relics"]

    for locale in ("ru", "en"):
        archetypes = json.loads(read(f"data/localization/{locale}/archetypes.json"))
        core = json.loads(read(f"data/localization/{locale}/core.json"))
        assert archetypes["profile_hub.unlock_hint.sadist_masochist"]
        assert core["achievement.reward.furnace_heart_and_sadist_masochist"]

    assert "profileSaveVersion = 8" in profile_save
    assert "version < 8" in profile_save
    assert 'entry.contentId == "sadist_masochist"' in profile_save
    assert "clearedThirdFloor" in profile_save

    print("Archetype unlock contract passed")


if __name__ == "__main__":
    main()
