// Rendering primitives evolved from the original ShapeDrawer.
#include "ShapeDrawer.h"
#include "Curves.h"
#include "Line.h"
#include "Station.h"
#include <cmath>
void drawRectangle(sf::RenderTarget &target, LineColor color, sf::Vector2f a, sf::Vector2f b,
                   sf::Vector2f c, sf::Vector2f d) {
    sf::ConvexShape shape;
    shape.setPointCount(4);
    shape.setPoint(0, a);
    shape.setPoint(1, b);
    shape.setPoint(2, c);
    shape.setPoint(3, d);
    shape.setFillColor(getSFMLColor(color));
    target.draw(shape);
}
void stroke(sf::RenderTarget &target, const std::vector<sf::Vector2f> &points, sf::Color color,
            float width) {
    if (points.size() < 2)
        return;
    for (std::size_t i = 1; i < points.size(); ++i) {
        auto a = points[i - 1], b = points[i];
        auto n = getNormal(a, b) * (width / 2);
        if (length(b - a) < .001f)
            continue;
        sf::ConvexShape shape;
        shape.setPointCount(4);
        shape.setPoint(0, a - n);
        shape.setPoint(1, a + n);
        shape.setPoint(2, b + n);
        shape.setPoint(3, b - n);
        shape.setFillColor(color);
        target.draw(shape);
    }
    for (auto p : points) {
        sf::CircleShape cap(width / 2, 16);
        cap.setOrigin({width / 2, width / 2});
        cap.setPosition(p);
        cap.setFillColor(color);
        target.draw(cap);
    }
}
void drawPath(sf::RenderTarget &target, LineColor color, const std::vector<sf::Vector2f> &points) {
    stroke(target, points, getSFMLColor(color), 7);
}
void drawPath(sf::RenderTarget &target, LineColor color,
              std::vector<sf::Vector2f>::const_iterator begin,
              std::vector<sf::Vector2f>::const_iterator end) {
    drawPath(target, color, std::vector<sf::Vector2f>(begin, end));
}
void drawHandle(sf::RenderTarget &target, LineColor color, sf::Vector2f a, sf::Vector2f b) {
    auto v = b - a;
    float len = length(v);
    if (len < .001f)
        return;
    auto p = b + v / len * 38.f;
    auto n = getNormal(a, b) * 11.f;
    stroke(target, {b, p}, getSFMLColor(color), 4);
    stroke(target, {p - n, p + n}, getSFMLColor(color), 4);
}
sf::ConvexShape createConvexShape(Shape type, float radius) {
    sf::ConvexShape shape;
    switch (type) {
    case Shape::TRIANGLE:
        shape.setPointCount(3);
        shape.setPoint(0, {0, -radius * 1.25f});
        shape.setPoint(1, {radius * 1.1f, radius * .75f});
        shape.setPoint(2, {-radius * 1.1f, radius * .75f});
        break;
    case Shape::SQUARE:
        shape.setPointCount(4);
        shape.setPoint(0, {-radius, -radius});
        shape.setPoint(1, {radius, -radius});
        shape.setPoint(2, {radius, radius});
        shape.setPoint(3, {-radius, radius});
        break;
    case Shape::DIAMOND:
        shape.setPointCount(4);
        shape.setPoint(0, {0, -radius * 1.3f});
        shape.setPoint(1, {radius, 0});
        shape.setPoint(2, {0, radius * 1.3f});
        shape.setPoint(3, {-radius, 0});
        break;
    case Shape::STAR:
        shape.setPointCount(10);
        for (int i = 0; i < 10; ++i) {
            float angle = -1.5707963f + static_cast<float>(i) * .6283185f;
            float r = radius * (i % 2 == 0 ? 1.3f : .56f);
            shape.setPoint(i, {r * std::cos(angle), r * std::sin(angle)});
        }
        break;
    case Shape::CIRCLE:
        shape.setPointCount(48);
        for (int i = 0; i < 48; ++i) {
            float a = 6.2831853f * static_cast<float>(i) / 48.f;
            shape.setPoint(i, {radius * std::cos(a), radius * std::sin(a)});
        }
        break;
    }
    shape.setFillColor(sf::Color(249, 248, 241));
    shape.setOutlineColor(sf::Color(38, 59, 53));
    shape.setOutlineThickness(2);
    return shape;
}
void symbol(sf::RenderTarget &target, Shape shape, sf::Vector2f p, float radius, sf::Color fill,
            sf::Color outline, float thickness) {
    auto s = createConvexShape(shape, radius);
    s.setPosition(p);
    s.setFillColor(fill);
    s.setOutlineColor(outline);
    s.setOutlineThickness(thickness);
    target.draw(s);
}
void Station::draw(sf::RenderTarget &target) const {
    auto shape = createConvexShape(destination, radius);
    shape.setPosition({x, y});
    target.draw(shape);
}
void Line::draw(sf::RenderTarget &target) const {
    for (std::size_t e = 0; e < edgeCount(); ++e) {
        auto [a, b] = edge(e);
        drawPath(target, lineColor, getCurve(a->getPosition(), b->getPosition()));
    }
}
