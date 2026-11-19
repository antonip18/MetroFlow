#include "Curves.h"
#include "Simulation.h"
#include "WorldGeometry.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition))                                                                          \
            throw std::runtime_error(std::string(__func__) + ":" + std::to_string(__LINE__) +      \
                                     " " #condition);                                              \
    } while (false)
namespace {
struct Fixture {
    StationHandler stations;
    LineHandler lines;
    Simulation sim{stations, lines};
    Fixture() {
        lines.reset();
        stations.clear();
        sim.spawning = false;
        stations.addStation(Station(Shape::CIRCLE, 180, 300));
        stations.addStation(Station(Shape::TRIANGLE, 300, 300));
        stations.addStation(Station(Shape::SQUARE, 420, 300));
        stations.addStation(Station(Shape::DIAMOND, 540, 500));
        stations.addStation(Station(Shape::STAR, 300, 600));
    }
    std::vector<Station *> route(std::initializer_list<int> ids) {
        std::vector<Station *> result;
        for (int id : ids)
            result.push_back(stations.byId(id));
        return result;
    }
    void line(int i, std::initializer_list<int> ids, bool loop = false) {
        CHECK(lines.setRoute(i, route(ids), loop));
        sim.synchronize();
    }
    void rider(int origin, Shape destination) {
        stations.byId(origin)->passengers.emplace_back(destination);
        ++sim.generated;
    }
    int population() const {
        int n = sim.score;
        for (const auto &s : stations.getStations())
            n += static_cast<int>(s.passengers.size());
        for (const auto &t : sim.getTrains())
            n += static_cast<int>(t.passengers.size());
        return n;
    }
    void advance(float seconds) {
        for (int i = 0; i < static_cast<int>(seconds * 60); ++i)
            sim.update(1.f / 60.f);
    }
};
void geometry() {
    std::vector<sf::Vector2f> points = {{0, 0},     {100, 0},   {0, 100},
                                        {100, 100}, {180, -90}, {-70, 140}};
    for (auto a : points)
        for (auto b : points) {
            auto path = getCurve(a, b), reverse = getCurve(b, a);
            std::reverse(reverse.begin(), reverse.end());
            CHECK(path == reverse);
            CHECK(path.front() == a);
            CHECK(path.back() == b);
            for (std::size_t i = 1; i < path.size(); ++i) {
                auto d = path[i] - path[i - 1];
                CHECK(std::abs(d.x) < .001f || std::abs(d.y) < .001f ||
                      std::abs(std::abs(d.x) - std::abs(d.y)) < .001f);
            }
            CHECK(std::isfinite(pathLength(path)));
            CHECK(length(pointOnPath(path, pathLength(path)) - b) < .01f);
        }
    CHECK(getNormal({0, 0}, {0, 0}) == sf::Vector2f{});
    CHECK(distanceToPath({20, 0}, {{0, 0}, {100, 0}}) < .001f);
    CHECK(World::crossings(getCurve({500, 400}, {1000, 400})) == 1);
    CHECK(World::crossings(getCurve({200, 400}, {500, 400})) == 0);
}
void stableStations() {
    Fixture f;
    auto *first = f.stations.byId(0);
    for (int i = 0; i < 2000; ++i)
        f.stations.addStation(Station(Shape::CIRCLE, 100, 200));
    CHECK(first == f.stations.byId(0));
    CHECK(first->getId() == 0);
}
void transactionalEdits() {
    Fixture f;
    f.line(0, {0, 1, 2});
    auto version = f.lines.at(0).revision();
    CHECK(!f.lines.setRoute(0, f.route({0, 1, 0}), false));
    CHECK(f.lines.at(0).revision() == version);
    CHECK(f.lines.at(0).getStations() == f.route({0, 1, 2}));
    CHECK(!f.lines.setRoute(0, f.route({0, 1}), true));
    CHECK(!f.lines.setRoute(7, f.route({0, 1}), false));
    f.line(0, {4, 0, 1, 2, 3});
    CHECK(f.lines.at(0).getStations().front()->getId() == 4);
    f.line(0, {4, 0, 3}, true);
    CHECK(f.lines.at(0).edgeCount() == 3);
    CHECK(f.lines.at(0).isLoop());
}
void tunnelBudget() {
    Fixture f;
    f.stations.addStation(Station(Shape::CIRCLE, 1100, 300));
    f.lines.resources.tunnels = 1;
    f.line(0, {0, 5});
    CHECK(f.lines.tunnelUsage() == 1);
    CHECK(!f.lines.setRoute(1, f.route({1, 5}), false));
    CHECK(f.lines.at(1).isEmpty());
    CHECK(f.lines.tunnelUsage() == 1);
    f.line(0, {});
    CHECK(f.lines.tunnelUsage() == 0);
    f.line(1, {1, 5});
    CHECK(f.lines.tunnelUsage() == 1);
}
void parallelLimit() {
    Fixture f;
    f.lines.resources.unlockedLines = 8;
    for (int i = 0; i < 4; ++i)
        CHECK(f.lines.setRoute(i, f.route({0, 1}), false));
    CHECK(!f.lines.setRoute(4, f.route({0, 1}), false));
    CHECK(f.lines.at(4).isEmpty());
    auto p = f.lines.edgePath(0, 0), q = f.lines.edgePath(3, 0);
    CHECK(length(p.front() - q.front()) > 20);
}
void routing() {
    Fixture f;
    f.line(0, {0, 1});
    f.line(1, {1, 2});
    auto &r = f.sim.getRouter();
    CHECK(r.board(0, 1, Shape::SQUARE, 0));
    CHECK(!r.board(1, 0, Shape::SQUARE, 0));
    CHECK(r.board(1, 2, Shape::SQUARE, 1));
    CHECK(!r.board(0, 1, Shape::STAR, 0));
    f.sim.reclaimTrain(f.sim.getTrains()[1].id);
    CHECK(!f.sim.getRouter().board(0, 1, Shape::SQUARE, 0));
}
void directDelivery() {
    Fixture f;
    f.line(0, {0, 1, 2});
    for (int i = 0; i < 6; ++i)
        f.rider(0, Shape::SQUARE);
    f.advance(12);
    CHECK(f.sim.score == 6);
    CHECK(f.population() == 6);
}
void transferDelivery() {
    Fixture f;
    f.line(0, {0, 1});
    f.line(1, {1, 2});
    for (int i = 0; i < 5; ++i)
        f.rider(0, Shape::SQUARE);
    f.advance(20);
    CHECK(f.sim.score == 5);
    CHECK(f.population() == 5);
}
void directedLoop() {
    Fixture f;
    f.line(0, {0, 1, 2}, true);
    CHECK(f.sim.getRouter().board(2, 0, Shape::CIRCLE, 0));
    CHECK(!f.sim.getRouter().board(2, 1, Shape::TRIANGLE, 0));
    f.rider(2, Shape::TRIANGLE);
    f.advance(20);
    CHECK(f.sim.score == 1);
}
void noWrongDestination() {
    Fixture f;
    f.line(0, {0, 1});
    f.rider(0, Shape::STAR);
    f.advance(20);
    CHECK(f.sim.score == 0);
    CHECK(f.stations.byId(0)->passengers.size() == 1);
    CHECK(f.population() == 1);
}
void resourceConservation() {
    Fixture f;
    f.line(0, {0, 1, 2});
    CHECK(f.sim.freeEngines() == 2);
    int id = f.sim.getTrains().front().id;
    CHECK(f.sim.addCarriage(id));
    CHECK(f.sim.freeCarriages() == 0);
    CHECK(!f.sim.addCarriage(id));
    for (int i = 0; i < 12; ++i)
        f.rider(0, Shape::SQUARE);
    f.advance(1);
    CHECK(f.sim.getTrains().front().passengers.size() == 12);
    CHECK(f.sim.reclaimCarriage(id));
    CHECK(f.sim.getTrains().front().passengers.size() == 6);
    CHECK(f.population() == 12);
    CHECK(f.sim.reclaimTrain(id));
    CHECK(f.sim.freeEngines() == 3);
    CHECK(f.sim.freeCarriages() == 1);
    CHECK(f.population() == 12);
    CHECK(f.sim.score == 0);
}
void editWithRiders() {
    Fixture f;
    f.line(0, {0, 1, 2});
    f.rider(0, Shape::SQUARE);
    f.advance(1);
    CHECK(f.sim.getTrains()[0].passengers.size() == 1);
    f.line(0, {3, 4, 2});
    CHECK(f.population() == 1);
    CHECK(f.sim.score == 0);
    f.advance(20);
    CHECK(f.sim.score == 1);
}
void removeAndReassign() {
    Fixture f;
    f.line(0, {0, 1});
    f.line(1, {1, 2});
    int id = f.sim.getTrains()[0].id;
    f.rider(0, Shape::SQUARE);
    f.advance(1);
    int before = f.population();
    CHECK(f.sim.moveTrain(id, 1));
    CHECK(f.population() == before);
    CHECK(f.sim.score == 0);
    CHECK(f.sim.freeEngines() == 1);
    f.line(1, {});
    CHECK(f.sim.getTrains().empty());
    CHECK(f.sim.freeEngines() == 3);
    CHECK(f.population() == before);
    f.sim.synchronize();
    CHECK(f.sim.getTrains().empty()); // moving does not silently respawn engines
}
void pauseAndWeek() {
    Fixture f;
    f.sim.paused = true;
    f.sim.update(20);
    CHECK(f.sim.elapsed == 0);
    f.sim.paused = false;
    f.sim.update(91);
    CHECK(f.sim.week == 2);
    CHECK(f.sim.choosingUpgrade);
    CHECK(f.lines.resources.engines == 4);
    float t = f.sim.elapsed;
    f.sim.update(100);
    CHECK(f.sim.elapsed == t);
    CHECK(f.lines.resources.engines == 4);
    CHECK(f.sim.chooseUpgrade(Upgrade::Tunnels));
    CHECK(f.lines.resources.tunnels == 5);
    CHECK(!f.sim.chooseUpgrade(Upgrade::Line));
    f.sim.update(1);
    CHECK(f.sim.elapsed > t);
}
void congestionAndHub() {
    Fixture f;
    for (int i = 0; i < 9; ++i)
        f.rider(0, Shape::SQUARE);
    f.advance(15);
    CHECK(!f.sim.gameOver);
    CHECK(f.stations.byId(0)->overcrowding > 14);
    CHECK(f.sim.upgradeStation(0));
    CHECK(f.stations.byId(0)->capacity() == 20);
    CHECK(!f.sim.upgradeStation(1));
    f.advance(10);
    CHECK(f.stations.byId(0)->overcrowding == 0);
    for (int i = 0; i < 12; ++i)
        f.rider(0, Shape::SQUARE);
    f.advance(29);
    CHECK(f.sim.gameOver);
    CHECK(f.sim.failedStation == 0);
    CHECK(f.population() == 21);
}
void reproducibility() {
    Fixture a, b;
    a.sim.spawning = b.sim.spawning = true;
    for (int i = 0; i < 1800; ++i) {
        a.sim.update(1.f / 60.f);
        b.sim.update(1.f / 60.f);
    }
    CHECK(a.sim.generated == b.sim.generated);
    CHECK(a.stations.getStations().size() == b.stations.getStations().size());
    for (std::size_t i = 0; i < a.stations.getStations().size(); ++i) {
        auto &x = a.stations.getStations()[i];
        auto &y = b.stations.getStations()[i];
        CHECK(x.getPosition() == y.getPosition());
        CHECK(x.passengers.size() == y.passengers.size());
    }
}
void adversarialEdits() {
    Fixture f;
    f.lines.resources.tunnels = 20;
    f.lines.resources.carriages = 30;
    std::mt19937 rng(48);
    for (int i = 0; i < 300; ++i) {
        int line = static_cast<int>(rng() % 3);
        std::vector<Station *> r;
        std::vector<int> ids = {0, 1, 2, 3, 4};
        std::shuffle(ids.begin(), ids.end(), rng);
        int n = static_cast<int>(rng() % 6);
        if (n == 1)
            n = 0;
        for (int j = 0; j < n; ++j)
            r.push_back(f.stations.byId(ids[j]));
        bool loop = n >= 3 && rng() % 2;
        CHECK(f.lines.setRoute(line, r, loop));
        f.sim.synchronize();
        f.sim.addTrain(line);
        if (!f.sim.getTrains().empty()) {
            int id = f.sim.getTrains()[rng() % f.sim.getTrains().size()].id;
            if (rng() % 3 == 0)
                f.sim.addCarriage(id);
            if (rng() % 5 == 0)
                f.sim.moveTrain(id, static_cast<int>(rng() % 3));
            if (rng() % 7 == 0)
                f.sim.reclaimTrain(id);
        }
        if (i % 5 == 0)
            f.rider(static_cast<int>(rng() % 5), static_cast<Shape>(rng() % 5));
        f.sim.update(.1f);
        CHECK(f.population() == f.sim.generated);
        CHECK(f.sim.freeEngines() >= 0);
        CHECK(f.sim.freeCarriages() >= 0);
        for (const auto &train : f.sim.getTrains()) {
            CHECK(f.lines.at(train.line).active());
            CHECK(std::isfinite(train.position.x));
            CHECK(static_cast<int>(train.passengers.size()) <= train.capacity());
        }
    }
}
void enginesStartAtFirstStop() {
    for (bool loop : {false, true}) {
        Fixture f;
        f.line(0, {2, 1, 0}, loop);
        CHECK(f.sim.addTrain(0));
        CHECK(f.sim.addTrain(0));
        CHECK(f.sim.getTrains().size() == 3);
        for (const auto &train : f.sim.getTrains()) {
            CHECK(train.from == 2);
            CHECK(train.to == 1);
            CHECK(train.distance == 0);
            CHECK(train.direction == 1);
            CHECK(length(train.position - f.lines.geometry(0).path.front()) < .01f);
        }
    }
}
} // namespace
int main() {
    std::pair<const char *, std::function<void()>> tests[] = {
        {"reversible octilinear geometry and river", geometry},
        {"stable station references", stableStations},
        {"atomic edits and loops", transactionalEdits},
        {"tunnel accounting and refunds", tunnelBudget},
        {"four parallel tracks", parallelLimit},
        {"served network routing", routing},
        {"direct delivery", directDelivery},
        {"transfer delivery", transferDelivery},
        {"directed loop", directedLoop},
        {"unreachable destination", noWrongDestination},
        {"engine and carriage conservation", resourceConservation},
        {"edit with passengers onboard", editWithRiders},
        {"remove and reassign", removeAndReassign},
        {"pause and weekly rewards", pauseAndWeek},
        {"congestion and interchange", congestionAndHub},
        {"seed reproducibility", reproducibility},
        {"300 adversarial edits", adversarialEdits},
        {"new engines start at first stop", enginesStartAtFirstStop}};
    int failed = 0;
    for (auto &test : tests)
        try {
            test.second();
            std::cout << "PASS " << test.first << '\n';
        } catch (const std::exception &e) {
            ++failed;
            std::cerr << "FAIL " << test.first << ": " << e.what() << '\n';
        }
    std::cout << std::size(tests) - failed << "/" << std::size(tests) << " scenarios passed\n";
    return failed ? 1 : 0;
}
