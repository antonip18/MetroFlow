#pragma once
#include "LineColor.h"
#include "Shape.h"
#include <SFML/Graphics.hpp>
#include <vector>
void drawRectangle(sf::RenderTarget &, LineColor, sf::Vector2f, sf::Vector2f, sf::Vector2f,
                   sf::Vector2f);
void drawHandle(sf::RenderTarget &, LineColor, sf::Vector2f, sf::Vector2f);
void drawPath(sf::RenderTarget &, LineColor, const std::vector<sf::Vector2f> &);
void drawPath(sf::RenderTarget &, LineColor, std::vector<sf::Vector2f>::const_iterator,
              std::vector<sf::Vector2f>::const_iterator);
void stroke(sf::RenderTarget &, const std::vector<sf::Vector2f> &, sf::Color, float width);
void symbol(sf::RenderTarget &, Shape, sf::Vector2f, float, sf::Color, sf::Color,
            float outline = 1.5f);
