cmake_minimum_required(VERSION 3.20)
project(megablast_pvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable Address/UndefinedBehavior sanitizers" OFF)
set(APP_VERSION "EA-0.1.0")
set(APP_GIT_HASH "nogit")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _git_hash_result
        OUTPUT_VARIABLE _git_hash_output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_git_hash_result EQUAL 0 AND NOT _git_hash_output STREQUAL "")
        set(APP_GIT_HASH "${_git_hash_output}")
    endif()
endif()
set(APP_VERSION_LABEL "${APP_VERSION}+${APP_GIT_HASH}")
configure_file("${CMAKE_SOURCE_DIR}/src/BuildInfo.h.in"
               "${CMAKE_BINARY_DIR}/generated/BuildInfo.h" @ONLY)

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
    src/RoundAudioRuntime.cpp
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
add_project_test_executable(controls_config tests/controls_config_test.cpp)
add_project_test_executable(input_runtime tests/input_runtime_test.cpp src/InputRuntime.cpp src/GameUpdateRuntime.cpp)
add_project_test_executable(game_logic tests/game_logic_test.cpp)
add_project_test_executable(arena_layout tests/arena_layout_test.cpp)
add_project_test_executable(particles tests/particles_test.cpp)
add_project_test_executable(fx_governor tests/fx_governor_test.cpp)
add_project_test_executable(audio_duck tests/audio_duck_test.cpp)
add_project_test_executable(bot_controller tests/bot_controller_test.cpp)
add_project_test_executable(settings_file_integration tests/settings_file_integration_test.cpp)
add_project_test_executable(cli_options tests/cli_options_test.cpp src/CliOptions.cpp)
add_project_test_executable(board_runtime tests/board_runtime_test.cpp src/BoardRuntime.cpp)
add_project_test_executable(round_runtime tests/round_runtime_test.cpp src/RoundRuntime.cpp)
add_project_test_executable(frame_runtime tests/frame_runtime_test.cpp src/FrameRuntime.cpp)
add_project_test_executable(render_runtime tests/render_runtime_test.cpp)
add_project_test_executable(fx_runtime tests/fx_runtime_test.cpp src/FxRuntime.cpp)
add_project_test_executable(effects_runtime tests/effects_runtime_test.cpp src/EffectsRuntime.cpp)
add_project_test_executable(audio_runtime tests/audio_runtime_test.cpp src/AssetRuntime.cpp)
add_project_test_executable(projectile_runtime tests/projectile_runtime_test.cpp src/ProjectileRuntime.cpp)
add_project_test_executable(input_state tests/input_state_test.cpp src/GameUpdateRuntime.cpp)

set(SFML_PROVIDER "AUTO" CACHE STRING "SFML provider: AUTO, SYSTEM, or FETCH")
set_property(CACHE SFML_PROVIDER PROPERTY STRINGS AUTO SYSTEM FETCH)
if(NOT SFML_PROVIDER MATCHES "^(AUTO|SYSTEM|FETCH)$")
    message(FATAL_ERROR "Invalid SFML_PROVIDER: ${SFML_PROVIDER}")
endif()
if(NOT SFML_PROVIDER STREQUAL "FETCH")
    find_package(SFML 3 COMPONENTS Graphics Window System Audio QUIET)
endif()
if(SFML_PROVIDER STREQUAL "FETCH" OR NOT SFML_FOUND)
    if(SFML_PROVIDER STREQUAL "SYSTEM")
        message(FATAL_ERROR "Install SFML 3 or use -DSFML_PROVIDER=FETCH")
    endif()
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependency libraries" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 016bea9491ccafc3529019fe1d403885a8b3a6ae)
    FetchContent_MakeAvailable(SFML)
endif()
target_link_libraries(megablast_pvp_sfml PRIVATE procedural_synth arena_layout particles settings_io audio_runtime bot_controller render_runtime SFML::Graphics SFML::Window SFML::System SFML::Audio)
target_link_libraries(arena_layout_test PRIVATE arena_layout)
target_link_libraries(particles_test PRIVATE particles)
target_link_libraries(settings_file_integration_test PRIVATE settings_io SFML::Window)
target_link_libraries(board_runtime_test PRIVATE arena_layout SFML::Graphics)
target_link_libraries(input_runtime_test PRIVATE bot_controller SFML::Graphics SFML::Audio)
target_link_libraries(effects_runtime_test PRIVATE particles)
target_link_libraries(audio_runtime_test PRIVATE audio_runtime)
target_link_libraries(projectile_runtime_test PRIVATE arena_layout particles procedural_synth SFML::Audio)
target_link_libraries(input_state_test PRIVATE bot_controller procedural_synth SFML::Audio SFML::Window)
target_link_libraries(frame_runtime_test PRIVATE SFML::Audio)
target_link_libraries(render_runtime_test PRIVATE render_runtime)
target_link_libraries(bot_controller PRIVATE procedural_synth)
target_link_libraries(render_runtime PUBLIC SFML::Graphics)
target_link_libraries(audio_runtime PUBLIC SFML::Audio)
target_link_libraries(procedural_synth   PUBLIC SFML::Audio)
if(WIN32)
    target_link_libraries(megablast_pvp_sfml PRIVATE Psapi)
endif()
target_include_directories(megablast_pvp_sfml PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_include_directories(procedural_synth PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(arena_layout PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(particles PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(settings_io PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(audio_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(bot_controller PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(render_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_test(NAME cli_help COMMAND megablast_pvp_sfml --help)
add_test(NAME cli_invalid_flag COMMAND megablast_pvp_sfml --bad-flag)
set_tests_properties(cli_invalid_flag PROPERTIES WILL_FAIL TRUE)
add_test(NAME cli_missing_perf_value COMMAND megablast_pvp_sfml --perf)
set_tests_properties(cli_missing_perf_value PROPERTIES WILL_FAIL TRUE)
add_test(NAME smoke_test COMMAND megablast_pvp_sfml --smoke-test --assets-dir ${CMAKE_SOURCE_DIR}/assets)

# Convenience run target (launches from project root so assets/ is found)
add_custom_target(run
    COMMAND $<TARGET_FILE:megablast_pvp_sfml>
    DEPENDS megablast_pvp_sfml
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Megachlast PvP..."
)

# Portable install/package layout: executable and assets share one directory
# so the runtime's default relative assets path works on every platform.
include(GNUInstallDirs)
install(TARGETS megablast_pvp_sfml
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .)
install(DIRECTORY assets/ DESTINATION assets
        PATTERN "settings.cfg" EXCLUDE)
install(FILES README.md LICENSE EARLY_ACCESS.md CHANGELOG.md KNOWN_ISSUES.md
        DESTINATION .)

set(CPACK_PACKAGE_NAME "megachlast_pvp")
set(CPACK_PACKAGE_VENDOR "Megachlast PvP")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Megachlast PvP one-screen duel shooter")
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)
