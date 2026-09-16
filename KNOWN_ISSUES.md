# Known Issues

## Cross-platform

- Cross Platform CI for `fd91ea4` failed due to missing FreeType development files on Linux, a static/shared SFML mismatch on macOS, and missing propagated SFML include paths plus `<algorithm>` on Windows. See [run 29322261325](https://github.com/wiiii653/megachlast_pvp/actions/runs/29322261325). Fixes are included in the working tree; a successful run on all supported platforms is still required before release.
- Cross Platform CI obtains SFML 3.0.1 externally from upstream on Linux/macOS and vcpkg on Windows. The separate CI workflow uses vcpkg on all three platforms. SFML is not vendored in this repository.
- Audio output device handling can differ by OS and driver stack.

## Linux

- OpenGL driver stacks can vary significantly; `--gl-profile` fallbacks may be required on some systems.
- Wayland/XWayland combinations can show different fullscreen behavior depending on compositor.
- Linux builds require an external SFML installation whose audio dependencies, including FLAC support, are available on the host.

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
