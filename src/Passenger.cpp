//
// Created by Antoni on 25.09.2026.
//

#include "Passenger.h"

Passenger::Passenger(Shape destination) : destination(destination) {}

Shape Passenger::getDestination() const {
    return destination;
}