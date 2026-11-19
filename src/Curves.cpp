#include "Curves.h"
#include <algorithm>
#include <cmath>
float length(sf::Vector2f v) {
    return std::hypot(v.x, v.y);
}
float crossProduct(sf::Vector2f a, sf::Vector2f b) {
    return a.x * b.y - a.y * b.x;
}
sf::Vector2f getNormal(sf::Vector2f a, sf::Vector2f b) {
    auto d = b - a;
    float n = length(d);
    return n < .0001f ? sf::Vector2f{} : sf::Vector2f{-d.y / n, d.x / n};
}
sf::Vector2f getNormal(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c) {
    auto n = getNormal(a, b) + getNormal(b, c);
    float len = length(n);
    return len < .0001f ? getNormal(a, b) : n / len;
}
std::vector<sf::Vector2f> getCurve(sf::Vector2f a, sf::Vector2f b) {
    // Canonical ordering makes A->B exactly the reverse of B->A.
    bool reversed = a.x > b.x || (a.x == b.x && a.y > b.y);
    if (reversed)
        std::swap(a, b);
    float dx = b.x - a.x, dy = b.y - a.y;
    float diagonal = std::min(std::abs(dx), std::abs(dy));
    sf::Vector2f elbow{a.x + diagonal, a.y + (dy >= 0 ? diagonal : -diagonal)};
    std::vector<sf::Vector2f> p{a};
    if (length(elbow - a) > .001f && length(elbow - b) > .001f)
        p.push_back(elbow);
    if (length(b - a) > .001f)
        p.push_back(b);
    if (reversed)
        std::reverse(p.begin(), p.end());
    return p;
}
void addCurve(std::vector<sf::Vector2f> &points, sf::Vector2f a, sf::Vector2f b) {
    auto p = getCurve(a, b);
    for (auto v : p)
        if (points.empty() || length(points.back() - v) > .001f)
            points.push_back(v);
}
float pathLength(const std::vector<sf::Vector2f> &p) {
    float total = 0;
    for (std::size_t i = 1; i < p.size(); ++i)
        total += length(p[i] - p[i - 1]);
    return total;
}
sf::Vector2f pointOnPath(const std::vector<sf::Vector2f> &p, float d) {
    if (p.empty())
        return {};
    d = std::max(0.f, d);
    for (std::size_t i = 1; i < p.size(); ++i) {
        float len = length(p[i] - p[i - 1]);
        if (d <= len && len > .0001f)
            return p[i - 1] + (p[i] - p[i - 1]) * (d / len);
        d -= len;
    }
    return p.back();
}
float distanceToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) {
    auto v = b - a;
    float n = v.x * v.x + v.y * v.y;
    if (n < .0001f)
        return length(p - a);
    float t = std::clamp(((p - a).x * v.x + (p - a).y * v.y) / n, 0.f, 1.f);
    return length(p - a - v * t);
}
float distanceToPath(sf::Vector2f p, const std::vector<sf::Vector2f> &path) {
    float result = 1e9f;
    for (std::size_t i = 1; i < path.size(); ++i)
        result = std::min(result, distanceToSegment(p, path[i - 1], path[i]));
    return result;
}
