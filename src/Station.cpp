#include "Station.h"
#include <cmath>
Station::Station(Shape destination, float x, float y) : destination(destination), x(x), y(y) {}
float Station::distance(const Station &station) const {
    return std::hypot(x - station.x, y - station.y);
}
float Station::getX() const {
    return x;
}
float Station::getY() const {
    return y;
}
sf::Vector2f Station::getPosition() const {
    return {x, y};
}
bool Station::contains(float px, float py) const {
    return std::hypot(px - x, py - y) <= radius + 7.f;
}
