#pragma once
#include "Shape.h"
class Passenger {
  public:
    explicit Passenger(Shape destination);
    Shape getDestination() const;

  private:
    Shape destination;
};
