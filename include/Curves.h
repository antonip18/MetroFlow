#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
float length(sf::Vector2f v);
float crossProduct(sf::Vector2f a, sf::Vector2f b);
sf::Vector2f getNormal(sf::Vector2f a, sf::Vector2f b);
sf::Vector2f getNormal(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c);
std::vector<sf::Vector2f> getCurve(sf::Vector2f a, sf::Vector2f b);
void addCurve(std::vector<sf::Vector2f> &points, sf::Vector2f a, sf::Vector2f b);
float pathLength(const std::vector<sf::Vector2f> &points);
sf::Vector2f pointOnPath(const std::vector<sf::Vector2f> &points, float distance);
float distanceToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b);
float distanceToPath(sf::Vector2f p, const std::vector<sf::Vector2f> &points);
