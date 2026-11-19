#include "WorldGeometry.h"
#include "Curves.h"
#include <cmath>
namespace World {
float riverX(float y) {
    return 735.f + 72.f * std::sin((y - 100.f) / 155.f);
}
const char *mapName(MapId map) {
    return map == MapId::River ? "RIVER DISTRICT" : "CANAL ISLANDS";
}
float canalY(float x, int channel) {
    return channel == 0 ? 335.f + 30.f * std::sin(x / 185.f)
                        : 545.f + 30.f * std::sin((x + 75.f) / 210.f);
}
float riverWidth(MapId map) {
    return map == MapId::River ? 52.f : 42.f;
}
std::vector<std::vector<sf::Vector2f>> waterways(MapId map) {
    std::vector<std::vector<sf::Vector2f>> result(map == MapId::River ? 1 : 2);
    if (map == MapId::River)
        for (int y = 155; y <= 750; y += 4)
            result[0].push_back({riverX(static_cast<float>(y)), static_cast<float>(y)});
    else
        for (int channel = 0; channel < 2; ++channel)
            for (int x = 48; x <= 1390; x += 4)
                result[channel].push_back(
                    {static_cast<float>(x), canalY(static_cast<float>(x), channel)});
    return result;
}
std::vector<sf::Vector2f> initialStations(MapId map) {
    if (map == MapId::River)
        return {{320, 350}, {535, 250}, {580, 550}, {1010, 350}, {1130, 590}};
    return {{260, 225}, {980, 235}, {510, 435}, {1170, 445}, {930, 665}};
}
bool water(sf::Vector2f p, float margin, MapId map) {
    if (map == MapId::River)
        return std::abs(p.x - riverX(p.y)) < 26.f + margin;
    return std::abs(p.y - canalY(p.x, 0)) < 21.f + margin ||
           std::abs(p.y - canalY(p.x, 1)) < 21.f + margin;
}
bool playable(sf::Vector2f p) {
    return p.x >= 100 && p.x <= 1330 && p.y >= 190 && p.y <= 710;
}
int crossings(const std::vector<sf::Vector2f> &path, MapId map) {
    int count = 0;
    bool inside = false;
    // Count connected water intervals along the same geometry used by trains/rendering.
    for (std::size_t i = 1; i < path.size(); ++i) {
        int steps = std::max(1, static_cast<int>(std::ceil(length(path[i] - path[i - 1]) / 2.f)));
        for (int j = (i == 1 ? 0 : 1); j <= steps; ++j) {
            bool current =
                water(path[i - 1] + (path[i] - path[i - 1]) *
                                        (static_cast<float>(j) / static_cast<float>(steps)),
                      0, map);
            if (current && !inside)
                ++count;
            inside = current;
        }
    }
    return count;
}
} // namespace World
