cmake_minimum_required(VERSION 3.28)
project(IDontKnowHowToNameThisGame LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GAME_FORCE_FETCH_RAYLIB "Always build bundled raylib instead of using a system raylib package" OFF)
option(GAME_STATIC_LINK_LIBRARIES "Link libraries and GCC runtime statically when using the GNU toolchain" ON)
option(GAME_BUILD_APP "Build the raylib game executable" ON)
option(GAME_BUILD_PERSISTENCE "Build JSON-backed save persistence" ON)

set(
    GAME_RAYLIB_GIT_REPOSITORY
    "https://github.com/raysan5/raylib.git"
    CACHE STRING
    "Pinned raylib repository used by FetchContent"
)
set(
    GAME_RAYLIB_GIT_TAG
    "dbc56a87da87d973a9c5baa4e7438a9d20121d28"
    CACHE STRING
    "Pinned raylib commit used by FetchContent"
)
set(
    GAME_NLOHMANN_JSON_URL
    "https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz"
    CACHE STRING
    "Pinned nlohmann/json archive used by FetchContent"
)
set(
    GAME_NLOHMANN_JSON_SHA256
    "42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa"
    CACHE STRING
    "SHA-256 of the pinned nlohmann/json archive"
)

set(GAME_CORE_SOURCES
    src/core/Vec2.hpp
    src/active_items/ActiveItemUseContext.cpp
    src/active_items/ActiveItemEffectType.cpp
    src/active_items/ActiveItemSystem.cpp
    src/active_items/ActiveItemAcquisitionSystem.cpp
    src/active_items/ActiveItemRerollSystem.cpp
    src/active_items/ActiveItemContextSystem.cpp
    src/cards/CardPile.cpp
    src/cards/Deck.cpp
    src/cards/Hand.cpp
    src/cards/DrawSystem.cpp
    src/cards/CardUpgrade.cpp
    src/core/Random.cpp
    src/entities/EntityId.cpp
    src/entities/Health.cpp
    src/entities/StatusContainer.cpp
    src/combat/CombatResources.cpp
    src/combat/CombatLog.cpp
    src/combat/CombatState.cpp
    src/combat/CombatResult.cpp
    src/combat/CardCost.cpp
    src/combat/CardPlayValidator.cpp
    src/combat/CardPlaySystem.cpp
    src/combat/EffectResolver.cpp
    src/combat/EffectScaling.cpp
    src/combat/EffectSystem.cpp
    src/combat/EnergySystem.cpp
    src/combat/CombatController.cpp
    src/combat/BossPhaseSystem.cpp
    src/combat/PlayerTurnSystem.cpp
    src/combat/EnemyTurnSystem.cpp
    src/combat/TurnSystem.cpp
    src/combat/Targeting.cpp
    src/combat/ModifierSystem.cpp
    src/combat/DamageSystem.cpp
    src/combat/BlockSystem.cpp
    src/combat/EnemyMoveSelector.cpp
    src/enemies/EnemyIntent.cpp
    src/effects/EffectValue.cpp
    src/effects/EffectType.cpp
    src/dice/DieType.cpp
    src/drones/DroneSystem.cpp
    src/statuses/StatusSystem.cpp
    src/localization/Locale.cpp
    src/game/GameEventBus.cpp
    src/ui/EnemyIntentPresentation.cpp
    src/rewards/RewardPoolRules.cpp
    src/run/RunCardEligibility.cpp
    src/run/RunRelicOwnership.cpp
    src/events/RunEventRequirement.cpp
    src/events/RunEventSelector.cpp
    src/relics/RelicInventory.cpp
    src/relics/RelicSystem.cpp
)

set(GAME_PERSISTENCE_SOURCES
    src/data/JsonLoader.cpp
    src/save/RunStateSerializer.cpp
    src/save/RunSaveSystem.cpp
)

function(game_apply_warnings target_name)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    endif()
endfunction()

add_library(game_core STATIC ${GAME_CORE_SOURCES})
target_include_directories(game_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(game_core PUBLIC cxx_std_20)
set_target_properties(game_core PROPERTIES CXX_SCAN_FOR_MODULES OFF)
game_apply_warnings(game_core)

find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_Interpreter_FOUND)
    add_custom_target(validate_sources
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_cmake_sources.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating CMake source list"
        VERBATIM
    )

    add_custom_target(validate_content
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_localization.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_project.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_floor_definitions.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_core_readiness.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_run_save_files.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_floor_vertical_slices.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_encounter_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_group_encounter_depth.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_multi_enemy_ui_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_active_item_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_reroll_die_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_active_item_acquisition_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_active_item_expansion_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_band_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_economy_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_breakdown_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_content_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_boss_phase_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_status_driven_relics_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_card_lifecycle_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_data_driven_status_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_run_save_room_integration.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_balance_simulation_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_balance_targets_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_telemetry_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_archetype_unlock_contract.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_card_depth_contract.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating localization, data content, floor definitions, core gameplay contracts, run save files, and floor vertical slices"
        VERBATIM
    )


    add_custom_target(validate_floors
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_floor_definitions.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating floor definitions"
        VERBATIM
    )

    add_custom_target(validate_first_floor
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_floor_vertical_slices.py --floor floor1
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating the first-floor vertical slice"
        VERBATIM
    )

    add_custom_target(validate_floor_slices
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_floor_vertical_slices.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating all implemented floor vertical slices"
        VERBATIM
    )

    add_custom_target(snapshot_act1_map
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/snapshot_act1_map.py --seed 1
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Printing an act 1 map snapshot"
        VERBATIM
    )

    add_custom_target(report_upgrade_coverage
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/report_upgrade_coverage.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Reporting card upgrade coverage"
        VERBATIM
    )

    add_custom_target(report_act1_content
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/report_act1_content.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Reporting act 1 relic, event, and consumable coverage"
        VERBATIM
    )

    add_custom_target(report_merchant_economy
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/report_merchant_economy.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Reporting merchant economy tuning"
        VERBATIM
    )

    add_custom_target(report_floor_balance
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/report_floor_balance.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Reporting floor encounter pressure and balance"
        VERBATIM
    )

    add_custom_target(simulate_balance
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/simulate_balance.py --samples 2000 --turns 5
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running deterministic starter-deck and encounter balance simulations"
        VERBATIM
    )

    add_custom_target(report_run_telemetry
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/summarize_run_telemetry.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Summarizing local run telemetry"
        VERBATIM
    )
else()
    message(STATUS "Python3 was not found; validate_content target is disabled")
endif()

if(GAME_BUILD_APP OR GAME_BUILD_PERSISTENCE)
    include(FetchContent)

    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Do not update pinned dependencies after the first fetch")

    FetchContent_Declare(
        nlohmann_json
        URL ${GAME_NLOHMANN_JSON_URL}
        URL_HASH SHA256=${GAME_NLOHMANN_JSON_SHA256}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

if(GAME_BUILD_APP)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

    if (NOT GAME_FORCE_FETCH_RAYLIB)
        find_package(raylib 6.0 QUIET)
    endif()

    if (GAME_FORCE_FETCH_RAYLIB OR NOT raylib_FOUND)
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY ${GAME_RAYLIB_GIT_REPOSITORY}
            GIT_TAG ${GAME_RAYLIB_GIT_TAG}
            GIT_SHALLOW FALSE
            GIT_PROGRESS TRUE
            GIT_SUBMODULES ""
        )
        FetchContent_MakeAvailable(raylib)
    endif()
endif()

if(GAME_BUILD_PERSISTENCE)
    add_library(game_persistence STATIC ${GAME_PERSISTENCE_SOURCES})
    target_include_directories(game_persistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(game_persistence PUBLIC cxx_std_20)
    target_link_libraries(game_persistence PUBLIC game_core nlohmann_json::nlohmann_json)
    set_target_properties(game_persistence PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    game_apply_warnings(game_persistence)
endif()

if(GAME_BUILD_APP)
    if(NOT TARGET game_persistence)
        message(FATAL_ERROR "GAME_BUILD_APP requires GAME_BUILD_PERSISTENCE=ON")
    endif()

set(GAME_APP_SOURCES
    src/main.cpp

    src/app/Application.hpp
    src/app/Application.cpp
    src/app/AppConfig.hpp
    src/app/AppConfig.cpp

    src/settings/UserSettings.hpp
    src/settings/UserSettings.cpp
    src/settings/UserSettingsRepository.hpp
    src/settings/UserSettingsRepository.cpp

    src/save/RunStateSerializer.hpp
    src/save/RunStateSerializer.cpp
    src/save/RunSaveSystem.hpp
    src/save/RunSaveSystem.cpp

    src/telemetry/RunTelemetryWriter.hpp
    src/telemetry/RunTelemetryWriter.cpp

    src/core/Random.hpp
    src/core/Random.cpp

    src/data/Json.hpp
    src/data/JsonLoader.hpp
    src/data/JsonLoader.cpp
    src/data/JsonReader.hpp
    src/data/JsonReader.cpp
    src/data/ContentRegistry.hpp
    src/data/ContentRegistry.cpp
    src/data/ContentValidator.hpp
    src/data/ContentValidator.cpp
    src/data/CardDatabase.hpp
    src/data/CardDatabase.cpp
    src/data/EnemyDatabase.hpp
    src/data/EnemyDatabase.cpp
    src/data/EventDatabase.hpp
    src/data/EventDatabase.cpp

    src/challenges/ChallengeDefinition.hpp
    src/challenges/ChallengeDatabase.hpp
    src/challenges/ChallengeDatabase.cpp
    src/challenges/ChallengeRules.hpp
    src/challenges/ChallengeRules.cpp

    src/achievements/AchievementDefinition.hpp
    src/achievements/AchievementDatabase.hpp
    src/achievements/AchievementDatabase.cpp
    src/achievements/AchievementRules.hpp
    src/achievements/AchievementRules.cpp

    src/encounters/EncounterDefinition.hpp
    src/encounters/EncounterDatabase.hpp
    src/encounters/EncounterDatabase.cpp

    src/data/parsers/EffectValueParser.hpp
    src/data/parsers/EffectValueParser.cpp
    src/data/parsers/EffectDefinitionParser.hpp
    src/data/parsers/EffectDefinitionParser.cpp
    src/data/parsers/CardDefinitionParser.hpp
    src/data/parsers/CardDefinitionParser.cpp
    src/data/parsers/EnemyDefinitionParser.hpp
    src/data/parsers/EnemyDefinitionParser.cpp
    src/data/parsers/EnemyActionDefinitionParser.hpp
    src/data/parsers/EnemyActionDefinitionParser.cpp
    src/data/parsers/RunEventDefinitionParser.hpp
    src/data/parsers/RunEventDefinitionParser.cpp
    src/data/parsers/ChallengeDefinitionParser.hpp
    src/data/parsers/ChallengeDefinitionParser.cpp
    src/data/parsers/AchievementDefinitionParser.hpp
    src/data/parsers/AchievementDefinitionParser.cpp

    src/localization/Locale.hpp
    src/localization/Locale.cpp
    src/localization/TextId.hpp
    src/localization/LocalizedString.hpp
    src/localization/MissingTextPolicy.hpp
    src/localization/LocalizationBundle.hpp
    src/localization/LocalizationBundle.cpp
    src/localization/LocalizationManager.hpp
    src/localization/LocalizationManager.cpp
    src/localization/TextFormatter.hpp
    src/localization/TextFormatter.cpp

    src/game/GameEvent.hpp
    src/game/GameEvent.cpp
    src/game/GameEventBus.hpp
    src/game/GameEventBus.cpp

    src/dice/DieType.hpp
    src/dice/DieType.cpp
    src/dice/DiceExpression.hpp
    src/dice/DiceExpression.cpp
    src/dice/DiceCorruption.hpp
    src/dice/DiceCorruption.cpp

    src/effects/EffectType.hpp
    src/effects/EffectType.cpp
    src/effects/EffectTarget.hpp
    src/effects/EffectTarget.cpp
    src/effects/EffectValue.hpp
    src/effects/EffectValue.cpp
    src/effects/EffectDefinition.hpp
    src/effects/EffectScalingDefinition.hpp


    src/relics/RelicId.hpp
    src/relics/RelicRarity.hpp
    src/relics/RelicRarity.cpp
    src/relics/RelicModifierDefinition.hpp
    src/relics/RelicModifierDefinition.cpp
    src/relics/RelicTriggerDefinition.hpp
    src/relics/RelicDefinition.hpp
    src/relics/RelicInstance.hpp
    src/relics/RelicInventory.hpp
    src/relics/RelicInventory.cpp
    src/relics/RelicDatabase.hpp
    src/relics/RelicDatabase.cpp
    src/relics/RelicSystem.hpp
    src/relics/RelicSystem.cpp

    src/data/parsers/RelicDefinitionParser.hpp
    src/data/parsers/RelicDefinitionParser.cpp

    src/active_items/ActiveItemId.hpp
    src/active_items/ActiveItemUseContext.hpp
    src/active_items/ActiveItemUseContext.cpp
    src/active_items/ActiveItemEffectType.hpp
    src/active_items/ActiveItemEffectType.cpp
    src/active_items/ActiveItemDefinition.hpp
    src/active_items/ActiveItemState.hpp
    src/active_items/ActiveItemUseResult.hpp
    src/active_items/ActiveItemDatabase.hpp
    src/active_items/ActiveItemDatabase.cpp
    src/active_items/ActiveItemSystem.hpp
    src/active_items/ActiveItemSystem.cpp
    src/active_items/ActiveItemAcquisitionSystem.hpp
    src/active_items/ActiveItemAcquisitionSystem.cpp
    src/active_items/ActiveItemRerollSystem.hpp
    src/active_items/ActiveItemRerollSystem.cpp
    src/active_items/ActiveItemContextSystem.hpp
    src/active_items/ActiveItemContextSystem.cpp
    src/data/parsers/ActiveItemDefinitionParser.hpp
    src/data/parsers/ActiveItemDefinitionParser.cpp

    src/consumables/ConsumableId.hpp
    src/consumables/ConsumableRarity.hpp
    src/consumables/ConsumableRarity.cpp
    src/consumables/ConsumableDefinition.hpp
    src/consumables/ConsumableDatabase.hpp
    src/consumables/ConsumableDatabase.cpp
    src/consumables/ConsumableSystem.hpp
    src/consumables/ConsumableSystem.cpp

    src/data/parsers/ConsumableDefinitionParser.hpp
    src/data/parsers/ConsumableDefinitionParser.cpp
    src/statuses/StatusId.hpp
    src/statuses/StatusType.hpp
    src/statuses/StatusType.cpp
    src/statuses/StatusDurationRule.hpp
    src/statuses/StatusDurationRule.cpp
    src/statuses/StatusDefinition.hpp
    src/statuses/StatusDatabase.hpp
    src/statuses/StatusDatabase.cpp
    src/statuses/StatusSystem.hpp
    src/statuses/StatusSystem.cpp

    src/events/RunEventEffect.hpp
    src/events/RunEventChoiceResult.hpp
    src/events/RunEventChoicePreviewFormatter.hpp
    src/events/RunEventChoicePreviewFormatter.cpp
    src/events/RunEventRequirement.hpp
    src/events/RunEventRequirement.cpp
    src/events/RunEventSelector.hpp
    src/events/RunEventSelector.cpp
    src/events/RunEventDefinition.hpp

    src/shop/ShopOffer.hpp
    src/shop/ShopState.hpp
    src/shop/ShopTuning.hpp
    src/shop/ShopTuning.cpp

    src/drones/DroneId.hpp
    src/drones/DroneActionDefinition.hpp
    src/drones/DroneDefinition.hpp
    src/drones/DroneDatabase.hpp
    src/drones/DroneDatabase.cpp
    src/drones/DroneSystem.hpp
    src/drones/DroneSystem.cpp

    src/data/parsers/StatusDefinitionParser.hpp
    src/data/parsers/DroneDefinitionParser.hpp
    src/data/parsers/DroneDefinitionParser.cpp
    src/data/parsers/StatusDefinitionParser.cpp

    src/cards/CardId.hpp
    src/cards/CardType.hpp
    src/cards/CardType.cpp
    src/cards/CardRarity.hpp
    src/cards/CardRarity.cpp
    src/cards/CardKeyword.hpp
    src/cards/CardKeyword.cpp
    src/cards/CardDefinition.hpp
    src/cards/CardDescriptionFormatter.hpp
    src/cards/CardDescriptionFormatter.cpp
    src/cards/CardUpgrade.hpp
    src/cards/CardUpgrade.cpp
    src/cards/CardInstanceId.hpp
    src/cards/CardInstance.hpp
    src/cards/CardInstanceFactory.hpp
    src/cards/CardInstanceFactory.cpp
    src/cards/CardPile.hpp
    src/cards/CardPile.cpp
    src/cards/Deck.hpp
    src/cards/Deck.cpp
    src/cards/Hand.hpp
    src/cards/Hand.cpp
    src/cards/DrawSystem.hpp
    src/cards/DrawSystem.cpp

    src/entities/EntityId.hpp
    src/entities/EntityId.cpp
    src/entities/EntityType.hpp
    src/entities/Health.hpp
    src/entities/Health.cpp
    src/entities/StatusContainer.hpp
    src/entities/StatusContainer.cpp
    src/entities/CombatEntity.hpp

    src/enemies/EnemyId.hpp
    src/enemies/EnemyRole.hpp
    src/enemies/EnemyIntent.hpp
    src/enemies/EnemyIntent.cpp
    src/enemies/EnemyActionCondition.hpp
    src/enemies/EnemyActionDefinition.hpp
    src/enemies/EnemyPhaseDefinition.hpp
    src/enemies/EnemyDefinition.hpp
    src/enemies/EnemyInstance.hpp

    src/combat/CombatPhase.hpp
    src/combat/CombatResources.hpp
    src/combat/CombatResources.cpp
    src/combat/CombatTelemetry.hpp
    src/combat/CombatLog.hpp
    src/combat/CombatLog.cpp
    src/combat/CombatState.hpp
    src/combat/CombatState.cpp
    src/combat/CombatOutcome.hpp
    src/combat/CombatResult.hpp
    src/combat/CombatResult.cpp
    src/combat/CombatController.hpp
    src/combat/CombatController.cpp
    src/combat/EnemyIntentState.hpp
    src/combat/EnemyAiState.hpp
    src/combat/BossPhaseRules.hpp
    src/combat/BossPhaseSystem.hpp
    src/combat/BossPhaseSystem.cpp
    src/combat/EnemyMoveSelector.hpp
    src/combat/EnemyMoveSelector.cpp
    src/combat/PlayerTurnSystem.hpp
    src/combat/PlayerTurnSystem.cpp
    src/combat/EnemyTurnSystem.hpp
    src/combat/EnemyTurnSystem.cpp
    src/combat/TurnSystem.hpp
    src/combat/TurnSystem.cpp
    src/combat/ModifierSystem.hpp
    src/combat/ModifierSystem.cpp
    src/combat/EffectContext.hpp
    src/combat/ResolvedEffectValue.hpp
    src/combat/EffectResolver.hpp
    src/combat/EffectResolver.cpp
    src/combat/EffectScaling.hpp
    src/combat/EffectScaling.cpp
    src/combat/Targeting.hpp
    src/combat/Targeting.cpp
    src/combat/DamagePreview.hpp
    src/combat/DamageSystem.hpp
    src/combat/DamageSystem.cpp
    src/combat/BlockSystem.hpp
    src/combat/BlockSystem.cpp
    src/combat/EnergySystem.hpp
    src/combat/EnergySystem.cpp
    src/combat/CardCost.hpp
    src/combat/CardCost.cpp
    src/combat/CardPlayFailureReason.hpp
    src/combat/EffectSystem.hpp
    src/combat/EffectSystem.cpp
    src/combat/CardPlayValidator.hpp
    src/combat/CardPlayValidator.cpp
    src/combat/CardPlaySystem.hpp
    src/combat/CardPlaySystem.cpp

    src/preview/PreviewValue.hpp
    src/preview/EffectPreview.hpp
    src/preview/CardPreview.hpp
    src/preview/CardPreviewSystem.hpp
    src/preview/CardPreviewSystem.cpp

    src/ui/CardTransform.hpp
    src/ui/StatusViewModel.hpp
    src/ui/CardViewModel.hpp
    src/ui/RelicViewModel.hpp
    src/ui/CardViewModelFactory.hpp
    src/ui/CardViewModelFactory.cpp
    src/ui/EnemyViewModel.hpp
    src/ui/EnemyIntentPresentation.hpp
    src/ui/EnemyIntentPresentation.cpp
    src/ui/PlayerViewModel.hpp
    src/ui/ConsumableViewModel.hpp
    src/ui/CombatViewModel.hpp
    src/ui/UiFont.hpp
    src/ui/UiFont.cpp
    src/ui/Utf8.hpp
    src/ui/VirtualViewport.hpp
    src/ui/CardVisualInstance.hpp
    src/ui/CardVisualInstance.cpp
    src/ui/CardView.hpp
    src/ui/CardView.cpp
    src/ui/HandLayout.hpp
    src/ui/HandLayout.cpp
    src/ui/HandView.hpp
    src/ui/HandView.cpp
    src/ui/EnemyView.hpp
    src/ui/EnemyView.cpp
    src/ui/PlayerView.hpp
    src/ui/PlayerView.cpp
    src/ui/CardViewModelBuilder.hpp
    src/ui/CardViewModelBuilder.cpp
    src/ui/CombatViewModelBuilder.hpp
    src/ui/CombatViewModelBuilder.cpp
    src/ui/CombatView.hpp
    src/ui/CombatView.cpp
    src/inspect/InspectEntry.hpp
    src/inspect/InspectPanelModel.hpp
    src/inspect/InspectModelBuilder.hpp
    src/inspect/InspectModelBuilder.cpp
    src/ui/InspectPanelView.hpp
    src/ui/InspectPanelView.cpp
    src/ui/RelicInspectModal.hpp
    src/ui/RelicInspectModal.cpp

    src/data/parsers/PlayableArchetypeParser.hpp
    src/data/parsers/PlayableArchetypeParser.cpp
    src/data/parsers/PlayerActorParser.hpp
    src/data/parsers/PlayerActorParser.cpp
    src/data/parsers/DifficultyDefinitionParser.hpp
    src/data/parsers/DifficultyDefinitionParser.cpp
    src/profile/ProfileCountEntry.hpp
    src/profile/ProfileData.hpp
    src/profile/ProfileProgressEntry.hpp
    src/profile/ProfileRunStatistics.hpp
    src/profile/ProfileSlot.hpp
    src/profile/UnlockReward.hpp
    src/profile/ProfileManager.hpp
    src/profile/ProfileManager.cpp
    src/profile/ProfileSaveSystem.hpp
    src/profile/ProfileSaveSystem.cpp
    src/archetypes/PlayableArchetypeId.hpp
    src/archetypes/PlayableArchetypeDefinition.hpp
    src/archetypes/PlayableArchetypeDatabase.hpp
    src/archetypes/PlayableArchetypeDatabase.cpp
    src/actors/PlayerActorId.hpp
    src/actors/PlayerActorDefinition.hpp
    src/actors/PlayerActorDatabase.hpp
    src/actors/PlayerActorDatabase.cpp
    src/run/DifficultyId.hpp
    src/run/DifficultyDefinition.hpp
    src/run/DifficultyDatabase.hpp
    src/run/DifficultyDatabase.cpp
    src/run/FloorDefinition.hpp
    src/run/FloorDatabase.hpp
    src/run/FloorDatabase.cpp
    src/run/RunCompletionType.hpp
    src/run/RunEndReason.hpp
    src/run/RunCompletion.hpp
    src/run/RunCompletion.cpp
    src/run/RunMapNodeType.hpp
    src/run/RunMapNode.hpp
    src/run/RunMapNode.cpp
    src/run/RunMap.hpp
    src/run/RunState.hpp
    src/run/RunPendingRoomState.hpp
    src/run/RunPhase.hpp
    src/run/RunMapGenerationConfig.hpp
    src/run/RunMapGenerationConfig.cpp
    src/run/RunMapGenerator.hpp
    src/run/RunMapGenerator.cpp
    src/run/RunFactory.hpp
    src/run/RunFactory.cpp
    src/run/RunStats.hpp
    src/run/RunActorState.hpp
    src/run/StressRules.hpp
    src/run/StressPsychopathRules.hpp
    src/run/StressBreakdownRules.hpp
    src/run/StressEconomyRules.hpp
    src/run/SadistMasochistRules.hpp
    src/run/SadistMasochistRules.cpp
    src/run/RunRelicOwnership.hpp
    src/run/RunRelicOwnership.cpp
    src/run/RunCardEligibility.hpp
    src/run/RunCardEligibility.cpp
    src/run/RunController.hpp
    src/run/RunController.cpp

    src/rewards/CardRewardOption.hpp
    src/rewards/RewardOption.hpp
    src/rewards/RewardContext.hpp
    src/rewards/RewardSelection.hpp
    src/rewards/RewardState.hpp
    src/rewards/RewardTuning.hpp
    src/rewards/RewardTuning.cpp
    src/rewards/RewardGenerator.hpp
    src/rewards/RewardGenerator.cpp
    src/rewards/RewardPoolRules.hpp
    src/rewards/RewardPoolRules.cpp
    src/rewards/RewardSystem.hpp
    src/rewards/RewardSystem.cpp
    src/flow/SceneId.hpp
    src/flow/SceneManager.hpp
    src/flow/SceneManager.cpp
    src/flow/GameFlowController.hpp
    src/flow/GameFlowController.cpp
    src/ui/BasicUi.hpp
    src/ui/BasicUi.cpp
    src/ui/ActiveItemComparisonView.hpp
    src/ui/ActiveItemComparisonView.cpp
    src/scenes/Scene.hpp
    src/scenes/SplashScene.hpp
    src/scenes/SplashScene.cpp
    src/progression/ChallengeEvaluator.hpp
    src/progression/ChallengeEvaluator.cpp
    src/progression/AchievementEvaluator.hpp
    src/progression/AchievementEvaluator.cpp
    src/progression/UnlockEvaluator.hpp
    src/progression/UnlockEvaluator.cpp
    src/scenes/MainMenuScene.hpp
    src/scenes/MainMenuScene.cpp
    src/scenes/SettingsScene.hpp
    src/scenes/SettingsScene.cpp
    src/scenes/SaveSlotScene.hpp
    src/scenes/SaveSlotScene.cpp
    src/scenes/ProfileHubScene.hpp
    src/scenes/ProfileHubScene.cpp
    src/scenes/ChallengeScene.hpp
    src/scenes/ChallengeScene.cpp
    src/scenes/AchievementScene.hpp
    src/scenes/AchievementScene.cpp
    src/scenes/CompendiumScene.hpp
    src/scenes/CompendiumScene.cpp
    src/scenes/ProfileProgressScene.hpp
    src/scenes/ProfileProgressScene.cpp
    src/scenes/DifficultySelectScene.hpp
    src/scenes/DifficultySelectScene.cpp
    src/scenes/RunMapScene.hpp
    src/scenes/RunMapScene.cpp
    src/scenes/MerchantRestScene.hpp
    src/scenes/MerchantRestScene.cpp
    src/scenes/CombatScene.hpp
    src/scenes/CombatScene.cpp
    src/scenes/CombatScenePiles.cpp
    src/scenes/CombatSceneRewards.cpp
    src/scenes/CombatSceneInputTargeting.cpp
    src/scenes/CombatSceneInspectConsumables.cpp
    src/scenes/RewardScene.hpp
    src/scenes/RewardScene.cpp
    src/scenes/FloorCompleteScene.hpp
    src/scenes/FloorCompleteScene.cpp
    src/scenes/RunDefeatScene.hpp
    src/scenes/RunDefeatScene.cpp
    src/scenes/RunCompleteScene.hpp
    src/scenes/RunCompleteScene.cpp
    src/scenes/ShopScene.hpp
    src/scenes/ShopScene.cpp
    src/scenes/EventScene.hpp
    src/scenes/EventScene.cpp
    src/scenes/EventOutcomeScene.hpp
    src/scenes/EventOutcomeScene.cpp
)

list(REMOVE_ITEM GAME_APP_SOURCES ${GAME_CORE_SOURCES} ${GAME_PERSISTENCE_SOURCES})

add_executable(i_dont_know_how_to_name_this_game ${GAME_APP_SOURCES})

target_include_directories(i_dont_know_how_to_name_this_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(i_dont_know_how_to_name_this_game PRIVATE
    cxx_std_20
)

game_apply_warnings(i_dont_know_how_to_name_this_game)

target_link_libraries(i_dont_know_how_to_name_this_game PRIVATE
    game_core
    game_persistence
    raylib
    nlohmann_json::nlohmann_json
)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND GAME_STATIC_LINK_LIBRARIES)
    target_link_options(i_dont_know_how_to_name_this_game PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

add_custom_command(TARGET i_dont_know_how_to_name_this_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        $<TARGET_FILE_DIR:i_dont_know_how_to_name_this_game>/config
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        $<TARGET_FILE_DIR:i_dont_know_how_to_name_this_game>/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:i_dont_know_how_to_name_this_game>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:i_dont_know_how_to_name_this_game>/data
)

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET i_dont_know_how_to_name_this_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:i_dont_know_how_to_name_this_game>/assets
    )
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT i_dont_know_how_to_name_this_game)

endif() # GAME_BUILD_APP

include(CTest)

if(BUILD_TESTING)
    add_executable(game_core_tests
        tests/CombatCoreTests.cpp
    )

    target_include_directories(game_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(game_core_tests PRIVATE cxx_std_20)
    target_link_libraries(game_core_tests PRIVATE game_core)
    set_target_properties(game_core_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    game_apply_warnings(game_core_tests)

    add_executable(gameplay_integration_tests
        tests/GameplayIntegrationTests.cpp
    )

    target_include_directories(gameplay_integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(gameplay_integration_tests PRIVATE cxx_std_20)
    target_link_libraries(gameplay_integration_tests PRIVATE game_core)
    set_target_properties(gameplay_integration_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    game_apply_warnings(gameplay_integration_tests)

    add_test(NAME combat_core COMMAND game_core_tests)
    set_tests_properties(combat_core PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_test(NAME gameplay_integration COMMAND gameplay_integration_tests)
    set_tests_properties(gameplay_integration PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    if(TARGET game_persistence)
        add_executable(run_state_persistence_tests tests/RunStatePersistenceTests.cpp)
        target_include_directories(run_state_persistence_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_compile_features(run_state_persistence_tests PRIVATE cxx_std_20)
        target_link_libraries(run_state_persistence_tests PRIVATE game_persistence)
        set_target_properties(run_state_persistence_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
        game_apply_warnings(run_state_persistence_tests)

        add_test(NAME run_state_persistence COMMAND run_state_persistence_tests)
        set_tests_properties(run_state_persistence PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()

    if(Python3_Interpreter_FOUND)
        add_test(NAME encounter_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_encounter_contract.py)
        set_tests_properties(encounter_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME group_encounter_depth COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_group_encounter_depth.py)
        set_tests_properties(group_encounter_depth PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME multi_enemy_ui_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_multi_enemy_ui_contract.py)
        set_tests_properties(multi_enemy_ui_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME enemy_ai_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_enemy_ai_contract.py)
        set_tests_properties(enemy_ai_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME active_item_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_active_item_contract.py)
        set_tests_properties(active_item_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME reroll_die_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_reroll_die_contract.py)
        set_tests_properties(reroll_die_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME active_item_acquisition_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_active_item_acquisition_contract.py)
        set_tests_properties(active_item_acquisition_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME active_item_expansion_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_active_item_expansion_contract.py)
        set_tests_properties(active_item_expansion_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME stress_band_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_band_contract.py)
        set_tests_properties(stress_band_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME stress_economy_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_economy_contract.py)
        set_tests_properties(stress_economy_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME stress_breakdown_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_breakdown_contract.py)
        set_tests_properties(stress_breakdown_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME stress_content_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_stress_content_contract.py)
        set_tests_properties(stress_content_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME boss_phase_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_boss_phase_contract.py)
        set_tests_properties(boss_phase_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME status_driven_relics_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_status_driven_relics_contract.py)
        set_tests_properties(status_driven_relics_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME card_lifecycle_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_card_lifecycle_contract.py)
        set_tests_properties(card_lifecycle_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME data_driven_status_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_data_driven_status_contract.py)
        set_tests_properties(data_driven_status_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME run_save_room_integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_run_save_room_integration.py)
        set_tests_properties(run_save_room_integration PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME balance_simulation_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_balance_simulation_contract.py)
        set_tests_properties(balance_simulation_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME balance_targets_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_balance_targets_contract.py)
        set_tests_properties(balance_targets_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME telemetry_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_telemetry_contract.py)
        set_tests_properties(telemetry_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME archetype_unlock_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_archetype_unlock_contract.py)
        set_tests_properties(archetype_unlock_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME card_depth_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_card_depth_contract.py)
        set_tests_properties(card_depth_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME relic_expansion_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_relic_expansion_contract.py)
        set_tests_properties(relic_expansion_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

        add_test(NAME event_chain_contract COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_event_chain_contract.py)
        set_tests_properties(event_chain_contract PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()
endif()
