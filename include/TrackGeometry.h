#pragma once
#include <SFML/System/Vector2.hpp>
#include <array>
#include <vector>
namespace TrackStyle {
constexpr float Width = 7.f, LaneSpacing = 11.f;
constexpr float HandleLength = 28.f, HandleHalfWidth = Width * .55f;
constexpr float HandleThickness = Width;
constexpr float CarLength = 22.f, CarWidth = 12.f, CarSpacing = 26.f;
} // namespace TrackStyle
struct TrackRoute {
    std::vector<sf::Vector2f> stops;
    bool loop = false;
};
struct RouteGeometry {
    std::vector<std::vector<sf::Vector2f>> edges;
    std::vector<sf::Vector2f> path;
    std::vector<float> stopDistances;
    float totalLength = 0;
};
// One cached geometry for drawing, mouse picking and rolling stock.
class TrackGeometry {
  public:
    explicit TrackGeometry(const std::array<TrackRoute, 8> &routes);
    std::array<RouteGeometry, 8> lines;
    int maximumParallelLines = 0;
};
struct TrackPose {
    sf::Vector2f position{};
    sf::Vector2f tangent{1, 0};
};
TrackPose sampleTrack(const std::vector<sf::Vector2f> &path, float distance, bool loop);
