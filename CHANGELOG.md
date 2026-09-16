# Changelog

## Unreleased

- Fixed CI dependency setup: install FreeType on Linux and explicitly build shared SFML on Linux/macOS.
- Fixed SFML dependency propagation to library consumers and added a missing standard header exposed by MSVC.
- Made the secondary CI workflow build and test Release consistently, including multi-configuration Windows generators.
- Added configurable controller slots for both players and corrected ship asset names.
- Added match setup with Shield, Rapid Fire, Spread Shot, and Overdrive modifiers and three selectable arenas.
- Added rounds-to-win match scoring, knockout feedback, and rematches using the selected setup.
- Made match-setup keyboard controls consistent with gameplay: P1 uses A/D and P2 uses Left/Right.
- Added start/back instructions to the match-setup screen.
- Fixed bonus points carrying over into new matches, rematches, and manual score resets.
- Added regression coverage for match-setup keyboard input and rematch resets.

## EA-0.1.0

- Added cross-platform CMake compatibility controls with `USE_BUNDLED_SFML`.
- Added Linux/macOS/Windows CI matrix using vcpkg SFML 3.
- Added Linux fallback from bundled SFML to system SFML when bundled audio requires unavailable `libFLAC.so.12`.
- Added in-game graphics settings for windowed resolution scale, post FX, scanlines, vignette, chromatic aberration, and copper bars.
- Added Early Access release guide and release quality checklist.
- Added in-game build label with git hash (`EA-0.1.0+<hash>`).
- Updated menu logo shine effect to be text-masked and better aligned.
- Added object-specific procedural hit SFX:
  - player hit
  - mirror reflect
  - barrier hit
  - bomb hit
  - power-up hit
  - special star hit
- Balanced hit SFX mix so critical cues (player hit/explosion) remain dominant.
