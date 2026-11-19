#pragma once
#include "LineColor.h"
#include "Station.h"
#include <utility>
#include <vector>
class Line {
  public:
    explicit Line(LineColor lineColor);
    LineColor getLineColor() const;
    bool isEmpty() const;
    bool active() const { return stations.size() >= 2; }
    bool isLoop() const { return loop; }
    const std::vector<Station *> &getStations() const { return stations; }
    std::size_t edgeCount() const;
    std::pair<Station *, Station *> edge(std::size_t i) const;
    int indexOf(int stationId) const;
    int revision() const { return version; }
    void draw(sf::RenderTarget &target) const;

  private:
    friend class LineHandler;
    LineColor lineColor;
    std::vector<Station *> stations;
    bool loop = false;
    int version = 0;
};
