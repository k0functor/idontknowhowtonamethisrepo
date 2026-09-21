#include "core/Random.hpp"
#include "data/Json.hpp"
#include "save/RunSaveSystem.hpp"
#include "save/RunStateSerializer.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("idk_game_persistence_tests_" + std::to_string(nonce));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void require(const bool condition, const std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

template <typename Function>
void requireThrowsContaining(Function&& function, const std::string_view expectedText) {
    try {
        std::forward<Function>(function)();
    } catch (const std::exception& error) {
        require(
            std::string_view(error.what()).find(expectedText) != std::string_view::npos,
            "exception did not contain the expected diagnostic"
        );
        return;
    }

    throw std::runtime_error("expected an exception");
}

RunState makeMapRun(const int gold = 137) {
    RunState run;
    run.archetypeId = PlayableArchetypeId("replicant");
    run.difficultyId = DifficultyId("normal");
    run.archetypeMechanicId = "drone_bay";
    run.challengeId = "";
    run.seed = 424242u;
    run.randomState = Random(run.seed).state();
    run.gold = gold;
    run.act = 2;
    run.currentFloorId = "floor2";
    run.currentFloorIndex = 2;
    run.nextFloorId = "floor3";
    run.enemyHpMultiplier = 1.15f;
    run.enemyDamageMultiplier = 1.10f;
    run.goldRewardMultiplier = 0.90f;
    run.deckCardIds = {
        CardId("replicant_strike"),
        CardId("replicant_guard"),
        CardId("replicant_overclock")
    };
    run.upgradedDeckIndices = {2, 0, 2, -1, 99};
    run.activeItem.itemId = "reroll_die";
    run.activeItem.charge = 2;
    run.consumableIds = {"repair_kit", "focus_tonic"};
    run.maxConsumables = 3;
    run.actorDefinitionIds = {"replicant_body"};
    run.rewardCardPoolIds = {"replicant_body", "replicant_body", ""};

    RunActorState actor;
    actor.definitionId = "replicant_body";
    actor.currentHp = 54;
    actor.maxHp = 71;
    actor.stress = 43;
    actor.maxStress = 200;
    actor.resolveCheckTriggered = true;
    actor.traitIds = {"synthetic", "synthetic", ""};
    actor.relicIds = {"rusted_capacitor", "rusted_capacitor"};
    run.actorStates.push_back(std::move(actor));
    run.relicIds = {"obsolete_legacy_copy"};

    RunMapNode node;
    node.id = 7;
    node.type = RunMapNodeType::Combat;
    node.state = RunMapNodeState::Completed;
    node.position = Vec2{320.f, 180.f};
    node.nextNodeIds = {};
    run.map.nodes.push_back(node);
    run.map.currentNodeId = node.id;

    run.stats.combatsWon = 4;
    run.stats.enemiesKilled = 8;
    run.stats.damageTaken = 31;
    run.stats.damageDealt = 246;
    run.stats.cardsPlayedInCombat = 39;
    run.stats.goldGained = 212;
    run.stats.goldSpent = 75;
    run.stats.cardsUpgraded = 1;
    run.stats.relicsGained = 1;
    run.stats.nodesCompleted = 6;
    return run;
}

RunState makePendingEventRun() {
    RunState run = makeMapRun();
    run.map.nodes.front().type = RunMapNodeType::Event;
    run.map.nodes.front().state = RunMapNodeState::Current;
    run.pendingRoom.type = RunPendingRoomType::Event;
    run.pendingRoom.nodeId = run.map.nodes.front().id;
    run.pendingRoom.eventId = "clockwork_confessional";
    return run;
}

void testRoundTripUsesCanonicalRepresentation() {
    const RunState source = makeMapRun();
    const Json canonical = RunStateSerializer::toJson(source);
    const RunState loaded = RunStateSerializer::fromJson(canonical, "round_trip.json");

    require(canonical.at("version") == 4, "current save version changed unexpectedly");
    require(canonical.at("phase") == "map", "map run must serialize with map phase");
    require(
        canonical.at("upgraded_deck_indices") == Json::array({0, 2}),
        "upgrade indices must be sorted, deduplicated, and range checked"
    );
    require(
        RunStateSerializer::toJson(loaded) == canonical,
        "serialize-deserialize-serialize must preserve canonical JSON"
    );
}

void testPendingRoomRoundTrip() {
    const Json canonical = RunStateSerializer::toJson(makePendingEventRun());
    const RunState loaded = RunStateSerializer::fromJson(canonical, "pending_event.json");

    require(canonical.at("phase") == "event", "pending event must serialize with event phase");
    require(loaded.pendingRoom.type == RunPendingRoomType::Event, "pending event type was lost");
    require(loaded.pendingRoom.nodeId == 7, "pending event node was lost");
    require(loaded.pendingRoom.eventId == "clockwork_confessional", "pending event id was lost");
    require(
        RunStateSerializer::toJson(loaded) == canonical,
        "pending room must survive a complete round trip"
    );
}

void testLegacyCardAndUpgradeMigration() {
    Json legacy = RunStateSerializer::toJson(makeMapRun());
    legacy["version"] = 1;
    legacy.erase("phase");
    legacy["deck_card_ids"] = Json::array({
        "cyborg_strike",
        "wanderer_guard",
        "cyborg_strike"
    });
    legacy.erase("upgraded_deck_indices");
    legacy["upgraded_card_ids"] = Json::array({"cyborg_strike", "wanderer_guard"});

    const RunState loaded = RunStateSerializer::fromJson(legacy, "legacy_v1.json");
    require(loaded.deckCardIds.size() == 3, "legacy deck size changed");
    require(loaded.deckCardIds[0].value == "replicant_strike", "cyborg card id was not migrated");
    require(
        loaded.deckCardIds[1].value == "lost_psychopath_guard",
        "wanderer card id was not migrated"
    );
    require(
        loaded.upgradedDeckIndices == std::vector<int>({0, 1}),
        "legacy upgraded card ids were not migrated to stable deck indices"
    );
}

void testPhaseMismatchIsRejected() {
    Json invalid = RunStateSerializer::toJson(makeMapRun());
    invalid["phase"] = "combat";

    requireThrowsContaining(
        [&invalid] {
            (void)RunStateSerializer::fromJson(invalid, "invalid_phase.json");
        },
        "does not match run state"
    );
}

void testAtomicSaveBackupFallbackRestoreAndDelete() {
    TemporaryDirectory temporaryDirectory;
    RunSaveSystem saves(temporaryDirectory.path());

    const RunState first = makeMapRun(101);
    const RunState second = makeMapRun(202);

    saves.saveRun(0, first);
    require(std::filesystem::is_regular_file(saves.savePath(0)), "primary save was not written");
    require(!std::filesystem::exists(saves.temporarySavePath(0)), "temporary save leaked after write");

    saves.saveRun(0, second);
    require(std::filesystem::is_regular_file(saves.backupSavePath(0)), "valid primary was not backed up");
    require(saves.loadRun(0).gold == 202, "new primary save was not loaded");

    {
        std::ofstream corruptPrimary(saves.savePath(0), std::ios::trunc);
        corruptPrimary << "{ definitely not valid JSON";
    }

    const RunSaveLoadResult fallback = saves.tryLoadRun(0);
    require(fallback.loaded, "backup fallback failed");
    require(fallback.loadedFromBackup, "fallback did not report backup usage");
    require(fallback.run.gold == 101, "fallback loaded the wrong save generation");

    saves.restoreBackupAsPrimary(0);
    const RunSaveLoadResult restored = saves.tryLoadRun(0);
    require(restored.loaded, "restored primary failed to load");
    require(!restored.loadedFromBackup, "restored primary still required backup fallback");
    require(restored.run.gold == 101, "backup restoration changed save contents");

    saves.deleteRun(0);
    require(!saves.hasRunSave(0), "deleteRun left a primary or backup save behind");
    require(!std::filesystem::exists(saves.temporarySavePath(0)), "deleteRun left a temporary save behind");
}

void testPacingStateRoundTrip() {
    RunState source = makeMapRun();
    source.pacing.activeSeconds = 123.5f;
    source.pacing.mapSeconds = 20.f;
    source.pacing.combatSeconds = 80.f;
    source.pacing.currentFloorSeconds = 45.f;
    source.pacing.floorStartNodesCompleted = 3;
    source.pacing.floorActive = true;
    source.pacing.roomActive = true;
    source.pacing.currentRoomNodeId = 7;
    source.pacing.currentRoomType = RunMapNodeType::Combat;
    source.pacing.currentRoomSeconds = 12.25f;
    source.pacing.completedRooms.push_back(RunRoomTiming{"floor1", 1, 3, RunMapNodeType::Event, 8.5f});
    source.pacing.completedFloors.push_back(RunFloorTiming{"floor1", 1, 68.f, 7});

    const Json canonical = RunStateSerializer::toJson(source);
    const RunState loaded = RunStateSerializer::fromJson(canonical, "pacing_round_trip.json");

    require(loaded.pacing.activeSeconds == source.pacing.activeSeconds, "active pacing time was lost");
    require(loaded.pacing.currentRoomNodeId == 7 && loaded.pacing.currentRoomSeconds == 12.25f,
            "active room pacing state was lost");
    require(loaded.pacing.completedRooms.size() == 1 && loaded.pacing.completedRooms.front().nodeId == 3,
            "completed room pacing samples were lost");
    require(loaded.pacing.completedFloors.size() == 1 && loaded.pacing.completedFloors.front().roomsCompleted == 7,
            "completed floor pacing samples were lost");
    require(RunStateSerializer::toJson(loaded) == canonical,
            "pacing state must survive a canonical save round trip");
}

RunState makePendingRoomRun(const RunPendingRoomType type, const RunMapNodeType nodeType) {
    RunState run = makeMapRun();
    run.map.nodes.front().type = nodeType;
    run.map.nodes.front().state = type == RunPendingRoomType::CombatReward
        ? RunMapNodeState::Completed
        : RunMapNodeState::Current;
    run.pendingRoom.type = type;
    run.pendingRoom.nodeId = run.map.nodes.front().id;

    if (type == RunPendingRoomType::CombatReward || type == RunPendingRoomType::ChestReward) {
        run.pendingRoom.reward.sourceNodeType = nodeType;
        run.pendingRoom.reward.options = {RewardOption::goldReward(42)};
    } else if (type == RunPendingRoomType::Shop) {
        run.pendingRoom.shop.mode = ShopStateMode::Shop;
        run.pendingRoom.shop.offers.push_back(ShopOffer{ShopOfferType::Card, "replicant_overclock", 55, false});
    } else if (type == RunPendingRoomType::MerchantRest) {
        run.pendingRoom.shop.mode = ShopStateMode::MerchantRest;
        run.pendingRoom.shop.maxCardPurchases = 1;
        run.pendingRoom.shop.merchantRestCardShopOpen = true;
        run.pendingRoom.shop.offers.push_back(ShopOffer{ShopOfferType::Card, "replicant_overclock", 40, false});
    } else if (type == RunPendingRoomType::Event) {
        run.pendingRoom.eventId = "clockwork_confessional";
    }
    return run;
}

void testEveryPersistentRoomKindRoundTrips() {
    struct Scenario {
        RunPendingRoomType type;
        RunMapNodeType nodeType;
        const char* label;
    };
    const std::vector<Scenario> scenarios{
        {RunPendingRoomType::CombatReward, RunMapNodeType::Elite, "combat reward"},
        {RunPendingRoomType::ChestReward, RunMapNodeType::Chest, "chest reward"},
        {RunPendingRoomType::Shop, RunMapNodeType::Shop, "shop"},
        {RunPendingRoomType::MerchantRest, RunMapNodeType::Rest, "merchant rest"},
        {RunPendingRoomType::Event, RunMapNodeType::Event, "event"}
    };

    for (const Scenario& scenario : scenarios) {
        const RunState source = makePendingRoomRun(scenario.type, scenario.nodeType);
        const Json canonical = RunStateSerializer::toJson(source);
        const RunState loaded = RunStateSerializer::fromJson(canonical, std::string(scenario.label) + ".json");
        require(loaded.pendingRoom.type == scenario.type, "pending room type changed during round trip");
        require(loaded.pendingRoom.nodeId == source.pendingRoom.nodeId, "pending room node changed during round trip");
        require(RunStateSerializer::toJson(loaded) == canonical, "pending room did not survive canonical round trip");
    }
}

struct TestCase {
    const char* name;
    void (*function)();
};
} // namespace

int main() {
    const std::vector<TestCase> tests{
        {"canonical round trip", testRoundTripUsesCanonicalRepresentation},
        {"pending room round trip", testPendingRoomRoundTrip},
        {"legacy migration", testLegacyCardAndUpgradeMigration},
        {"phase mismatch rejection", testPhaseMismatchIsRejected},
        {"pacing round trip", testPacingStateRoundTrip},
        {"all pending room kinds", testEveryPersistentRoomKindRoundTrips},
        {"backup fallback and restore", testAtomicSaveBackupFallbackRestoreAndDelete}
    };

    int failures = 0;
    for (const TestCase& test : tests) {
        try {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " persistence test(s) failed\n";
        return 1;
    }

    std::cout << "All persistence tests passed\n";
    return 0;
}
