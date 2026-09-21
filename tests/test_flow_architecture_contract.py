from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

flow_header = (ROOT / "src/flow/GameFlowController.hpp").read_text(encoding="utf-8")
flow_source = (ROOT / "src/flow/GameFlowController.cpp").read_text(encoding="utf-8")
progress_header = (ROOT / "src/flow/ProfileProgressionService.hpp").read_text(encoding="utf-8")
progress_source = (ROOT / "src/flow/ProfileProgressionService.cpp").read_text(encoding="utf-8")
map_header = (ROOT / "src/scenes/RunMapScene.hpp").read_text(encoding="utf-8")
map_source = (ROOT / "src/scenes/RunMapScene.cpp").read_text(encoding="utf-8")
layout_header = (ROOT / "src/scenes/RunMapLayout.hpp").read_text(encoding="utf-8")
layout_source = (ROOT / "src/scenes/RunMapLayout.cpp").read_text(encoding="utf-8")

assert "ProfileProgressionService profileProgression_;" in flow_header
assert "profileProgression_(content_, localization_, profileManager_)" in flow_source
assert "class ProfileProgressionService" in progress_header
assert "ProfileProgressionService::grantFloorCompletionUnlocks" in progress_source

for old_method in (
    "GameFlowController::unlockProfileContentFromRunState",
    "GameFlowController::unlockProfileContentFromRewardSelection",
    "GameFlowController::unlockProfileContentFromShopPurchase",
    "GameFlowController::unlockProfileContentFromEventOutcome",
    "GameFlowController::completeEligibleChallenges",
    "GameFlowController::completeEligibleAchievements",
):
    assert old_method not in flow_source, f"profile responsibility leaked back into GameFlowController: {old_method}"

assert len(flow_source.splitlines()) <= 3100, "GameFlowController grew past the post-extraction budget"

assert '#include "RunMapLayout.hpp"' in map_source
assert "class RunMapLayout" in layout_header
assert "RunMapLayout::nodeScreenPosition" in layout_source
assert "RunMapLayout::overlayBounds" in layout_source

for old_method in (
    "RunMapScene::nodeScreenPosition",
    "RunMapScene::mapViewportBounds",
    "RunMapScene::mapScale",
    "RunMapScene::mapContentWidth",
    "RunMapScene::nodeBounds",
    "RunMapScene::restModalBounds",
    "RunMapScene::overlayBounds",
    "RunMapScene::upgradePreviewModalBounds",
):
    assert old_method not in map_source, f"layout responsibility leaked back into RunMapScene: {old_method}"

for old_declaration in (
    "Vector2 nodeScreenPosition",
    "Rectangle mapViewportBounds",
    "Rectangle restModalBounds",
    "Rectangle overlayBounds",
):
    assert old_declaration not in map_header, f"stale layout declaration remains: {old_declaration}"

assert len(map_source.splitlines()) <= 2350, "RunMapScene grew past the post-extraction budget"

print("Flow architecture contract passed")
