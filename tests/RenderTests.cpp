#include "GameEventsHandler.h"
#include <filesystem>
#include <iostream>
int main(int argc, char **argv) {
    try {
        const std::filesystem::path output = argc > 1 ? argv[1] : "screenshots";
        std::filesystem::create_directories(output);
        GameEventsHandler app(std::filesystem::path(METROFLOW_TEST_ASSETS) / "DejaVuSans.ttf");
        sf::RenderTexture target({1440, 900});
        auto save = [&](const std::string &name) {
            app.update(0);
            target.clear();
            app.draw(target);
            target.display();
            if (!target.getTexture().copyToImage().saveToFile(output / (name + ".png")))
                throw std::runtime_error("Screenshot failed: " + name);
            std::cout << "Rendered " << name << '\n';
        };
        save("title");
        app.state().title = false;
        app.game().seedDemo();
        save("gameplay");
        app.state().help = true;
        save("guide");
        app.state().help = false;
        app.game().choosingUpgrade = true;
        app.game().week = 2;
        save("upgrade");
        app.game().choosingUpgrade = false;
        app.game().gameOver = true;
        app.game().failedStation = 0;
        save("gameover");
        app.game().gameOver = false;
        app.state().resetConfirm = true;
        save("restart");
        app.state().resetConfirm = false;
        app.game().reset();
        app.game().paused = true;
        app.state().title = false;
        app.mousePressed(320, 350);
        app.mouseMoved(535, 250);
        save("edit");
        app.mouseReleased();
        app.game().reset(2026, World::MapId::Canals);
        app.game().seedDemo();
        save("canals");
        app.state().title = true;
        save("map-selection");
        app.state().title = false;
        app.game().reset();
        app.game().spawning = false;
        app.game().paused = true;
        app.lines().reset();
        app.stations().clear();
        app.lines().resources.unlockedLines = 4;
        app.lines().resources.carriages = 3;
        for (auto p : std::vector<sf::Vector2f>{{400, 300},
                                                {400, 680},
                                                {400, 600},
                                                {400, 520},
                                                {400, 440},
                                                {1000, 260},
                                                {1000, 640}})
            app.stations().addStation(Station(Shape::CIRCLE, p.x, p.y));
        for (int i = 0; i < 4; ++i)
            if (!app.lines().setRoute(i, {app.stations().byId(0), app.stations().byId(i + 1)},
                                      false))
                throw std::runtime_error("Vertical fixture rejected");
        app.game().synchronize();
        std::vector<int> ids;
        for (const auto &t : app.game().getTrains())
            ids.push_back(t.id);
        for (std::size_t i = 1; i < ids.size(); ++i)
            app.game().reclaimTrain(ids[i]);
        for (int i = 0; i < 3; ++i)
            app.game().addCarriage(ids.front());
        save("vertical-four-lines");
        if (!app.lines().setRoute(
                0, {app.stations().byId(0), app.stations().byId(5), app.stations().byId(6)}, false))
            throw std::runtime_error("Bend fixture rejected");
        app.game().synchronize();
        app.game().paused = false;
        app.game().update(1.25f);
        app.game().paused = true;
        save("carriages-on-bend");
        std::cout << "11 render states passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
