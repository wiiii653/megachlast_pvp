# Megachlast PvP (one-screen duel)

Two-player duel shooter on a single shared screen.
Players face each other: P2 top, P1 bottom. Horizontal movement only. Mirror blocks reflect and redirect bullets.

## Controls

Player 1 (bottom):

- A / D: move left/right
- Left Ctrl or Z: fire

Player 2 (top):

- Left / Right arrows: move left/right
- Right Ctrl or /: fire

Controllers:

- Each player can use a configured controller alongside their keyboard bindings.
- PS4, PS5, and Xbox controllers are supported through their connected SFML joystick slots.
- Left stick or D-pad: move left/right; south button, L1, or R1: fire.
- In menus and Settings: D-pad/left stick navigates, south confirms, east goes back; north opens Settings and west opens Donate from the main menu.
- During play: Start pauses/resumes, Select resets the round, and east returns to the menu. South confirms a rematch after Game Over.
- In Settings, select `P1 Controller` or `P2 Controller` and use Left/Right to choose `OFF` or joystick slot `Joy 1` through `Joy 8` (the first two slots are the defaults).

General:

- Space: pause/resume
- R: reset scores
- O: open settings screen
- F11: toggle fullscreen
- Escape: open menu from game screens (quit only when already in menu)

Before each match, choose one 12-second round modifier: Shield, Rapid Fire, Spread Shot, or Overdrive. Choose a curated arena (Mirror Maze, Fortress, or Open Reactor) with Q/E. In match setup, P1 uses A/D, P2 uses Left/Right, and Enter starts the match. Escape returns to the menu. Controllers use the stick or D-pad to choose modifiers, P1's L1/R1 to choose the arena, and either controller's south button to start.

## Rules

- Each player has ENERGY (0..100).
- Getting hit reduces ENERGY by 10 per bullet (friendly fire enabled).
- When ENERGY hits 0: the shooter scores +1 and victim respawns with a brief invulnerability window.
- Mirror blocks reflect bullets and rotate on hit.
- Being hit also applies a brief SLOWED debuff (half movement speed for up to 2 s, stacks per hit).
- First to 5 frags wins a round; the first player to win two rounds wins the match (configurable via `TARGET_SCORE` and `ROUNDS_TO_WIN` in `assets/settings.cfg`).
- After a match, Enter starts a rematch with the same arena and modifiers.

## Build (SFML 3)

This project requires **SFML 3.x**.

- SFML is an external dependency and is never stored in this repository.
- CMake uses an installed SFML 3 package, or automatically downloads pinned SFML 3.0.1. Use `-DSFML_PROVIDER=FETCH` to force the download or `SYSTEM` to require an installed package.
- Note: Ubuntu 24.04 default repositories provide SFML 2.6 (`libsfml-dev`), which is not sufficient.
- Cross Platform CI builds static SFML from the same upstream revision on all three platforms.

```bash
cmake -S . -B build
cmake --build build -j
```

For a basic install layout (without runtime dependency bundling), use:

```bash
cmake --install build --prefix release/megachlast_pvp
cpack --config build/CPackConfig.cmake
```

On Windows, use `--config Release` with `cmake --install` and `-C Release` with `cpack`.

## GitHub Releases

Push a version tag such as `v0.1.0` to run the multiplatform release workflow.
It builds and smoke-checks Linux, macOS, and Windows, then attaches the
platform archives to a GitHub Release automatically. The repository's GitHub
Actions workflow requires write access to repository contents for this step.
Tags with a suffix, such as `v0.1.0-rc.3`, produce prereleases.

- Linux x86_64 (Ubuntu 24.04+): extract the `.tar.gz` and run `play.sh`.
- Windows x64: extract the `.zip` and run `play.bat`.
- macOS Apple Silicon: open the `.dmg`, copy the game folder to a writable
  location, eject the image, and run `play.command`. Builds are not notarized.

Release archives include assets, runtime dependencies and license notices;
`SHA256SUMS.txt` provides checksums. Packaging uses `scripts/package_release.py`
with a `FETCH` build, not the basic CPack layout above.

### Linux

Install/provide SFML 3 through a package manager such as vcpkg. If using vcpkg:

```bash
vcpkg install sfml
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
ctest --test-dir build --output-on-failure
```

If SFML is installed in a standard system prefix, the default configure command also works:

```bash
cmake -S . -B build
```

### macOS

The default configure command downloads SFML when needed. Alternatively, use vcpkg:

```bash
vcpkg install sfml
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Windows 10/11 (MSVC)

Use a VS Developer PowerShell; CMake can download SFML automatically. Alternatively, with vcpkg:

```powershell
vcpkg install sfml:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --parallel
ctest --test-dir build --build-config Release --output-on-failure
```

The executable links `Psapi` on Windows for perf logging support.

## Run

```bash
# From project root (assets/ must be in CWD):
./build/megablast_pvp

# Or via CMake:
cmake --build build --target run
```

## Early Access

Early Access release process and quality gate checklist are documented in [EARLY_ACCESS.md](EARLY_ACCESS.md).
Runtime overlay includes a build label in the form `EA-0.1.0+<git-hash>` for bug report traceability.
Release notes and active issue tracking are in [CHANGELOG.md](CHANGELOG.md) and [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

## Settings & CLI

- `--no-music` — start without playing background music.
- `--no-postfx` — disable post-processing and cosmetic overlays (copper bars, scanlines, edge vignettes, chromatic aberration, final vignette) for lowest latency.
- `--assets-dir PATH` — override the default `assets/` directory when running.
- `--bot` — enable AI opponent (controls Player 2).
- `--bot-difficulty <easy|medium|hard>` — choose AI difficulty (default: medium).
- `--perf <high|medium|low|ultra>` — select performance profile.
- Runtime note: during heavy `PLAYING` scenes, effects now auto-scale down and recover to keep frame pacing smooth.
- Runtime note: key gameplay SFX (hits/explosions) briefly duck active music streams for cue clarity.
- `--log-perf PATH` — write periodic perf CSV.
- `--log-interval N` — perf log interval in seconds.
- `--log-duration N` — auto-exit after N seconds (useful with perf logging).
- `--gl-info` — print graphics environment and active OpenGL context diagnostics at startup.
- `--smoke-test` — verify required assets and initial game state without opening a window.
- `--gl-profile <default|clean|nvidia|dri3-off|software>` — choose a graphics loader profile before SFML creates the window.

Perf CSV columns: `time,perf,fx_level,fps,alive_particles,alive_bullets,spawn_budget,spawn_used,vm_rss_kb`.

Linux graphics fallback profiles:

- `default` — do not alter the user's graphics environment.
- `clean` — clear common GL loader overrides so the system GLVND/Mesa setup chooses the vendor.
- `nvidia` — clear overrides and force NVIDIA's PRIME/GLX path via `__NV_PRIME_RENDER_OFFLOAD=1` and `__GLX_VENDOR_LIBRARY_NAME=nvidia`.
- `dri3-off` — clear overrides and disable DRI3 via `LIBGL_DRI3_DISABLE=1`, useful for some NVIDIA/XWayland failures.
- `software` — clear overrides and force Mesa software rendering via `LIBGL_ALWAYS_SOFTWARE=1` for diagnostics or last-resort launch.

If launch logs mention `failed to load driver: nvidia-drm`, try:

```bash
./build/megablast_pvp --gl-info --gl-profile clean
./build/megablast_pvp --gl-info --gl-profile nvidia
./build/megablast_pvp --gl-info --gl-profile dri3-off
./build/megablast_pvp --gl-info --gl-profile software
```

Keyboard shortcuts:

- `B` — toggle bot on/off
- `V` — cycle bot difficulty (EASY → MED → HARD)
- `G` — toggle bot debug logging
- `P` — cycle performance profile (HIGH → MED → LOW → ULTRA)
- `M` — mute/unmute audio
- `,` / `.` — decrease/increase volume (applies to music and sfx)
- `K` — save current settings to `assets/settings.cfg`
- `L` — load settings from `assets/settings.cfg` (in settings screen)

Settings screen:

- `Up` / `Down` — select a row.
- `Left` / `Right` — change numeric settings, including windowed resolution scale.
- `Enter` — toggle boolean settings or activate save/load rows.
- Graphics rows control windowed resolution scale, 16:10/16:9 canvas aspect ratio, post-processing master toggle, scanlines, edge/final vignette, chromatic aberration, and copper bars.
- `WINDOW_SCALE` is a scale multiplier for the internal render target. `SCREEN_ASPECT=16:10` uses 640x400 (for example, scale `3` is 1920x1200); `SCREEN_ASPECT=16:9` uses a true 640x360 arena (scale `3` is 1920x1080). Aspect-ratio changes apply after restarting the game. Fullscreen still uses the desktop video mode.
- `--no-postfx` overrides saved graphics settings and disables cosmetic post-processing for the session.

Optional runtime overrides can be placed in `assets/settings.cfg` (copy and edit `assets/settings.cfg.example`).
Persistent toggles include `BOT_ENABLED`, `BOT_DIFFICULTY`, `MUSIC_VOLUME`, `SFX_VOLUME`, `MUTE`, controller slots, and graphics options.
Supported gameplay/audio/graphics keys include: `TARGET_SCORE`, `P_SPEED`, `BULLET_SPEED`, `BULLET_TTL`, `HIT_R`, `DAMAGE`, `FIRE_CD_P1_FRAMES`, `FIRE_CD_P2_FRAMES`, `BOT_ENABLED`, `BOT_DIFFICULTY`, `MUSIC_VOLUME`, `SFX_VOLUME`, `MUTE`, `P1_CONTROLLER`, `P2_CONTROLLER`, `WINDOW_SCALE`, `SCREEN_ASPECT`, `POSTFX_ENABLED`, `SCANLINES`, `VIGNETTE`, `CHROMATIC_ABERRATION`, `COPPER_BARS`.
Supported bot tuning overrides include: `BOT_DODGE_ZONE`, `BOT_DODGE_X_THR`, `BOT_ALIGN_TOL`, `BOT_FIRE_PROB`, `BOT_REACTION`, `BOT_POWERUP_INTEREST`, `BOT_STRAFE_LO`, `BOT_STRAFE_HI`, `BOT_BOMB_FEAR`, `BOT_AIM_LEAD_EASY`, `BOT_AIM_LEAD_MED`, `BOT_AIM_LEAD_HARD`.

## Assets

- `assets/sansation.ttf` — font
- `assets/press_start_2p.ttf` — Press Start 2P title font (SIL OFL 1.1)
- `assets/press_start_2p.OFL.txt` — Press Start 2P license
- `assets/menu.mp3` — menu music
- `assets/ingame.mp3` — in-game music
- `assets/get_ready.mp3` — round-start jingle
- `assets/pl1blu.png` — Player 1 ship sprite
- `assets/pl2red.png` — Player 2 ship sprite

All sound effects (fire, hit, explosions, power-ups) are **procedurally generated** at runtime via `ProceduralSynth` — no `.wav` files needed.

## Power-ups & Traits (shootable / collectible)

Megachlast PvP features shootable floating power-up orbs that grant short-term "traits" to the collecting player (the player whose bullet hits the orb). Power-ups are spawned periodically in the arena and can be destroyed/collected by either player (friendly fire is enabled). Below is the full list of shootable power-ups and what they do.

Important gameplay parameters (from the code):

- Max simultaneous power-ups: 4
- Spawn interval: ~7 seconds (first spawn is slightly sooner)
- Orb lifetime (time-to-live): 12 seconds (then the orb vanishes)
- Collection method: a bullet that overlaps the orb gives the effect to the bullet's owner; the orb and the bullet are both removed on collection
- Visuals: each power-up type has a distinct neon color and a spinning hex icon; active traits are shown on the HUD as small colored dots

Power-up types:

- SHIELD

  - Effect: grants the collector 5 seconds of shield (invulnerability to damage and bomb blast while shielded).
  - Duration: 5.0 s
  - Color: ice blue
  - Notes: shields also prevent damage from bombs (checked in blast logic).

- RAPID

  - Effect: halves the player's fire cooldown so they can shoot faster.
  - Duration: 5.0 s
  - Color: gold
  - Notes: the game's fire cooldown frames are divided by two while the trait is active (minimum 1 frame enforced).

- SPREAD

  - Effect: enables a 3-way spread shot (center + angled left/right) for the player when firing.
  - Duration: 5.0 s
  - Color: lime (green)
  - Notes: spread uses fixed ±18° angles for the side bullets.

- HEAL

  - Effect: instantly restores 40 ENERGY to the collector (clamped to maximum 100).
  - Duration: instant (no timed trait).
  - Color: mint
  - Notes: useful to recover from damage without waiting for respawn.

- CHAOS

  - Effect: scrambles all mirror block orientations in the arena (each mirror becomes randomly / or \).
  - Duration: instant (global effect).
  - Color: purple
  - Notes: a tactical power that can disrupt opponent aiming and existing mirror-based redirects.

- REVERSE

  - Effect: applies a 5 second debuff to the opponent that reverses their horizontal steering input (left/right are flipped).
  - Duration: 5.0 s (applied to opponent)
  - Color: orange
  - Notes: collected by shooting an orb; the effect is applied to the opponent (debuff).

HUD & feedback

- When a timed trait (SHIELD, RAPID, SPREAD, REVERSE) is active it is shown as a small colored "trait dot" on the player's HUD row. The dot disappears when the timer runs out.
- Power-up spawn and collect SFX are played (POWERUP_SPAWN and POWERUP_COLLECT). Collecting spawns a small particle burst.

Design notes

- Because power-ups are collected by bullets, mirrors and bouncing bullets can result in surprising collections (including accidental self-collection). Friendly fire and mirror reflections are part of the intended interaction.
- CHAOS is the only global (non-timed) type that immediately mutates arena state. REVERSE is a strategic debuff that targets the opponent rather than the collector.

## Arena Hazards

### Bombs

Each round may spawn 0–3 **bombs** depending on the generated layout (some layouts are intentionally more hazardous than others). Shoot a bomb to detonate it. The `CHAOS` power-up may also add extra bombs during a round.

- Blast radius: ~52 px — destroys all mirrors within range and deals damage to any player caught in the blast.
- Bombs do not trigger on bullet pass-through; only a direct bullet hit detonates them.
- Shielded players are immune to bomb blast damage.
- A bomb hit also applies the SLOWED debuff to the victim.

### Special Stars

Up to 3 **special stars** drift slowly around the arena each round. Shoot one to collect it.

- Awards **+50 bonus points** to the shooter each time.
- Stars respawn each round.

## Barriers

Each player starts with a **destructible barrier** of bricks placed just in front of their spawn row (40 bricks per side, spanning the full arena width). Bricks have 3 HP each and are damaged by bullets from either player. Destroyed bricks are removed permanently for that round. Barriers reset on each new round.
