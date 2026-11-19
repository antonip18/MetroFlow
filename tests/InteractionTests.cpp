#include "Curves.h"
#include "GameEventsHandler.h"
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition))                                                                          \
            throw std::runtime_error(std::string(__func__) + ":" + std::to_string(__LINE__) +      \
                                     " " #condition);                                              \
    } while (false)
namespace {
struct Fixture {
    GameEventsHandler app{std::filesystem::path(METROFLOW_TEST_ASSETS) / "DejaVuSans.ttf"};
    Fixture() {
        app.state().title = false;
        app.game().spawning = false;
        app.game().paused = true;
        app.lines().resources.tunnels = 20;
    }
    sf::Vector2f at(int id) { return app.stations().byId(id)->getPosition(); }
    void press(sf::Vector2f p) { app.mousePressed(p.x, p.y); }
    void move(sf::Vector2f p) { app.mouseMoved(p.x, p.y); }
    void drag(sf::Vector2f a, sf::Vector2f b) {
        press(a);
        move(b);
        app.mouseReleased();
    }
    void route(int l, std::initializer_list<int> stops) {
        std::vector<Station *> r;
        for (int i : stops)
            r.push_back(app.stations().byId(i));
        CHECK(app.lines().setRoute(l, r, false));
        app.game().synchronize();
    }
    void moveTrainsAway() {
        app.game().paused = false;
        app.game().update(1.4f);
        app.game().paused = true;
    }
    bool equal(int l, std::initializer_list<int> ids) {
        std::vector<int> actual;
        for (auto *s : app.lines().at(l).getStations())
            actual.push_back(s->getId());
        return actual == std::vector<int>(ids);
    }
};
void buildBothEnds() {
    Fixture f;
    f.drag(f.at(0), f.at(1));
    CHECK(f.equal(0, {0, 1}));
    CHECK(f.app.game().getTrains().size() == 1);
    f.moveTrainsAway();
    f.drag(f.at(1), f.at(2));
    CHECK(f.equal(0, {0, 1, 2}));
    f.drag(f.at(0), f.at(3));
    CHECK(f.equal(0, {3, 0, 1, 2}));
}
void trim() {
    Fixture f;
    f.route(0, {0, 1, 2, 3});
    f.moveTrainsAway();
    f.drag(f.at(3), f.at(1));
    CHECK(f.equal(0, {0, 1}));
}
void insert() {
    Fixture f;
    f.route(0, {0, 1});
    f.moveTrainsAway();
    auto p = f.app.lines().edgePath(0, 0);
    f.drag(pointOnPath(p, pathLength(p) * .6f), f.at(2));
    CHECK(f.equal(0, {0, 2, 1}));
}
void loopAndOpen() {
    Fixture f;
    f.route(0, {0, 1, 2});
    f.moveTrainsAway();
    f.drag(f.at(2), f.at(0));
    CHECK(f.app.lines().at(0).isLoop());
    auto p = f.app.lines().edgePath(0, 2);
    auto point = pointOnPath(p, pathLength(p) * .5f);
    f.app.mousePressed(point.x, point.y, true);
    CHECK(!f.app.lines().at(0).isLoop());
    CHECK(f.app.lines().at(0).getStations().size() == 3);
}
void reorderInterior() {
    Fixture f;
    f.route(0, {0, 1, 2, 3});
    f.moveTrainsAway();
    auto p = f.app.lines().edgePath(0, 2);
    f.drag(f.at(1), pointOnPath(p, pathLength(p) * .5f));
    CHECK(f.equal(0, {0, 2, 1, 3}));
}
void replaceAndRemove() {
    Fixture f;
    f.route(0, {0, 1, 2});
    f.moveTrainsAway();
    f.drag(f.at(1), f.at(3));
    CHECK(f.equal(0, {0, 3, 2}));
    auto p = f.at(3);
    f.app.mousePressed(p.x, p.y, true);
    CHECK(f.equal(0, {0, 2}));
}
void cancelAndReject() {
    Fixture f;
    f.route(0, {0, 1});
    f.moveTrainsAway();
    f.press(f.at(1));
    f.move(f.at(2));
    f.app.keyPressed(sf::Keyboard::Key::Escape);
    f.app.mouseReleased();
    CHECK(f.equal(0, {0, 1}));
    f.app.lines().resources.tunnels = 0;
    f.drag(f.at(1), f.at(3));
    CHECK(f.equal(0, {0, 1}));
    CHECK(!f.app.state().toast.empty());
}
void resources() {
    Fixture f;
    f.route(0, {0, 1});
    f.moveTrainsAway();
    auto p = f.app.lines().edgePath(0, 0);
    auto reserve = UiLayout::resource(0).position + sf::Vector2f{15, 15};
    f.drag(reserve, pointOnPath(p, pathLength(p) * .7f));
    CHECK(f.app.game().getTrains().size() == 2);
    auto train = visualTrainPosition(f.app.game().getTrains()[0], f.app.lines());
    f.drag(UiLayout::resource(1).position + sf::Vector2f{15, 15}, train);
    CHECK(f.app.game().getTrains()[0].carriages == 1);
    f.drag(UiLayout::resource(3).position + sf::Vector2f{15, 15}, f.at(1));
    CHECK(f.app.stations().byId(1)->interchange);
}
void moveTrain() {
    Fixture f;
    f.route(0, {0, 1});
    f.route(1, {2, 3});
    f.moveTrainsAway();
    auto first = f.app.game().getTrains()[0];
    auto path = f.app.lines().edgePath(1, 0);
    f.drag(visualTrainPosition(first, f.app.lines()), pointOnPath(path, pathLength(path) * .6f));
    CHECK(f.app.game().getTrains()[0].line == 1);
    auto train = f.app.game().getTrains()[0];
    f.drag(visualTrainPosition(train, f.app.lines()),
           UiLayout::resource(0).position + sf::Vector2f{20, 20});
    CHECK(f.app.game().getTrains().size() == 1);
}
void pauseModalAndReset() {
    Fixture f;
    f.app.keyPressed(sf::Keyboard::Key::Space);
    CHECK(!f.app.game().paused);
    f.app.keyPressed(sf::Keyboard::Key::H);
    float time = f.app.game().elapsed;
    f.app.update(2);
    CHECK(f.app.game().elapsed == time);
    f.app.keyPressed(sf::Keyboard::Key::Escape);
    f.app.update(1);
    CHECK(f.app.game().elapsed > time);
    f.app.keyPressed(sf::Keyboard::Key::R);
    CHECK(f.app.state().resetConfirm);
    f.app.keyPressed(sf::Keyboard::Key::Escape);
    CHECK(!f.app.state().resetConfirm);
    f.app.keyPressed(sf::Keyboard::Key::R);
    f.app.keyPressed(sf::Keyboard::Key::Enter);
    CHECK(f.app.game().elapsed == 0);
    CHECK(f.app.game().getTrains().empty());
}
void upgradeAndDelete() {
    Fixture f;
    f.route(0, {0, 1});
    f.app.keyPressed(sf::Keyboard::Key::Delete);
    CHECK(f.app.lines().at(0).isEmpty());
    CHECK(f.app.game().freeEngines() == 3);
    f.app.game().paused = false;
    f.app.game().update(91);
    CHECK(f.app.game().choosingUpgrade);
    f.app.keyPressed(sf::Keyboard::Key::Num1);
    CHECK(f.app.lines().resources.unlockedLines == 4);
    CHECK(!f.app.game().choosingUpgrade);
}
void unselectedHandle() {
    Fixture f;
    f.route(1, {0, 1});
    f.app.keyPressed(sf::Keyboard::Key::Num1);
    auto h = f.app.game().endpointHandle(1, false);
    f.press((h[1] + h[2]) * .5f);
    CHECK(f.app.state().editLine == 1);
    CHECK(f.app.state().edit == Edit::End);
    f.move(f.at(2));
    f.app.mouseReleased();
    CHECK(f.equal(1, {0, 1, 2}));
}
void mapSelection() {
    Fixture f;
    f.app.state().title = true;
    f.press(UiLayout::mapCard(1).position + sf::Vector2f{20, 20});
    CHECK(f.app.lines().map == World::MapId::Canals);
    f.app.keyPressed(sf::Keyboard::Key::Enter);
    CHECK(!f.app.state().title);
    CHECK(f.at(0) == World::initialStations(World::MapId::Canals)[0]);
    f.app.keyPressed(sf::Keyboard::Key::R);
    f.app.keyPressed(sf::Keyboard::Key::Enter);
    CHECK(f.app.lines().map == World::MapId::Canals);
    f.press(UiLayout::maps().position + sf::Vector2f{20, 20});
    CHECK(f.app.state().title);
    CHECK(f.app.game().paused);
    f.app.keyPressed(sf::Keyboard::Key::Num1);
    f.app.keyPressed(sf::Keyboard::Key::Enter);
    CHECK(f.app.lines().map == World::MapId::River);
}
void trimThroughTerminal() {
    for (bool start : {false, true}) {
        Fixture f;
        f.route(0, {0, 1, 2, 3});
        auto h = f.app.game().endpointHandle(0, start);
        f.press((h[1] + h[2]) * .5f);
        f.move(f.at(start ? 0 : 3));
        CHECK(f.app.state().preview.size() == 3);
        // Previewing removal must not change the live route until release.
        CHECK(f.equal(0, {0, 1, 2, 3}));
        f.app.mouseReleased();
        CHECK(start ? f.equal(0, {1, 2, 3}) : f.equal(0, {0, 1, 2}));
    }
}
void continuousHandleTrim() {
    for (bool start : {false, true}) {
        Fixture f;
        f.route(0, {0, 1, 2, 3});
        auto h = f.app.game().endpointHandle(0, start);
        f.press((h[1] + h[2]) * .5f);
        f.move(f.at(start ? 0 : 3));
        f.move(f.at(start ? 1 : 2));
        f.app.mouseReleased();
        CHECK(start ? f.equal(0, {2, 3}) : f.equal(0, {0, 1}));
    }
}
void trimCancelAndClear() {
    Fixture f;
    f.route(0, {0, 1});
    auto h = f.app.game().endpointHandle(0, false);
    f.press((h[1] + h[2]) * .5f);
    f.move(f.at(1));
    f.app.keyPressed(sf::Keyboard::Key::Escape);
    f.app.mouseReleased();
    CHECK(f.equal(0, {0, 1}));
    CHECK(f.app.game().getTrains().size() == 1);
    f.press((h[1] + h[2]) * .5f);
    f.move(f.at(1));
    f.move(f.at(0));
    CHECK(f.app.state().preview.empty());
    f.app.mouseReleased();
    CHECK(f.app.lines().at(0).isEmpty());
    CHECK(f.app.game().freeEngines() == 3);
}
void removedExampleButton() {
    Fixture f;
    f.app.state().title = true;
    f.press(UiLayout::secondary().position + sf::Vector2f{30, 20});
    f.app.mouseReleased();
    CHECK(f.app.state().title);
    CHECK(f.app.game().getTrains().empty());
    CHECK(f.app.stations().getStations().size() == 5);
}
} // namespace
int main() {
    std::pair<const char *, std::function<void()>> tests[] = {
        {"build and extend both ends", buildBothEnds},
        {"backtracking trims", trim},
        {"insert from track", insert},
        {"close and open loop", loopAndOpen},
        {"reorder interior stop", reorderInterior},
        {"replace and remove stop", replaceAndRemove},
        {"cancel and reject invalid preview", cancelAndReject},
        {"drag resources", resources},
        {"drag and return train", moveTrain},
        {"pause, help and restart", pauseModalAndReset},
        {"delete and upgrade", upgradeAndDelete},
        {"unselected handle", unselectedHandle},
        {"map selection and restart", mapSelection},
        {"trim terminal using either handle", trimThroughTerminal},
        {"continuous handle trimming", continuousHandleTrim},
        {"cancel trim and reclaim entire line", trimCancelAndClear},
        {"removed example button is inactive", removedExampleButton}};
    int failed = 0;
    for (auto &test : tests)
        try {
            test.second();
            std::cout << "PASS " << test.first << '\n';
        } catch (const std::exception &e) {
            ++failed;
            std::cerr << "FAIL " << test.first << ": " << e.what() << '\n';
        }
    std::cout << std::size(tests) - failed << "/" << std::size(tests)
              << " interaction scenarios passed\n";
    return failed ? 1 : 0;
}
