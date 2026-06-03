# Known Issues

## Cross-platform

- First CI run may fail on a specific runner due to transient dependency fetch/build issues.
- CI uses vcpkg SFML 3 with bundled SFML disabled; local builds may still use bundled SFML when its runtime dependencies match the host.
- Audio output device handling can differ by OS and driver stack.

## Linux

- OpenGL driver stacks can vary significantly; `--gl-profile` fallbacks may be required on some systems.
- Wayland/XWayland combinations can show different fullscreen behavior depending on compositor.
- If bundled SFML audio depends on an unavailable FLAC runtime such as `libFLAC.so.12`, CMake falls back to system SFML when available.

## macOS

- vcpkg SFML 3 is the CI-tested build route. Other SFML installs may need `SFML_DIR` or `CMAKE_PREFIX_PATH` set explicitly.
- Depending on local SFML install path and SIP/runtime linker behavior, additional runtime path adjustments may be needed outside CI.

## Windows 10/11

- vcpkg SFML 3 with MSVC is the CI-tested build route.
- Runtime behavior depends on graphics and audio drivers; occasional startup/runtime differences may appear across machines.
- HTTPS git pushes require valid credential setup (PAT or SSH).

## Gameplay / Early Access

- Balance is still in active tuning.
- `assets/settings.cfg` format is not guaranteed stable between Early Access builds.
