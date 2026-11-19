#pragma once
#include "Shape.h"
#include <SFML/System/Vector2.hpp>
#include <random>
class RandomShapeGenerator {
  public:
    explicit RandomShapeGenerator(unsigned seed) : generator(seed) {}
    Shape shape(int count);
    sf::Vector2f position();
    int integer(int lo, int hi);
    float real(float lo, float hi);

  private:
    std::mt19937 generator;
};
