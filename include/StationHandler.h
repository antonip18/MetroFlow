#pragma once
#include "Station.h"
#include <deque>
class StationHandler {
  public:
    void addStation(Station station);
    const std::deque<Station> &getStations() const;
    std::deque<Station> &getStations() { return stations; }
    Station *getStationAt(float x, float y);
    Station *byId(int id);
    const Station *byId(int id) const;
    void clear() { stations.clear(); }

  private:
    // push_back preserves references used by Line; stations are never erased mid-game.
    std::deque<Station> stations;
};
