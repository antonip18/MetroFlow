#include "Curves.h"
#include "Simulation.h"
#include "TrainGeometry.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c))                                                                                  \
            throw std::runtime_error(std::string(__func__) + ":" + std::to_string(__LINE__) +      \
                                     " " #c);                                                      \
    } while (false)
namespace {
struct Fixture {
    StationHandler stations;
    LineHandler lines;
    Simulation sim{stations, lines};
    Fixture() {
        lines.reset();
        stations.clear();
        lines.resources.unlockedLines = 8;
        lines.resources.tunnels = 100;
        sim.spawning = false;
    }
    Station *add(sf::Vector2f p) {
        stations.addStation(Station(Shape::CIRCLE, p.x, p.y));
        return stations.byId(static_cast<int>(stations.getStations().size()) - 1);
    }
    void route(int i, std::initializer_list<int> ids, bool loop = false) {
        std::vector<Station *> r;
        for (int id : ids)
            r.push_back(stations.byId(id));
        CHECK(lines.setRoute(i, r, loop));
    }
};
void verticalLimit() {
    Fixture f;
    f.add({400, 250});
    for (int i = 0; i < 5; ++i)
        f.add({400, 700.f - 50.f * i});
    for (int i = 0; i < 4; ++i)
        f.route(i, {0, i + 1});
    for (int a = 0; a < 4; ++a)
        for (int b = a + 1; b < 4; ++b)
            CHECK(std::abs(f.lines.geometry(a).path.front().x -
                           f.lines.geometry(b).path.front().x) >= 10.9f);
    auto before = f.lines.at(4).revision();
    CHECK(!f.lines.setRoute(4, {f.stations.byId(0), f.stations.byId(5)}, false));
    CHECK(f.lines.at(4).revision() == before);
    CHECK(!f.lines.at(4).active());
}
void orientations() {
    for (auto d : std::vector<sf::Vector2f>{{1, 0}, {1, 1}, {1, -1}}) {
        std::array<TrackRoute, 8> routes;
        for (int i = 0; i < 4; ++i)
            routes[i].stops = {{200, 400}, sf::Vector2f{200, 400} + d * (250.f + 20.f * i)};
        TrackGeometry g(routes);
        CHECK(g.maximumParallelLines == 4);
        for (int a = 0; a < 4; ++a)
            for (int b = a + 1; b < 4; ++b)
                CHECK(distanceToPath(sampleTrack(g.lines[a].path, 100, false).position,
                                     g.lines[b].path) > 10.8f);
    }
}
void mirroredCorners() {
    for (int sx : {-1, 1})
        for (int sy : {-1, 1})
            for (bool swap : {false, true}) {
                std::array<TrackRoute, 8> routes;
                sf::Vector2f a{600, 450}, d{300.f * sx, 140.f * sy};
                if (swap)
                    std::swap(d.x, d.y);
                for (int i = 0; i < 4; ++i)
                    routes[i].stops = i % 2 ? std::vector<sf::Vector2f>{a + d, a}
                                            : std::vector<sf::Vector2f>{a, a + d};
                TrackGeometry g(routes);
                for (int i = 0; i < 4; ++i)
                    for (int j = i + 1; j < 4; ++j)
                        for (float x = 0; x < g.lines[i].totalLength; x += 5)
                            CHECK(distanceToPath(sampleTrack(g.lines[i].path, x, false).position,
                                                 g.lines[j].path) > TrackStyle::Width);
            }
}
void intermediateStops() {
    Fixture f;
    f.add({400, 250});
    f.add({400, 450});
    f.add({400, 650});
    f.route(0, {0, 1, 2});
    f.route(1, {2, 0});
    CHECK(length(f.lines.edgePath(0, 0).back() - f.lines.edgePath(0, 1).front()) < .01f);
    CHECK(std::abs(f.lines.geometry(0).path.front().x - f.lines.geometry(0).path.back().x) < .01f);
}
void pointContacts() {
    std::array<TrackRoute, 8> r;
    for (int i = 0; i < 8; ++i)
        r[i].stops = {{100.f + 50.f * i, 300}, {150.f + 50.f * i, 300}};
    TrackGeometry g(r);
    CHECK(g.maximumParallelLines == 1);
}
void intervalCount() {
    std::array<TrackRoute, 8> r;
    for (int i = 0; i < 7; ++i)
        r[i].stops = {{100.f + 80.f * i, 300}, {200.f + 80.f * i, 300}};
    TrackGeometry g(r);
    CHECK(g.maximumParallelLines == 2);
}
void nearParallel() {
    std::array<TrackRoute, 8> r;
    for (int i = 0; i < 4; ++i)
        r[i].stops = {{400.f + i, 250}, {400.f + i, 650}};
    TrackGeometry g(r);
    CHECK(g.maximumParallelLines == 4);
    for (int i = 1; i < 4; ++i)
        CHECK(length(g.lines[i].path.front() - g.lines[i - 1].path.front()) > 10.9f);
}
void centeredConsists() {
    Fixture f;
    f.add({400, 250});
    f.add({400, 450});
    f.add({400, 650});
    f.route(0, {0, 1, 2});
    for (int cars = 0; cars <= 3; ++cars)
        for (int stop = 0; stop < 3; ++stop)
            for (int dir : {-1, 1}) {
                Train t;
                t.line = 0;
                t.from = stop;
                t.direction = dir;
                t.carriages = cars;
                auto poses = trainPoses(t, f.lines);
                sf::Vector2f sum{};
                for (auto p : poses) {
                    sum += p.position;
                    CHECK(std::abs(p.tangent.x) < .001f);
                }
                CHECK(length(sum / static_cast<float>(poses.size()) -
                             f.stations.byId(stop)->getPosition()) < .01f);
            }
}
void bendPoses() {
    Fixture f;
    f.add({300, 500});
    f.add({600, 300});
    f.route(0, {0, 1});
    const auto &path = f.lines.geometry(0).path;
    CHECK(path.size() == 3);
    Train t;
    t.line = 0;
    t.from = 0;
    t.carriages = 3;
    t.distance = length(path[1] - path[0]);
    auto poses = trainPoses(t, f.lines);
    CHECK(length(poses.front().tangent - poses.back().tangent) > .5f);
    for (auto pose : poses)
        CHECK(distanceToPath(pose.position, path) < .01f);
}
void visibleLanes() {
    Fixture f;
    f.add({400, 250});
    f.add({400, 650});
    for (int i = 0; i < 4; ++i)
        f.route(i, {0, 1});
    for (int i = 0; i < 4; ++i) {
        Train t;
        t.line = i;
        t.from = 0;
        t.carriages = 3;
        auto center = visualTrainPosition(t, f.lines);
        CHECK(length(center - f.stations.byId(0)->getPosition()) < 20.f);
        for (auto pose : trainPoses(t, f.lines))
            CHECK(std::abs(pose.position.x - center.x) < .01f);
    }
}
void loopsAndHandles() {
    Fixture f;
    f.add({300, 300});
    f.add({500, 300});
    f.add({500, 500});
    f.route(0, {0, 1, 2}, true);
    Train t;
    t.line = 0;
    t.from = 0;
    t.carriages = 3;
    for (auto pose : trainPoses(t, f.lines))
        CHECK(distanceToPath(pose.position, f.lines.geometry(0).path) < .01f);
    f.route(0, {0, 1});
    f.route(1, {0, 1});
    f.sim.synchronize();
    auto a = f.lines.handle(0, true), b = f.lines.handle(1, true);
    CHECK(std::abs(length(a[2] - a[1]) - TrackStyle::Width * 1.1f) < .01f);
    CHECK(length((a[1] + a[2]) * .5f - (b[1] + b[2]) * .5f) > TrackStyle::LaneSpacing);
    f.lines.resources.carriages = 3;
    int id = f.sim.getTrains().front().id;
    for (int i = 0; i < 3; ++i)
        CHECK(f.sim.addCarriage(id));
    auto h = f.sim.endpointHandle(0, true);
    CHECK(length((h[1] + h[2]) * .5f - h[0]) > 50.f);
}
void bothMaps() {
    StationHandler stations;
    LineHandler lines;
    Simulation sim(stations, lines);
    for (auto map : {World::MapId::River, World::MapId::Canals}) {
        sim.reset(42, map);
        CHECK(lines.map == map);
        CHECK(stations.getStations().size() == 5);
        for (const auto &s : stations.getStations())
            CHECK(!World::water(s.getPosition(), 15, map));
        sim.update(22);
        CHECK(stations.getStations().size() > 5);
        for (const auto &s : stations.getStations())
            CHECK(!World::water(s.getPosition(), 15, map));
        sim.seedDemo();
        CHECK(lines.map == map);
        CHECK(sim.getTrains().size() == 3);
        CHECK(lines.tunnelUsage() <= lines.resources.tunnels);
        CHECK(sim.paused);
    }
    CHECK(World::crossings({{200, 200}, {200, 700}}, World::MapId::Canals) == 2);
}
} // namespace
int main() {
    const std::pair<const char *, std::function<void()>> tests[] = {
        {"vertical limit", verticalLimit},      {"horizontal and diagonal", orientations},
        {"mirrored corners", mirroredCorners},  {"intermediate stops", intermediateStops},
        {"point contacts", pointContacts},      {"simultaneous colours", intervalCount},
        {"near parallel", nearParallel},        {"centered consists", centeredConsists},
        {"individual car poses", bendPoses},    {"visible train lanes", visibleLanes},
        {"loops and handles", loopsAndHandles}, {"two maps", bothMaps}};
    try {
        for (const auto &[name, test] : tests) {
            test();
            std::cout << "PASS " << name << '\n';
        }
        std::cout << "12 track scenarios passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
