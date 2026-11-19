// Extended from the original LineSelector: limited palette and numbered slots.
#include "LineSelector.h"
#include <cmath>
float LineSelector::getFullLength() const {
    return 2 * radius * 8 + spacing * 7;
}
sf::Vector2f LineSelector::getCirclePosition(std::size_t i) const {
    return {774.f + static_cast<float>(i) * (2 * radius + spacing), Yposition};
}
void LineSelector::setSelectedColor(LineColor c) {
    selectedColor = c;
}
std::optional<LineColor> LineSelector::getColorAt(float x, float y) const {
    for (int i = 0; i < unlocked; ++i) {
        auto p = getCirclePosition(i);
        if (std::hypot(x - p.x, y - p.y) <= radius + 8)
            return getLineColor(i);
    }
    return std::nullopt;
}
void LineSelector::draw(sf::RenderTarget &target) const {
    for (int i = 0; i < 8; ++i) {
        sf::CircleShape c(radius);
        c.setOrigin({radius, radius});
        c.setPosition(getCirclePosition(i));
        c.setFillColor(i < unlocked ? getSFMLColor(getLineColor(i)) : sf::Color(218, 221, 215));
        if (getLineIndex(selectedColor) == i) {
            c.setOutlineColor(sf::Color(35, 58, 52));
            c.setOutlineThickness(3);
        }
        target.draw(c);
    }
}
