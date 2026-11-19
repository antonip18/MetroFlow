#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
namespace World {
enum class MapId { River, Canals };
const char *mapName(MapId map);
float canalY(float x, int channel);
float riverWidth(MapId map);
std::vector<std::vector<sf::Vector2f>> waterways(MapId map);
std::vector<sf::Vector2f> initialStations(MapId map);
constexpr float Width = 1440, Height = 900;
float riverX(float y);
bool water(sf::Vector2f p, float margin = 0, MapId map = MapId::River);
bool playable(sf::Vector2f p);
int crossings(const std::vector<sf::Vector2f> &path, MapId map = MapId::River);
} // namespace World
