//
// Created by Antoni on 25.09.2026.
//

#include "StationHandler.h"

void StationHandler::addStation(Station station) {
    station.id = static_cast<int>(stations.size());
    stations.push_back(std::move(station));
}

const std::deque<Station> &StationHandler::getStations() const {
    return stations;
}

Station *StationHandler::getStationAt(float x, float y) {
    for (auto &station : stations)
        if (station.contains(x, y))
            return &station;
    return nullptr;
}
Station *StationHandler::byId(int id) {
    return id >= 0 && id < static_cast<int>(stations.size())
               ? &stations[static_cast<std::size_t>(id)]
               : nullptr;
}
const Station *StationHandler::byId(int id) const {
    return id >= 0 && id < static_cast<int>(stations.size())
               ? &stations[static_cast<std::size_t>(id)]
               : nullptr;
}
