#pragma once
#include "LineColor.h"
#include <SFML/Graphics.hpp>
#include <optional>
class LineSelector {
  public:
    float getFullLength() const;
    void draw(sf::RenderTarget &target) const;
    std::optional<LineColor> getColorAt(float x, float y) const;
    sf::Vector2f getCirclePosition(std::size_t index) const;
    void setSelectedColor(LineColor color);
    int unlocked = 3;

  private:
    LineColor selectedColor = LineColor::RED;
    float radius = 16, spacing = 22, Yposition = 832;
};
