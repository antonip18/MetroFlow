#include "Simulation.h"
#include "Curves.h"
#include "TrainGeometry.h"
#include "WorldGeometry.h"
#include <algorithm>
#include <cmath>

Simulation::Simulation(StationHandler &stations, LineHandler &lines)
    : stationHandler(stations), lineHandler(lines) {
    reset();
}
void Simulation::reset(unsigned value, World::MapId map) {
    // Clear borrowed pointers before releasing the station deque.
    trains.clear();
    lineHandler.reset();
    lineHandler.map = map;
    stationHandler.clear();
    random = RandomShapeGenerator(value);
    seed = value;
    score = generated = 0;
    week = 1;
    elapsed = stationTimer = passengerTimer = 0;
    nextTrainId = 1;
    paused = gameOver = choosingUpgrade = false;
    speed = 1;
    spawning = true;
    failedStation = -1;
    message.clear();
    seenRevision.fill(-1);
    wasActive.fill(false);
    const Shape shapes[] = {Shape::CIRCLE, Shape::TRIANGLE, Shape::SQUARE, Shape::CIRCLE,
                            Shape::TRIANGLE};
    auto positions = World::initialStations(map);
    for (std::size_t i = 0; i < positions.size(); ++i)
        stationHandler.addStation(Station(shapes[i], positions[i].x, positions[i].y));
    rebuildRouter();
}
int Simulation::freeEngines() const {
    return lineHandler.resources.engines - static_cast<int>(trains.size());
}
int Simulation::freeCarriages() const {
    int n = 0;
    for (const auto &t : trains)
        n += t.carriages;
    return lineHandler.resources.carriages - n;
}
int Simulation::freeInterchanges() const {
    int n = 0;
    for (const auto &s : stationHandler.getStations())
        n += s.interchange ? 1 : 0;
    return lineHandler.resources.interchanges - n;
}
Train *Simulation::findTrain(int id) {
    for (auto &t : trains)
        if (t.id == id)
            return &t;
    return nullptr;
}
int Simulation::closestStation(sf::Vector2f p) const {
    int id = -1;
    float best = 1e9f;
    for (const auto &s : stationHandler.getStations()) {
        float d = length(s.getPosition() - p);
        if (d < best) {
            best = d;
            id = s.getId();
        }
    }
    return id;
}
void Simulation::unload(Train &train) {
    auto *s = stationHandler.byId(closestStation(train.position));
    if (s)
        for (auto p : train.passengers) {
            // Editing/reassigning never earns delivery points: riders rejoin a queue.
            s->passengers.push_back(p);
        }
    train.passengers.clear();
}
void Simulation::rebuildRouter() {
    std::array<bool, 8> served{};
    for (const auto &t : trains)
        served[t.line] = true;
    router.rebuild(stationHandler, lineHandler, served);
}
void Simulation::depart(Train &t) {
    const auto &line = lineHandler.at(t.line);
    const auto &stops = line.getStations();
    int index = line.indexOf(t.from), n = static_cast<int>(stops.size());
    if (index < 0 || n < 2) {
        t.to = t.from;
        return;
    }
    if (line.isLoop())
        t.direction = 1;
    else {
        if (index == n - 1)
            t.direction = -1;
        if (index == 0)
            t.direction = 1;
    }
    int next = (index + t.direction + n) % n;
    t.to = stops[next]->getId();
}
bool Simulation::addTrain(int line) {
    if (line < 0 || line >= 8 || !lineHandler.at(line).active()) {
        message = "Draw a line first.";
        return false;
    }
    if (freeEngines() <= 0) {
        message = "No spare locomotives. Move an existing train or wait for next week.";
        return false;
    }
    const auto &stops = lineHandler.at(line).getStations();
    Train t;
    t.id = nextTrainId++;
    t.line = line;
    t.from = stops.front()->getId();
    t.direction = 1;
    t.position = stationHandler.byId(t.from)->getPosition();
    t.dwell = PassengerRouter::Dwell;
    depart(t);
    t.position = visualTrainPosition(t, lineHandler);
    trains.push_back(t);
    rebuildRouter();
    return true;
}
bool Simulation::moveTrain(int id, int line) {
    auto *t = findTrain(id);
    if (!t || line < 0 || line >= 8 || !lineHandler.at(line).active()) {
        message = "Drop the train onto an active route.";
        return false;
    }
    if (t->line == line)
        return true;
    unload(*t);
    t->line = line;
    t->from = lineHandler.at(line).getStations().front()->getId();
    t->direction = 1;
    t->distance = 0;
    t->position = stationHandler.byId(t->from)->getPosition();
    t->dwell = PassengerRouter::Dwell;
    depart(*t);
    t->position = visualTrainPosition(*t, lineHandler);
    rebuildRouter();
    return true;
}
bool Simulation::addCarriage(int id) {
    auto *t = findTrain(id);
    if (!t) {
        message = "Drop a carriage onto a train.";
        return false;
    }
    if (freeCarriages() <= 0) {
        message = "No spare carriages.";
        return false;
    }
    if (t->carriages >= 3) {
        message = "A locomotive supports at most three carriages.";
        return false;
    }
    ++t->carriages;
    return true;
}
bool Simulation::reclaimTrain(int id) {
    auto it =
        std::find_if(trains.begin(), trains.end(), [&](const Train &t) { return t.id == id; });
    if (it == trains.end())
        return false;
    unload(*it);
    trains.erase(it);
    rebuildRouter();
    return true;
}
bool Simulation::reclaimCarriage(int id) {
    auto *t = findTrain(id);
    if (!t || t->carriages == 0)
        return false;
    --t->carriages;
    auto *s = stationHandler.byId(closestStation(t->position));
    while (static_cast<int>(t->passengers.size()) > t->capacity()) {
        s->passengers.push_back(t->passengers.back());
        t->passengers.pop_back();
    }
    return true;
}
bool Simulation::upgradeStation(int id) {
    auto *s = stationHandler.byId(id);
    if (!s || s->interchange) {
        message = "Choose a regular station.";
        return false;
    }
    if (freeInterchanges() <= 0) {
        message = "No spare interchanges.";
        return false;
    }
    s->interchange = true;
    return true;
}
bool Simulation::chooseUpgrade(Upgrade upgrade) {
    if (!choosingUpgrade)
        return false;
    auto &r = lineHandler.resources;
    switch (upgrade) {
    case Upgrade::Line:
        if (r.unlockedLines >= 8) {
            message = "All eight lines are unlocked.";
            return false;
        }
        ++r.unlockedLines;
        break;
    case Upgrade::Carriage:
        ++r.carriages;
        break;
    case Upgrade::Tunnels:
        r.tunnels += 2;
        break;
    case Upgrade::Interchange:
        ++r.interchanges;
        break;
    }
    choosingUpgrade = false;
    return true;
}
void Simulation::synchronize() {
    bool dirty = false;
    for (int l = 0; l < 8; ++l) {
        const auto &line = lineHandler.at(l);
        if (seenRevision[l] == line.revision())
            continue;
        dirty = true;
        seenRevision[l] = line.revision();
        for (auto it = trains.begin(); it != trains.end();) {
            if (it->line != l) {
                ++it;
                continue;
            }
            if (!line.active()) {
                unload(*it);
                it = trains.erase(it);
                continue;
            }
            bool edgeExists = false;
            for (std::size_t e = 0; e < line.edgeCount(); ++e) {
                auto [a, b] = line.edge(e);
                if (a->getId() == it->from && b->getId() == it->to) {
                    edgeExists = true;
                    it->direction = 1;
                }
                if (!line.isLoop() && b->getId() == it->from && a->getId() == it->to) {
                    edgeExists = true;
                    it->direction = -1;
                }
            }
            if (!edgeExists) {
                // Changed track snaps to its closest surviving station; keep all riders onboard.
                float best = 1e9f;
                Station *nearest = nullptr;
                for (auto *s : line.getStations()) {
                    float d = length(s->getPosition() - it->position);
                    if (d < best) {
                        best = d;
                        nearest = s;
                    }
                }
                it->from = nearest->getId();
                it->position = nearest->getPosition();
                it->distance = 0;
                it->dwell = PassengerRouter::Dwell;
                depart(*it);
            }
            ++it;
        }
        if (!wasActive[l] && line.active() && freeEngines() > 0)
            addTrain(l);
        wasActive[l] = line.active();
    }
    for (auto &train : trains)
        train.position = visualTrainPosition(train, lineHandler);
    if (dirty)
        rebuildRouter();
}
void Simulation::serve(Train &t, Station &s) {
    // Arrival is the only operation that awards a delivery.
    for (auto it = t.passengers.begin(); it != t.passengers.end();) {
        if (it->getDestination() == s.getShape()) {
            ++score;
            it = t.passengers.erase(it);
        } else if (!router.stay(s.getId(), t.to, it->getDestination(), t.line)) {
            s.passengers.push_back(*it);
            it = t.passengers.erase(it);
        } else
            ++it;
    }
    for (auto it = s.passengers.begin();
         it != s.passengers.end() && static_cast<int>(t.passengers.size()) < t.capacity();) {
        // Riders returned here by a network edit are delivered by a stopped train, not by the edit.
        if (it->getDestination() == s.getShape()) {
            ++score;
            it = s.passengers.erase(it);
        } else if (router.board(s.getId(), t.to, it->getDestination(), t.line)) {
            t.passengers.push_back(*it);
            it = s.passengers.erase(it);
        } else
            ++it;
    }
}
void Simulation::updateTrain(Train &t, float dt) {
    if (t.dwell > 0) {
        t.dwell -= dt;
        if (t.dwell <= 0) {
            serve(t, *stationHandler.byId(t.from));
            t.dwell = 0;
        }
        return;
    }
    auto *a = stationHandler.byId(t.from);
    auto *b = stationHandler.byId(t.to);
    if (!a || !b)
        return;
    const auto &line = lineHandler.at(t.line);
    int edge = t.direction == 1 ? line.indexOf(t.from) : line.indexOf(t.to);
    auto path = lineHandler.edgePath(t.line, edge);
    float total = pathLength(path);
    t.distance += PassengerRouter::Speed * dt;
    if (t.distance >= total) {
        t.from = t.to;
        t.distance = 0;
        t.position = b->getPosition();
        depart(t);
        serve(t, *b);
        t.dwell = PassengerRouter::Dwell;
    }
    t.position = visualTrainPosition(t, lineHandler);
}
void Simulation::generateNewStations(float dt) {
    stationTimer += dt;
    if (stationTimer < 21.f || stationHandler.getStations().size() >= 24)
        return;
    stationTimer -= 21.f;
    for (int trial = 0; trial < 120; ++trial) {
        auto p = random.position();
        if (World::water(p, 45, lineHandler.map))
            continue;
        bool free = true;
        for (const auto &s : stationHandler.getStations())
            if (length(p - s.getPosition()) < 125.f) {
                free = false;
                break;
            }
        if (free) {
            stationHandler.addStation(Station(
                random.shape(static_cast<int>(stationHandler.getStations().size())), p.x, p.y));
            rebuildRouter();
            return;
        }
    }
}
void Simulation::generatePassengers(float dt) {
    passengerTimer += dt;
    float interval = std::max(.4f, 1.65f - .075f * static_cast<float>(week - 1));
    if (passengerTimer < interval)
        return;
    passengerTimer -= interval;
    auto &stations = stationHandler.getStations();
    int count = static_cast<int>(stations.size());
    auto &origin = stations[static_cast<std::size_t>(random.integer(0, count - 1))];
    std::vector<Shape> destinations;
    for (const auto &s : stations)
        if (s.getShape() != origin.getShape())
            destinations.push_back(s.getShape());
    if (destinations.empty())
        return;
    origin.passengers.emplace_back(destinations[static_cast<std::size_t>(
        random.integer(0, static_cast<int>(destinations.size()) - 1))]);
    ++generated;
}
void Simulation::tick(float dt) {
    elapsed += dt;
    if (spawning) {
        generateNewStations(dt);
        generatePassengers(dt);
    }
    for (auto &t : trains)
        updateTrain(t, dt);
    for (auto &s : stationHandler.getStations()) {
        if (static_cast<int>(s.passengers.size()) > s.capacity())
            s.overcrowding += dt;
        else
            s.overcrowding = std::max(0.f, s.overcrowding - dt * 2.f);
        if (s.overcrowding >= OvercrowdingLimit) {
            gameOver = true;
            failedStation = s.getId();
        }
    }
    if (!gameOver && elapsed >= static_cast<float>(week) * WeekDuration) {
        ++week;
        ++lineHandler.resources.engines;
        choosingUpgrade = true;
    }
}
void Simulation::update(float dt) {
    synchronize();
    if (paused || gameOver || choosingUpgrade)
        return;
    // Fixed substeps bound arrival/congestion errors; the UI supplies at most 0.25 wall seconds.
    float remaining = std::max(0.f, dt) * static_cast<float>(speed);
    while (remaining > 0.00001f && !gameOver && !choosingUpgrade) {
        float step = std::min(remaining, 1.f / 60.f);
        tick(step);
        remaining -= step;
    }
}
void Simulation::seedDemo() {
    reset(2026, lineHandler.map);
    spawning = false;
    if (lineHandler.map == World::MapId::River) {
        stationHandler.addStation(Station(Shape::DIAMOND, 250, 600));
        stationHandler.addStation(Station(Shape::SQUARE, 1190, 230));
        stationHandler.addStation(Station(Shape::STAR, 930, 650));
    } else {
        stationHandler.addStation(Station(Shape::DIAMOND, 240, 655));
        stationHandler.addStation(Station(Shape::SQUARE, 1250, 230));
        stationHandler.addStation(Station(Shape::STAR, 780, 435));
    }
    auto route = [&](std::initializer_list<int> ids) {
        std::vector<Station *> r;
        for (int i : ids)
            r.push_back(stationHandler.byId(i));
        return r;
    };
    if (lineHandler.map == World::MapId::River) {
        lineHandler.setRoute(0, route({5, 0, 1, 3, 6}), false);
        lineHandler.setRoute(1, route({1, 2, 7, 4, 3}), false);
        lineHandler.setRoute(2, route({0, 2, 5}), true);
    } else {
        lineHandler.setRoute(0, route({0, 1, 6}), false);
        lineHandler.setRoute(1, route({1, 7, 2, 5}), false);
        lineHandler.setRoute(2, route({2, 7, 3, 4}), false);
    }
    synchronize();
    addCarriage(1);
    upgradeStation(1);
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 3; ++j) {
            auto *s = stationHandler.byId(i);
            auto sh = stationHandler.byId((i + j + 1) % 8)->getShape();
            if (sh != s->getShape()) {
                s->passengers.emplace_back(sh);
                ++generated;
            }
        }
    update(2.f);
    paused = true;
}

std::array<sf::Vector2f, 3> Simulation::endpointHandle(int line, bool start) const {
    // Keep caps clear of the largest consist at either terminal.
    float clearance = TrackStyle::HandleLength;
    for (const auto &train : trains)
        if (train.line == line)
            clearance =
                std::max(clearance, (static_cast<float>(train.carriages) * TrackStyle::CarSpacing +
                                     TrackStyle::CarLength) *
                                            .5f +
                                        TrackStyle::Width * 1.5f);
    return lineHandler.handle(line, start, clearance);
}
