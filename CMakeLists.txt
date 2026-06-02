cmake_minimum_required(VERSION 3.28)
project(IDontKnowHowToNameThisGame LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GAME_FORCE_FETCH_RAYLIB "Always build bundled raylib instead of using a system raylib package" OFF)
option(GAME_STATIC_LINK_LIBRARIES "Link libraries and GCC runtime statically when using the GNU toolchain" ON)

find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_Interpreter_FOUND)
    add_custom_target(validate_content
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_localization.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_project.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating localization and data content"
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
else()
    message(STATUS "Python3 was not found; validate_content target is disabled")
endif()

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)

if (NOT GAME_FORCE_FETCH_RAYLIB)
    find_package(raylib 6.0 QUIET)
endif()

if (GAME_FORCE_FETCH_RAYLIB OR NOT raylib_FOUND)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    )
    FetchContent_MakeAvailable(raylib)
endif()

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(i_dont_know_how_to_name_this_game
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
    src/events/RunEventRequirement.hpp
    src/events/RunEventRequirement.cpp
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
    src/enemies/EnemyIntent.hpp
    src/enemies/EnemyIntent.cpp
    src/enemies/EnemyActionDefinition.hpp
    src/enemies/EnemyDefinition.hpp
    src/enemies/EnemyInstance.hpp

    src/combat/CombatPhase.hpp
    src/combat/CombatResources.hpp
    src/combat/CombatResources.cpp
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
    src/profile/ProfileData.hpp
    src/profile/ProfileSlot.hpp
    src/profile/ProfileManager.hpp
    src/profile/ProfileManager.cpp
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
    src/run/RunMapNode.hpp
    src/run/RunMapNode.cpp
    src/run/RunMap.hpp
    src/run/RunState.hpp
    src/run/RunPendingRoomState.hpp
    src/run/RunMapGenerationConfig.hpp
    src/run/RunMapGenerationConfig.cpp
    src/run/RunMapGenerator.hpp
    src/run/RunMapGenerator.cpp
    src/run/RunFactory.hpp
    src/run/RunFactory.cpp
    src/run/RunStats.hpp
    src/run/RunActorState.hpp
    src/run/StressRules.hpp
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
    src/scenes/Scene.hpp
    src/scenes/SplashScene.hpp
    src/scenes/SplashScene.cpp
    src/scenes/MainMenuScene.hpp
    src/scenes/MainMenuScene.cpp
    src/scenes/SettingsScene.hpp
    src/scenes/SettingsScene.cpp
    src/scenes/SaveSlotScene.hpp
    src/scenes/SaveSlotScene.cpp
    src/scenes/ProfileHubScene.hpp
    src/scenes/ProfileHubScene.cpp
    src/scenes/DifficultySelectScene.hpp
    src/scenes/DifficultySelectScene.cpp
    src/scenes/RunMapScene.hpp
    src/scenes/RunMapScene.cpp
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
    src/scenes/ShopScene.hpp
    src/scenes/ShopScene.cpp
    src/scenes/EventScene.hpp
    src/scenes/EventScene.cpp
)


target_include_directories(i_dont_know_how_to_name_this_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(i_dont_know_how_to_name_this_game PRIVATE
    cxx_std_20
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(i_dont_know_how_to_name_this_game PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(i_dont_know_how_to_name_this_game PRIVATE
        /W4
        /permissive-
    )
endif()

target_link_libraries(i_dont_know_how_to_name_this_game PRIVATE
    raylib
    nlohmann_json::nlohmann_json
)

# The game is distributed as a small standalone binary first, so prefer static
# linking by default when building with GCC/MinGW. This intentionally restores
# the old project behavior while keeping a CMake option for package-manager or
# Linux/macOS builds that cannot provide every dependency as a static library.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND GAME_STATIC_LINK_LIBRARIES)
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
