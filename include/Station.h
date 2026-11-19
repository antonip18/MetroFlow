#pragma once
#include "Passenger.h"
#include <SFML/Graphics.hpp>
#include <deque>
class Station {
  public:
    Station(Shape destination, float x, float y);
    float distance(const Station &station) const;
    void draw(sf::RenderTarget &target) const;
    float getX() const;
    float getY() const;
    sf::Vector2f getPosition() const;
    Shape getShape() const { return destination; }
    bool contains(float px, float py) const;
    int getId() const { return id; }
    int capacity() const { return interchange ? 20 : 8; }
    std::deque<Passenger> passengers;
    float overcrowding = 0;
    bool interchange = false;

  private:
    friend class StationHandler;
    Shape destination;
    float x, y;
    float radius = 15;
    int id = -1;
};
sf::ConvexShape createConvexShape(Shape type, float radius);
