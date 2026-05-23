cmake_minimum_required(VERSION 3.28)
project(IDontKnowHowToNameThisGame LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

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

    src/data/Json.hpp
    src/data/JsonLoader.hpp
    src/data/JsonLoader.cpp
    src/data/JsonReader.hpp
    src/data/JsonReader.cpp

    src/dice/DieType.hpp
    src/dice/DieType.cpp
    src/dice/DiceExpression.hpp
    src/dice/DiceExpression.cpp
    src/dice/DiceCorruption.hpp
    src/dice/DiceCorruption.cpp
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
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    nlohmann_json::nlohmann_json
)

add_custom_target(copy_runtime_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        ${CMAKE_BINARY_DIR}/config
)

add_dependencies(i_dont_know_how_to_name_this_game copy_runtime_files)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT i_dont_know_how_to_name_this_game)
