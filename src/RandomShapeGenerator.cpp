#include "RandomShapeGenerator.h"
int RandomShapeGenerator::integer(int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(generator);
}
float RandomShapeGenerator::real(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(generator);
}
Shape RandomShapeGenerator::shape(int count) {
    if (count == 8)
        return Shape::DIAMOND;
    if (count == 14)
        return Shape::STAR;
    return static_cast<Shape>(integer(0, 2));
}
sf::Vector2f RandomShapeGenerator::position() {
    return {real(120, 1300), real(210, 690)};
}
