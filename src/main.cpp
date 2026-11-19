#include "GameEventsHandler.h"
#include "WorldGeometry.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
namespace {
std::filesystem::path scoreFile() {
    const char *base = std::getenv("XDG_STATE_HOME");
    std::filesystem::path dir;
    if (base)
        dir = base;
    else if (const char *user = std::getenv("HOME"))
        dir = std::filesystem::path(user) / ".local" / "state";
    else
        dir = std::filesystem::temp_directory_path();
    return dir / "MetroFlow" / "best-score.txt";
}
void fit(sf::RenderWindow &window) {
    auto size = window.getSize();
    if (!size.x || !size.y)
        return;
    float ratio = static_cast<float>(size.x) / static_cast<float>(size.y),
          ideal = World::Width / World::Height;
    sf::View view(sf::FloatRect({0, 0}, {World::Width, World::Height}));
    if (ratio > ideal) {
        float w = ideal / ratio;
        view.setViewport({{(1 - w) / 2, 0}, {w, 1}});
    } else {
        float h = ratio / ideal;
        view.setViewport({{0, (1 - h) / 2}, {1, h}});
    }
    window.setView(view);
}
} // namespace
int main(int argc, char **argv) {
    try {
        bool demo = false;
        int frames = -1;
        unsigned seed = 2026;
        World::MapId map = World::MapId::River;
        std::string screenshot;
        std::filesystem::path assetOverride;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--demo")
                demo = true;
            else if (arg == "--map" && i + 1 < argc) {
                std::string name = argv[++i];
                if (name != "river" && name != "canals")
                    throw std::runtime_error("Map must be river or canals.");
                map = name == "river" ? World::MapId::River : World::MapId::Canals;
            } else if (arg == "--seed" && i + 1 < argc)
                seed = static_cast<unsigned>(std::stoul(argv[++i]));
            else if (arg == "--frames" && i + 1 < argc)
                frames = std::stoi(argv[++i]);
            else if (arg == "--screenshot" && i + 1 < argc)
                screenshot = argv[++i];
            else if (arg == "--assets" && i + 1 < argc)
                assetOverride = argv[++i];
            else {
                std::cout << "MetroFlow [--map river|canals] [--seed N] [--demo] [--frames N] "
                             "[--screenshot path.png] "
                             "[--assets directory]\n";
                return arg == "--help" ? 0 : 1;
            }
        }
        auto assets = assetOverride.empty()
                          ? std::filesystem::absolute(argv[0]).parent_path() / "assets"
                          : assetOverride;
        if (!std::filesystem::exists(assets / "DejaVuSans.ttf"))
            assets = "assets";
        sf::ContextSettings settings;
        settings.antiAliasingLevel = 4;
        sf::RenderWindow window(sf::VideoMode({1440, 900}), "MetroFlow", sf::Style::Default,
                                sf::State::Windowed, settings);
        window.setVerticalSyncEnabled(true);
        window.setKeyRepeatEnabled(false);
        fit(window);
        GameEventsHandler game(assets / "DejaVuSans.ttf");
        game.game().reset(seed, map);
        if (demo) {
            game.game().seedDemo();
            game.state().title = false;
        }
        auto score = scoreFile();
        {
            std::ifstream in(score);
            int value = 0;
            if (in >> value)
                game.state().bestScore = std::max(0, value);
        }
        sf::Clock clock;
        int frame = 0;
        while (window.isOpen()) {
            while (const auto event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>())
                    window.close();
                if (event->is<sf::Event::Resized>())
                    fit(window);
                if (event->is<sf::Event::FocusLost>()) {
                    game.game().paused = true;
                    game.cancelInteraction();
                }
                if (const auto *e = event->getIf<sf::Event::KeyPressed>())
                    game.keyPressed(e->code);
                if (const auto *e = event->getIf<sf::Event::MouseButtonPressed>()) {
                    auto p = window.mapPixelToCoords(e->position);
                    if (e->button == sf::Mouse::Button::Left ||
                        e->button == sf::Mouse::Button::Right)
                        game.mousePressed(
                            p.x, p.y, e->button == sf::Mouse::Button::Right,
                            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift));
                }
                if (const auto *e = event->getIf<sf::Event::MouseMoved>()) {
                    auto p = window.mapPixelToCoords(e->position);
                    game.mouseMoved(p.x, p.y);
                }
                if (const auto *e = event->getIf<sf::Event::MouseButtonReleased>();
                    e && e->button == sf::Mouse::Button::Left) {
                    auto p = window.mapPixelToCoords(e->position);
                    game.mouseMoved(p.x, p.y);
                    game.mouseReleased();
                }
            }
            if (!window.isOpen())
                break;
            game.update(std::min(clock.restart().asSeconds(), .25f));
            window.clear(sf::Color(35, 58, 52));
            game.draw(window);
            window.display();
            ++frame;
            if ((frames >= 0 && frame >= frames) || (!screenshot.empty() && frame >= 3)) {
                if (!screenshot.empty()) {
                    sf::Texture capture(window.getSize());
                    capture.update(window);
                    if (!capture.copyToImage().saveToFile(screenshot))
                        throw std::runtime_error("Could not write screenshot.");
                }
                window.close();
            }
        }
        if (!demo && frames < 0) {
            std::error_code error;
            std::filesystem::create_directories(score.parent_path(), error);
            if (!error) {
                std::ofstream out(score);
                if (out)
                    out << game.state().bestScore << '\n';
            }
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "MetroFlow: " << e.what() << '\n';
        return 1;
    }
}
