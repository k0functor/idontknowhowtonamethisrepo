from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
quality_h = (ROOT / "src/rewards/CardRewardQuality.hpp").read_text(encoding="utf-8")
quality_cpp = (ROOT / "src/rewards/CardRewardQuality.cpp").read_text(encoding="utf-8")
reward = (ROOT / "src/rewards/RewardGenerator.cpp").read_text(encoding="utf-8")
shop = (ROOT / "src/shop/ShopGenerator.cpp").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

for token in ("chooseOffers", "relevanceScore", "themeMask"):
    assert token in quality_h
for token in ("exactCopies", "candidateUsesDrone", "std::popcount", "score -= overlap * 5"):
    assert token in quality_cpp
assert "CardRewardQuality::chooseOffers" in reward
assert shop.count("CardRewardQuality::chooseOffers") >= 2
assert "src/rewards/CardRewardQuality.cpp" in cmake
print("card offer quality contract passed")
