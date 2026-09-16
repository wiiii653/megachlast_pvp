# Early Access Release Guide

This document defines the minimum quality bar and release process for Early Access builds of Megachlast PvP.

## Release Channel

- Channel: Early Access
- Stability target: playable, non-destructive, recoverable from common errors
- Supported platforms: Linux, macOS, Windows 10/11

## Minimum Release Criteria

1. Build passes on all supported platforms via CI.
2. Test suite passes (`ctest`) on all supported platforms.
3. Game launches from project root and can start a round.
4. Core input works (move/fire/pause/menu/settings).
5. Audio path is functional (music + SFX, mute/volume controls).
6. Settings save/load works (`assets/settings.cfg`), including gameplay, audio, bot, and graphics settings.
7. No known crash-on-launch issues on supported platforms.

## Pre-Release Checklist

1. Run local sanity checks:
   - `cmake -S . -B build_sfml3`
   - `cmake --build build_sfml3 -j`
   - `ctest --test-dir build_sfml3 --output-on-failure`
2. Ensure cross-platform CI workflow is green. Cross Platform CI obtains SFML 3.0.1 externally from upstream on Linux/macOS and vcpkg on Windows. Check [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for outstanding failures.
3. Verify README setup/run commands are still accurate.
4. Verify assets required by runtime are present in `assets/`.
5. Confirm known issues list is up to date.

## Known Early Access Limitations

- Performance and visual consistency may vary by GPU driver stack.
- Gameplay tuning and balance are still in active iteration.
- Save format (`assets/settings.cfg`) is not guaranteed stable between Early Access builds.
- Some diagnostics and graphics profile options are primarily Linux-oriented.
- SFML is not vendored in the repository; builds require an external system, upstream, or vcpkg SFML 3 installation.

## Player-Facing Notes Template

Use this format for each Early Access release post:

1. Build identifier/date.
2. Platforms tested.
3. Main gameplay changes.
4. Fixes.
5. Known issues.
6. How to report bugs (steps, platform, logs, repro frequency).

## Rollback Rule

If a release introduces launch failures or repeatable gameplay-blocking bugs on any supported platform, roll back to the previous known-good Early Access build.
