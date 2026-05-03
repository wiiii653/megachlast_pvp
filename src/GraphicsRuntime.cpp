#include "GraphicsRuntime.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Context.hpp>

#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <stdlib.h>
#endif

namespace graphics_runtime {
namespace {

constexpr unsigned int GlVendor = 0x1F00;
constexpr unsigned int GlRenderer = 0x1F01;
constexpr unsigned int GlVersion = 0x1F02;

const char* envValue(const char* name)
{
    const char* value = std::getenv(name);
    return (value && value[0] != '\0') ? value : nullptr;
}

void setEnv(const char* name, const char* value)
{
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

void unsetEnv(const char* name)
{
#if defined(_WIN32)
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

void unsetGlLoaderOverrides()
{
    unsetEnv("MESA_LOADER_DRIVER_OVERRIDE");
    unsetEnv("LIBGL_DRIVERS_PATH");
    unsetEnv("__GLX_VENDOR_LIBRARY_NAME");
    unsetEnv("__NV_PRIME_RENDER_OFFLOAD");
    unsetEnv("LIBGL_DRI3_DISABLE");
    unsetEnv("LIBGL_ALWAYS_SOFTWARE");
}

void printEnv(std::FILE* out, const char* name)
{
    const char* value = envValue(name);
    std::fprintf(out, "  %s=%s\n", name, value ? value : "<unset>");
}

using GlGetStringFn = const unsigned char* (*)(unsigned int);

const char* glString(unsigned int name)
{
    auto fn = reinterpret_cast<GlGetStringFn>(sf::Context::getFunction("glGetString"));
    if(!fn) return nullptr;
    const unsigned char* value = fn(name);
    return reinterpret_cast<const char*>(value);
}

} // namespace

void applyGlProfile(graphics_profile::GlProfile profile, bool verbose)
{
    if(verbose)
        std::fprintf(stderr, "GL profile: %s\n", graphics_profile::glProfileName(profile));

    switch(profile){
    case graphics_profile::GlProfile::Default:
        break;
    case graphics_profile::GlProfile::Clean:
        unsetGlLoaderOverrides();
        break;
    case graphics_profile::GlProfile::Nvidia:
        unsetGlLoaderOverrides();
        setEnv("__NV_PRIME_RENDER_OFFLOAD", "1");
        setEnv("__GLX_VENDOR_LIBRARY_NAME", "nvidia");
        break;
    case graphics_profile::GlProfile::Dri3Off:
        unsetGlLoaderOverrides();
        setEnv("LIBGL_DRI3_DISABLE", "1");
        break;
    case graphics_profile::GlProfile::Software:
        unsetGlLoaderOverrides();
        setEnv("LIBGL_ALWAYS_SOFTWARE", "1");
        break;
    }
}

sf::ContextSettings makeWindowContextSettings()
{
    sf::ContextSettings settings;
    settings.depthBits = 0;
    settings.stencilBits = 0;
    settings.antiAliasingLevel = 0;
    settings.majorVersion = 2;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Default;
    settings.sRgbCapable = false;
    return settings;
}

void printStartupDiagnostics(std::FILE* out)
{
    std::fprintf(out, "Graphics startup diagnostics:\n");
#if defined(__linux__)
    printEnv(out, "XDG_SESSION_TYPE");
    printEnv(out, "WAYLAND_DISPLAY");
    printEnv(out, "DISPLAY");
#endif
    printEnv(out, "MESA_LOADER_DRIVER_OVERRIDE");
    printEnv(out, "__GLX_VENDOR_LIBRARY_NAME");
    printEnv(out, "__NV_PRIME_RENDER_OFFLOAD");
    printEnv(out, "LIBGL_DRIVERS_PATH");
    printEnv(out, "LIBGL_DRI3_DISABLE");
    printEnv(out, "LIBGL_ALWAYS_SOFTWARE");
    printEnv(out, "DRI_PRIME");

    const char* mesaOverride = envValue("MESA_LOADER_DRIVER_OVERRIDE");
    if(mesaOverride && std::strstr(mesaOverride, "nvidia-drm")){
        std::fprintf(out,
                     "  warning: MESA_LOADER_DRIVER_OVERRIDE=%s is usually wrong for GLX apps; try --gl-profile clean or nvidia.\n",
                     mesaOverride);
    }
}

void printWindowDiagnostics(std::FILE* out, const sf::RenderWindow& window)
{
    const sf::ContextSettings& settings = window.getSettings();
    std::fprintf(out, "Graphics window diagnostics:\n");
    std::fprintf(out, "  context=%u.%u depth=%u stencil=%u aa=%u srgb=%s\n",
                 settings.majorVersion,
                 settings.minorVersion,
                 settings.depthBits,
                 settings.stencilBits,
                 settings.antiAliasingLevel,
                 settings.sRgbCapable ? "yes" : "no");
    const char* vendor = glString(GlVendor);
    const char* renderer = glString(GlRenderer);
    const char* version = glString(GlVersion);
    std::fprintf(out, "  GL_VENDOR=%s\n", vendor ? vendor : "<unavailable>");
    std::fprintf(out, "  GL_RENDERER=%s\n", renderer ? renderer : "<unavailable>");
    std::fprintf(out, "  GL_VERSION=%s\n", version ? version : "<unavailable>");
    if(renderer && (std::strstr(renderer, "llvmpipe") || std::strstr(renderer, "softpipe"))){
        std::fprintf(out,
                     "  warning: software OpenGL renderer active; hardware acceleration is not being used.\n");
    }
}

} // namespace graphics_runtime
