from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
reward = json.loads((ROOT / "data/rewards/reward_tables.json").read_text(encoding="utf-8"))
shop = json.loads((ROOT / "data/shop/shop_tables.json").read_text(encoding="utf-8"))
flow = (ROOT / "src/flow/GameFlowController.cpp").read_text(encoding="utf-8")
generator = (ROOT / "src/shop/ShopGenerator.cpp").read_text(encoding="utf-8")
view = (ROOT / "src/ui/CombatView.cpp").read_text(encoding="utf-8")
model = (ROOT / "src/ui/CardViewModel.hpp").read_text(encoding="utf-8")
builder = (ROOT / "src/ui/CardViewModelBuilder.cpp").read_text(encoding="utf-8")

assert reward["gold_growth_percent_per_floor"] > 0
for key in (
    "card_price_growth_percent_per_floor",
    "relic_price_growth_percent_per_floor",
    "consumable_price_growth_percent_per_floor",
    "card_removal_price_per_floor",
    "card_removal_price_per_use",
    "affordable_card_offers",
    "affordable_card_price_cap_percent",
):
    assert key in shop, f"missing economy tuning: {key}"
assert "tuning.floorGoldMultiplier(context.run.currentFloorIndex)" in (ROOT / "src/rewards/RewardGenerator.cpp").read_text(encoding="utf-8")
assert "ensureAffordableCardOffers(shop, run, tuning)" in generator
assert "run.stats.cardsRemoved" in generator
assert "ShopGenerator::createShop" in flow
assert "modifierPreviewLabel" not in model
assert "modifierPreviewLabel" not in builder
assert "outcomePreviewLabel" not in model, "calculated attack/block/heal values must not be duplicated in the card UI"
print("run economy and card hover cleanup contract passed")
