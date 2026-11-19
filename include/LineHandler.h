#pragma once
#include "Line.h"
#include "Resources.h"
#include "TrackGeometry.h"
#include "WorldGeometry.h"
#include <memory>
#include <optional>
#include <string>
struct SegmentHit {
    int line, edge;
};
class LineHandler {
  public:
    LineHandler();
    const std::vector<Line> &getLines() const { return lines; }
    const Line &at(int i) const { return lines.at(static_cast<std::size_t>(i)); }
    int selected() const { return currentLine; }
    void setCurrentLine(LineColor color);
    bool setRoute(int line, const std::vector<Station *> &route, bool loop);
    bool validate(int line, const std::vector<Station *> &route, bool loop,
                  std::string &reason) const;
    int tunnelUsage() const;
    int routeTunnels(const std::vector<Station *> &route, bool loop) const;
    std::vector<sf::Vector2f> edgePath(int line, int edge, bool offset = true) const;
    std::optional<SegmentHit> segmentAt(sf::Vector2f p, float tolerance = 12) const;
    const RouteGeometry &geometry(int line) const;
    std::array<sf::Vector2f, 3> handle(int line, bool start, float clearance = 0) const;
    void reset();
    World::MapId map = World::MapId::River;
    Resources resources;
    std::string message;

  private:
    std::array<TrackRoute, 8> routes(int replacement = -1, const std::vector<Station *> &route = {},
                                     bool loop = false) const;
    mutable std::unique_ptr<TrackGeometry> trackGeometry;
    std::vector<Line> lines;
    int currentLine = 0;
};
