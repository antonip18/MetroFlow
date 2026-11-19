#pragma once
#include <SFML/Graphics/Color.hpp>
enum class LineColor { RED, BLUE, GREEN, YELLOW, PURPLE, CYAN, ORANGE, DARK_RED };
sf::Color getSFMLColor(LineColor lineColor);
LineColor getLineColor(int lineNumber);
int getLineIndex(LineColor lineColor);
