from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

combat_header = (ROOT / "src/scenes/CombatScene.hpp").read_text(encoding="utf-8")
combat_sources = {
    name: (ROOT / f"src/scenes/{name}").read_text(encoding="utf-8")
    for name in (
        "CombatScene.cpp",
        "CombatScenePiles.cpp",
        "CombatSceneRewards.cpp",
        "CombatSceneInspectConsumables.cpp",
    )
}
combat_layout_header = (ROOT / "src/scenes/CombatSceneLayout.hpp").read_text(encoding="utf-8")
combat_layout_source = (ROOT / "src/scenes/CombatSceneLayout.cpp").read_text(encoding="utf-8")

shop_header = (ROOT / "src/scenes/ShopScene.hpp").read_text(encoding="utf-8")
shop_source = (ROOT / "src/scenes/ShopScene.cpp").read_text(encoding="utf-8")
shop_layout_header = (ROOT / "src/scenes/ShopLayout.hpp").read_text(encoding="utf-8")
shop_layout_source = (ROOT / "src/scenes/ShopLayout.cpp").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

assert "class CombatSceneLayout" in combat_layout_header
assert "CombatSceneLayout::pileOverlayBounds" in combat_layout_source
assert "CombatSceneLayout::rewardCardChoiceOptionBounds" in combat_layout_source
assert "CombatSceneLayout::combatItemInspectModalBounds" in combat_layout_source

for source in combat_sources.values():
    assert '#include "CombatSceneLayout.hpp"' in source

for old_method in (
    "CombatScene::playedCardCenterPosition",
    "CombatScene::drawPileButtonBounds",
    "CombatScene::pileOverlayBounds",
    "CombatScene::combatItemInspectModalBounds",
    "CombatScene::consumableConfirmationBounds",
    "CombatScene::rewardModalBounds",
    "CombatScene::rewardCardChoiceOptionBounds",
):
    assert all(old_method not in source for source in combat_sources.values()), (
        f"combat layout responsibility leaked back into a scene: {old_method}"
    )

for old_declaration in (
    "Vector2 playedCardCenterPosition",
    "Rectangle drawPileButtonBounds",
    "Rectangle pileOverlayBounds",
    "Rectangle combatItemInspectModalBounds",
    "Rectangle rewardModalBounds",
):
    assert old_declaration not in combat_header, f"stale combat layout declaration remains: {old_declaration}"

combat_scene_lines = sum(len(source.splitlines()) for source in combat_sources.values())
assert combat_scene_lines <= 4150, "combat scene implementation grew past the post-layout budget"

assert "class ShopLayout" in shop_layout_header
assert "ShopLayout::offerBounds" in shop_layout_source
assert "ShopLayout::removeCardBounds" in shop_layout_source
assert '#include "ShopLayout.hpp"' in shop_source

for old_method in (
    "ShopScene::panelBounds",
    "ShopScene::offerBounds",
    "ShopScene::cardOfferColumnCount",
    "ShopScene::removeModeBounds",
    "ShopScene::purchaseConfirmationBounds",
    "ShopScene::relicOwnerModalBounds",
    "ShopScene::visibleRemoveCardCount",
):
    assert old_method not in shop_source, f"shop layout responsibility leaked back into ShopScene: {old_method}"

for old_declaration in (
    "Rectangle panelBounds",
    "Rectangle offerBounds",
    "Rectangle removeModeBounds",
    "Rectangle purchaseConfirmationBounds",
    "std::size_t visibleRemoveCardCount",
):
    assert old_declaration not in shop_header, f"stale shop layout declaration remains: {old_declaration}"

assert len(shop_source.splitlines()) <= 1050, "ShopScene grew past the post-layout budget"

for path in (
    "src/scenes/CombatSceneLayout.hpp",
    "src/scenes/CombatSceneLayout.cpp",
    "src/scenes/ShopLayout.hpp",
    "src/scenes/ShopLayout.cpp",
):
    assert path in cmake, f"layout source is missing from CMake: {path}"

print("Scene layout architecture contract passed")
