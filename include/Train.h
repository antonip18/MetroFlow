#pragma once
#include "Passenger.h"
#include <SFML/System/Vector2.hpp>
#include <vector>
struct Train {
    int id = 0;
    int line = 0;
    int from = -1, to = -1;
    int direction = 1;
    float distance = 0;
    float dwell = 0;
    int carriages = 0;
    std::vector<Passenger> passengers;
    sf::Vector2f position{};
    int capacity() const { return 6 * (1 + carriages); }
};
