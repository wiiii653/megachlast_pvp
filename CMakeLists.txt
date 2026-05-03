cmake_minimum_required(VERSION 3.20)
project(megablast_pvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable Address/UndefinedBehavior sanitizers" OFF)
option(USE_BUNDLED_SFML "Prefer bundled SFML at ./SFML-3.0.1 when available" ON)

function(enable_project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(enable_project_sanitizers target)
    if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

function(configure_project_target target)
    enable_project_warnings(${target})
    enable_project_sanitizers(${target})
endfunction()

function(add_project_test_executable test_name)
    set(target "${test_name}_test")
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    configure_project_target(${target})
    add_test(NAME ${test_name} COMMAND ${target})
endfunction()

enable_testing()

add_executable(megablast_pvp_sfml
    src/AppRuntime.cpp
    src/AssetRuntime.cpp
    src/BoardRuntime.cpp
    src/EffectsRuntime.cpp
    src/FxRuntime.cpp
    src/main_sfml.cpp
    src/CliOptions.cpp
    src/FrameRuntime.cpp
    src/GameUpdateRuntime.cpp
    src/GraphicsRuntime.cpp
    src/HudRuntime.cpp
    src/InputRuntime.cpp
    src/ProjectileRuntime.cpp
    src/RoundRuntime.cpp
    src/SimulationRuntime.cpp
    src/PerfLog.cpp)

# ProceduralSynth library (shared between game and test)
add_library(procedural_synth STATIC src/ProceduralSynth.cpp)
add_library(arena_layout STATIC src/ArenaLayout.cpp)
add_library(particles STATIC src/Particles.cpp)
add_library(settings_io STATIC src/SettingsIO.cpp)
add_library(audio_runtime STATIC src/AudioRuntime.cpp)
add_library(bot_controller STATIC src/BotController.cpp)
add_library(render_runtime STATIC src/RenderRuntime.cpp src/RenderRuntimeComposite.cpp)
configure_project_target(megablast_pvp_sfml)
configure_project_target(procedural_synth)
configure_project_target(arena_layout)
configure_project_target(particles)
configure_project_target(settings_io)
configure_project_target(audio_runtime)
configure_project_target(bot_controller)
configure_project_target(render_runtime)

add_project_test_executable(settings_parse tests/settings_parse_test.cpp)
add_project_test_executable(game_logic tests/game_logic_test.cpp)
add_project_test_executable(arena_layout tests/arena_layout_test.cpp)
add_project_test_executable(particles tests/particles_test.cpp)
add_project_test_executable(fx_governor tests/fx_governor_test.cpp)
add_project_test_executable(audio_duck tests/audio_duck_test.cpp)
add_project_test_executable(bot_controller tests/bot_controller_test.cpp)
add_project_test_executable(settings_file_integration tests/settings_file_integration_test.cpp)
add_project_test_executable(cli_options tests/cli_options_test.cpp src/CliOptions.cpp)
add_project_test_executable(board_runtime tests/board_runtime_test.cpp src/BoardRuntime.cpp)
add_project_test_executable(fx_runtime tests/fx_runtime_test.cpp src/FxRuntime.cpp)
add_project_test_executable(effects_runtime tests/effects_runtime_test.cpp src/EffectsRuntime.cpp)
add_project_test_executable(audio_runtime tests/audio_runtime_test.cpp src/AssetRuntime.cpp)

set(LOCAL_SFML_DIR "${CMAKE_SOURCE_DIR}/SFML-3.0.1")
set(USE_LOCAL_SFML OFF)
if(USE_BUNDLED_SFML AND EXISTS "${LOCAL_SFML_DIR}/lib/cmake/SFML/SFMLConfig.cmake")
    message(STATUS "Using local SFML package: ${LOCAL_SFML_DIR}")
    list(PREPEND CMAKE_PREFIX_PATH "${LOCAL_SFML_DIR}")
    set(USE_LOCAL_SFML ON)
elseif(USE_BUNDLED_SFML)
    message(STATUS "Bundled SFML requested but not found at ${LOCAL_SFML_DIR}; falling back to system SFML.")
endif()

find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)
target_link_libraries(megablast_pvp_sfml PRIVATE procedural_synth arena_layout particles settings_io audio_runtime bot_controller render_runtime SFML::Graphics SFML::Window SFML::System SFML::Audio)
target_link_libraries(arena_layout_test PRIVATE arena_layout)
target_link_libraries(particles_test PRIVATE particles)
target_link_libraries(settings_file_integration_test PRIVATE settings_io)
target_link_libraries(board_runtime_test PRIVATE arena_layout bot_controller SFML::Graphics)
target_link_libraries(effects_runtime_test PRIVATE particles)
target_link_libraries(audio_runtime_test PRIVATE audio_runtime)
target_link_libraries(bot_controller PRIVATE procedural_synth)
target_link_libraries(render_runtime PRIVATE SFML::Graphics)
target_link_libraries(audio_runtime PRIVATE SFML::Audio)
target_link_libraries(procedural_synth   PRIVATE SFML::Audio)
target_include_directories(procedural_synth PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(arena_layout PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(particles PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(settings_io PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(audio_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(bot_controller PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(render_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)

if(USE_LOCAL_SFML)
    set_target_properties(megablast_pvp_sfml PROPERTIES
        BUILD_RPATH   "${LOCAL_SFML_DIR}/lib"
        INSTALL_RPATH "${LOCAL_SFML_DIR}/lib")
endif()

add_test(NAME cli_help COMMAND megablast_pvp_sfml --help)
add_test(NAME cli_invalid_flag COMMAND megablast_pvp_sfml --bad-flag)
set_tests_properties(cli_invalid_flag PROPERTIES WILL_FAIL TRUE)
add_test(NAME cli_missing_perf_value COMMAND megablast_pvp_sfml --perf)
set_tests_properties(cli_missing_perf_value PROPERTIES WILL_FAIL TRUE)

if(USE_LOCAL_SFML)
    if(WIN32)
        set(_LOCAL_SFML_TEST_ENV "PATH=${LOCAL_SFML_DIR}/lib;$ENV{PATH}")
    elseif(APPLE)
        set(_LOCAL_SFML_TEST_ENV "DYLD_LIBRARY_PATH=${LOCAL_SFML_DIR}/lib")
    elseif(UNIX)
        set(_LOCAL_SFML_TEST_ENV "LD_LIBRARY_PATH=${LOCAL_SFML_DIR}/lib")
    endif()

    if(DEFINED _LOCAL_SFML_TEST_ENV)
        set_tests_properties(board_runtime PROPERTIES
            ENVIRONMENT "${_LOCAL_SFML_TEST_ENV}")
        set_tests_properties(audio_runtime PROPERTIES
            ENVIRONMENT "${_LOCAL_SFML_TEST_ENV}")
    endif()
endif()

# Convenience run target (launches from project root so assets/ is found)
if(USE_LOCAL_SFML)
    if(WIN32)
        set(_LOCAL_SFML_RUN_ENV "PATH=${LOCAL_SFML_DIR}/lib;$ENV{PATH}")
    elseif(APPLE)
        set(_LOCAL_SFML_RUN_ENV "DYLD_LIBRARY_PATH=${LOCAL_SFML_DIR}/lib")
    elseif(UNIX)
        set(_LOCAL_SFML_RUN_ENV "LD_LIBRARY_PATH=${LOCAL_SFML_DIR}/lib")
    endif()

    if(DEFINED _LOCAL_SFML_RUN_ENV)
        add_custom_target(run
            COMMAND ${CMAKE_COMMAND} -E env "${_LOCAL_SFML_RUN_ENV}" $<TARGET_FILE:megablast_pvp_sfml>
            DEPENDS megablast_pvp_sfml
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running Megachlast PvP..."
        )
    else()
        add_custom_target(run
            COMMAND $<TARGET_FILE:megablast_pvp_sfml>
            DEPENDS megablast_pvp_sfml
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running Megachlast PvP..."
        )
    endif()
else()
    add_custom_target(run
        COMMAND $<TARGET_FILE:megablast_pvp_sfml>
        DEPENDS megablast_pvp_sfml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running Megachlast PvP..."
    )
endif()
