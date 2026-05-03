#pragma once

#include "GraphicsProfile.h"

#include <SFML/Window/ContextSettings.hpp>

#include <cstdio>

namespace sf { class RenderWindow; }

namespace graphics_runtime {

void applyGlProfile(graphics_profile::GlProfile profile, bool verbose);
sf::ContextSettings makeWindowContextSettings();
void printStartupDiagnostics(std::FILE* out);
void printWindowDiagnostics(std::FILE* out, const sf::RenderWindow& window);

} // namespace graphics_runtime
