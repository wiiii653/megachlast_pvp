# Known Issues

## Cross-platform

- CI downloads a pinned SFML 3.0.1 source revision and builds it statically on all platforms. SFML is not vendored in this repository.
- Audio output device handling can differ by OS and driver stack.

## Linux

- OpenGL driver stacks can vary significantly; `--gl-profile` fallbacks may be required on some systems.
- Wayland/XWayland combinations can show different fullscreen behavior depending on compositor.
- Release archives target Ubuntu 24.04 or newer on x86_64. Graphics drivers and glibc remain host dependencies.

## macOS

- Release archives target Apple Silicon; Intel Macs are not currently packaged.
- Builds are ad-hoc signed, not notarized. Gatekeeper may require explicit approval in Privacy & Security.

## Windows 10/11

- Release archives target x64 with static SFML and MSVC runtime libraries.
- Runtime behavior depends on graphics and audio drivers; occasional startup/runtime differences may appear across machines.
- HTTPS git pushes require valid credential setup (PAT or SSH).

## Gameplay / Early Access

- Balance is still in active tuning.
- `assets/settings.cfg` format is not guaranteed stable between Early Access builds.
