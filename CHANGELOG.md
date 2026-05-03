# Changelog

## EA-0.1.0

- Added cross-platform CMake compatibility controls with `USE_BUNDLED_SFML`.
- Added Linux/macOS/Windows CI matrix with compiler coverage and sanitizer job.
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
