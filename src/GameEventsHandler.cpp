#include "GameEventsHandler.h"
#include "Curves.h"
#include <algorithm>
#include <cmath>
GameEventsHandler::GameEventsHandler(const std::filesystem::path &fontPath)
    : simulation(stationHandler, lineHandler),
      drawingHandler(stationHandler, lineHandler, lineSelector, simulation, fontPath) {}
void GameEventsHandler::notify(const std::string &s) {
    if (s.empty())
        return;
    ui.toast = s;
    ui.toastTime = 5.f;
}
void GameEventsHandler::selectLine(int i) {
    if (i < 0 || i >= lineHandler.resources.unlockedLines)
        return;
    lineHandler.setCurrentLine(getLineColor(i));
    lineSelector.setSelectedColor(getLineColor(i));
    ui.tool = Tool::None;
}
int GameEventsHandler::trainAt(sf::Vector2f p) const {
    for (const auto &t : simulation.getTrains())
        for (const auto &pose : trainPoses(t, lineHandler)) {
            auto d = p - pose.position;
            float along = d.x * pose.tangent.x + d.y * pose.tangent.y;
            float across = d.x * pose.tangent.y - d.y * pose.tangent.x;
            if (std::abs(along) < TrackStyle::CarLength * .5f + 3 &&
                std::abs(across) < TrackStyle::CarWidth * .5f + 3)
                return t.id;
        }
    return -1;
}
void GameEventsHandler::cancelInteraction() {
    ui.edit = Edit::None;
    ui.preview.clear();
    ui.draggedTrain = -1;
    ui.tool = Tool::None;
    ui.lastStation = -1;
}
void GameEventsHandler::reset() {
    cancelInteraction();
    simulation.reset(simulation.seed, lineHandler.map);
    ui.title = ui.help = ui.resetConfirm = false;
    selectLine(0);
    ui.toastTime = 0;
}
bool GameEventsHandler::commit(int line, std::vector<Station *> route, bool loop) {
    if (route.size() < 2) {
        route.clear();
        loop = false;
    }
    if (route.size() < 3)
        loop = false;
    if (!lineHandler.setRoute(line, route, loop)) {
        notify(lineHandler.message);
        return false;
    }
    simulation.synchronize();
    return true;
}
void GameEventsHandler::beginEdit(int line, Edit edit, int index) {
    selectLine(line);
    ui.editLine = line;
    ui.edit = edit;
    ui.editIndex = index;
    ui.preview = lineHandler.at(line).getStations();
    ui.previewLoop = lineHandler.at(line).isLoop();
    ui.fromHandle = false;
}
void GameEventsHandler::removeAt(sf::Vector2f p, bool shift) {
    int train = trainAt(p);
    if (train >= 0) {
        if (shift)
            simulation.reclaimCarriage(train);
        else
            simulation.reclaimTrain(train);
        return;
    }
    int l = lineHandler.selected();
    auto route = lineHandler.at(l).getStations();
    bool loop = lineHandler.at(l).isLoop();
    if (auto *s = stationHandler.getStationAt(p.x, p.y)) {
        auto it = std::find(route.begin(), route.end(), s);
        if (it != route.end()) {
            route.erase(it);
            commit(l, route, loop);
        }
        return;
    }
    if (auto hit = lineHandler.segmentAt(p)) {
        l = hit->line;
        selectLine(l);
        route = lineHandler.at(l).getStations();
        loop = lineHandler.at(l).isLoop();
        if (loop) {
            std::rotate(route.begin(), route.begin() + ((hit->edge + 1) % route.size()),
                        route.end());
            loop = false;
        } else {
            std::size_t cut = static_cast<std::size_t>(hit->edge + 1);
            if (cut >= route.size() - cut)
                route.erase(route.begin() + static_cast<std::ptrdiff_t>(cut), route.end());
            else
                route.erase(route.begin(), route.begin() + static_cast<std::ptrdiff_t>(cut));
        }
        commit(l, route, loop);
    }
}
void GameEventsHandler::applyTool() {
    bool ok = false;
    if (ui.tool == Tool::Engine) {
        if (auto hit = lineHandler.segmentAt(ui.mouse, 24))
            ok = simulation.addTrain(hit->line);
        else if (auto *s = stationHandler.getStationAt(ui.mouse.x, ui.mouse.y)) {
            if (lineHandler.at(lineHandler.selected()).indexOf(s->getId()) >= 0)
                ok = simulation.addTrain(lineHandler.selected());
        }
    } else if (ui.tool == Tool::Carriage)
        ok = simulation.addCarriage(trainAt(ui.mouse));
    else if (ui.tool == Tool::Interchange) {
        if (auto *s = stationHandler.getStationAt(ui.mouse.x, ui.mouse.y))
            ok = simulation.upgradeStation(s->getId());
    }
    if (ok) {
        notify("Resource deployed.");
        ui.tool = Tool::None;
    } else
        notify(simulation.message.empty() ? "Drop this resource onto a compatible target."
                                          : simulation.message);
    simulation.message.clear();
}
void GameEventsHandler::mousePressed(float x, float y, bool right, bool shift) {
    ui.mouse = ui.press = {x, y};
    ui.moved = false;
    ui.lastStation = -1;
    if (ui.title) {
        for (int i = 0; i < 2; ++i)
            if (UiLayout::mapCard(i).contains(ui.mouse)) {
                simulation.reset(simulation.seed, static_cast<World::MapId>(i));
                selectLine(0);
                return;
            }
        if (UiLayout::play().contains(ui.mouse)) {
            reset();
            notify("Select a colour, then drag between stations.");
        }
        return;
    }
    if (ui.help) {
        ui.help = false;
        return;
    }
    if (ui.resetConfirm) {
        if (UiLayout::play().contains(ui.mouse))
            reset();
        else if (UiLayout::secondary().contains(ui.mouse))
            ui.resetConfirm = false;
        return;
    }
    if (simulation.gameOver) {
        if (UiLayout::play().contains(ui.mouse))
            reset();
        return;
    }
    if (simulation.choosingUpgrade) {
        for (int i = 0; i < 4; ++i)
            if (UiLayout::upgrade(i).contains(ui.mouse)) {
                if (!simulation.chooseUpgrade(static_cast<Upgrade>(i)))
                    notify(simulation.message);
            }
        return;
    }
    if (right) {
        cancelInteraction();
        removeAt(ui.mouse, shift);
        return;
    }
    if (UiLayout::maps().contains(ui.mouse)) {
        cancelInteraction();
        ui.title = true;
        simulation.paused = true;
        return;
    }
    if (UiLayout::pause().contains(ui.mouse)) {
        simulation.paused = !simulation.paused;
        return;
    }
    if (UiLayout::speed().contains(ui.mouse)) {
        simulation.speed = simulation.speed == 1 ? 2 : simulation.speed == 2 ? 4 : 1;
        return;
    }
    if (UiLayout::help().contains(ui.mouse)) {
        cancelInteraction();
        ui.help = true;
        return;
    }
    if (UiLayout::removeLine().contains(ui.mouse)) {
        commit(lineHandler.selected(), {}, false);
        return;
    }
    if (auto color = lineSelector.getColorAt(x, y)) {
        selectLine(getLineIndex(*color));
        return;
    }
    for (int i = 0; i < 4; ++i)
        if (UiLayout::resource(i).contains(ui.mouse)) {
            if (i == 2) {
                notify("Tunnels are used automatically by river crossings. Editing refunds unused "
                       "tunnels.");
                return;
            }
            Tool tool = i == 0 ? Tool::Engine : i == 1 ? Tool::Carriage : Tool::Interchange;
            ui.tool = ui.tool == tool ? Tool::None : tool;
            return;
        }
    if (ui.tool != Tool::None) {
        applyTool();
        return;
    }
    // Every cap is clickable; selected line wins exact ties.
    for (int step = 0; step < 8; ++step) {
        int i = (lineHandler.selected() + step) % 8;
        if (!lineHandler.at(i).active() || lineHandler.at(i).isLoop())
            continue;
        for (bool start : {true, false}) {
            auto h = simulation.endpointHandle(i, start);
            if (distanceToPath(ui.mouse, {h[1], h[2]}) < 7.f) {
                beginEdit(i, start ? Edit::Start : Edit::End, -1);
                ui.fromHandle = true;
                return;
            }
        }
    }
    ui.draggedTrain = trainAt(ui.mouse);
    if (ui.draggedTrain >= 0)
        return;
    const int selected = lineHandler.selected();
    const auto &line = lineHandler.at(selected);
    if (auto *s = stationHandler.getStationAt(x, y)) {
        ui.lastStation = s->getId();
        if (line.isEmpty()) {
            beginEdit(selected, Edit::End, -1);
            ui.preview = {s};
            return;
        }
        int index = line.indexOf(s->getId());
        if (index >= 0) {
            if (!line.isLoop() && index == 0)
                beginEdit(selected, Edit::Start, index);
            else if (!line.isLoop() && index == static_cast<int>(line.getStations().size()) - 1)
                beginEdit(selected, Edit::End, index);
            else
                beginEdit(selected, Edit::Replace, index);
        } else
            notify("Drag an endpoint or a track segment to this station.");
        return;
    }
    if (auto hit = lineHandler.segmentAt(ui.mouse)) {
        beginEdit(hit->line, Edit::Insert, hit->edge);
        return;
    }
}
void GameEventsHandler::mouseMoved(float x, float y) {
    ui.mouse = {x, y};
    if (length(ui.mouse - ui.press) > 5)
        ui.moved = true;
    if (ui.edit == Edit::None)
        return;
    auto *s = stationHandler.getStationAt(x, y);
    if (!s || s->getId() == ui.lastStation)
        return;
    ui.lastStation = s->getId();
    auto &r = ui.preview;
    auto it = std::find(r.begin(), r.end(), s);
    if (ui.edit == Edit::Replace)
        return;
    if (ui.edit == Edit::Insert) {
        if (it == r.end()) {
            r.insert(r.begin() + ui.editIndex + 1, s);
            ++ui.editIndex;
        }
        return;
    }
    if (ui.previewLoop)
        return;
    if (it == r.end()) {
        if (ui.edit == Edit::Start)
            r.insert(r.begin(), s);
        else
            r.push_back(s);
        return;
    }
    std::size_t index = static_cast<std::size_t>(it - r.begin());
    // A handle starts outside the platform: crossing the terminal removes it.
    // Keep station-origin drags unchanged, so either endpoint can still be extended.
    bool terminal = ui.edit == Edit::Start ? index == 0 : index == r.size() - 1;
    if (ui.fromHandle && terminal) {
        r.erase(it);
        return;
    }
    bool opposite = ui.edit == Edit::End ? index == 0 : index == r.size() - 1;
    if (opposite && r.size() >= 3) {
        ui.previewLoop = true;
        return;
    }
    if (ui.edit == Edit::Start)
        r.erase(r.begin(), ui.fromHandle ? it + 1 : it);
    else
        r.erase(ui.fromHandle ? it : it + 1, r.end());
}
void GameEventsHandler::mouseReleased() {
    if (ui.title || ui.help || ui.resetConfirm || simulation.gameOver || simulation.choosingUpgrade)
        return;
    if (ui.draggedTrain >= 0) {
        if (auto hit = lineHandler.segmentAt(ui.mouse, 24)) {
            if (!simulation.moveTrain(ui.draggedTrain, hit->line))
                notify(simulation.message);
        } else if (UiLayout::resource(0).contains(ui.mouse))
            simulation.reclaimTrain(ui.draggedTrain);
        ui.draggedTrain = -1;
        return;
    }
    if (ui.edit != Edit::None) {
        if (ui.edit == Edit::Replace && ui.moved) {
            auto *replacement = stationHandler.getStationAt(ui.mouse.x, ui.mouse.y);
            auto moved = ui.preview[ui.editIndex];
            if (replacement &&
                std::find(ui.preview.begin(), ui.preview.end(), replacement) == ui.preview.end())
                ui.preview[ui.editIndex] = replacement;
            else if (auto hit = lineHandler.segmentAt(ui.mouse, 20);
                     hit && hit->line == ui.editLine) {
                int insertion = hit->edge + 1;
                ui.preview.erase(ui.preview.begin() + ui.editIndex);
                if (ui.editIndex < insertion)
                    --insertion;
                ui.preview.insert(ui.preview.begin() + insertion, moved);
            }
        }
        if (ui.moved)
            commit(ui.editLine, ui.preview, ui.previewLoop);
        ui.edit = Edit::None;
        ui.preview.clear();
        return;
    }
    if (ui.tool != Tool::None && ui.moved) {
        bool onMenu = false;
        for (int i = 0; i < 4; ++i)
            if (UiLayout::resource(i).contains(ui.mouse))
                onMenu = true;
        if (!onMenu)
            applyTool();
    }
}
void GameEventsHandler::keyPressed(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Escape) {
        if (ui.help)
            ui.help = false;
        else if (ui.resetConfirm)
            ui.resetConfirm = false;
        else
            cancelInteraction();
        return;
    }
    if (ui.title) {
        if (key == sf::Keyboard::Key::Num1 || key == sf::Keyboard::Key::Num2) {
            simulation.reset(simulation.seed, key == sf::Keyboard::Key::Num1
                                                  ? World::MapId::River
                                                  : World::MapId::Canals);
            selectLine(0);
        }
        if (key == sf::Keyboard::Key::Enter)
            reset();
        return;
    }
    if (simulation.gameOver) {
        if (key == sf::Keyboard::Key::R || key == sf::Keyboard::Key::Enter)
            reset();
        return;
    }
    if (ui.resetConfirm) {
        if (key == sf::Keyboard::Key::Enter)
            reset();
        return;
    }
    if (key == sf::Keyboard::Key::H || key == sf::Keyboard::Key::F1) {
        cancelInteraction();
        ui.help = !ui.help;
        return;
    }
    if (ui.help)
        return;
    int number = -1;
    switch (key) {
    case sf::Keyboard::Key::Num1:
        number = 0;
        break;
    case sf::Keyboard::Key::Num2:
        number = 1;
        break;
    case sf::Keyboard::Key::Num3:
        number = 2;
        break;
    case sf::Keyboard::Key::Num4:
        number = 3;
        break;
    case sf::Keyboard::Key::Num5:
        number = 4;
        break;
    case sf::Keyboard::Key::Num6:
        number = 5;
        break;
    case sf::Keyboard::Key::Num7:
        number = 6;
        break;
    case sf::Keyboard::Key::Num8:
        number = 7;
        break;
    default:
        break;
    }
    if (simulation.choosingUpgrade) {
        if (number >= 0 && number < 4)
            simulation.chooseUpgrade(static_cast<Upgrade>(number));
        return;
    }
    if (ui.edit != Edit::None || ui.draggedTrain >= 0)
        cancelInteraction();
    if (number >= 0) {
        selectLine(number);
        return;
    }
    switch (key) {
    case sf::Keyboard::Key::Space:
        simulation.paused = !simulation.paused;
        break;
    case sf::Keyboard::Key::Tab:
        simulation.speed = simulation.speed == 1 ? 2 : simulation.speed == 2 ? 4 : 1;
        break;
    case sf::Keyboard::Key::Delete:
        commit(lineHandler.selected(), {}, false);
        break;
    case sf::Keyboard::Key::E:
        ui.tool = Tool::Engine;
        break;
    case sf::Keyboard::Key::W:
        ui.tool = Tool::Carriage;
        break;
    case sf::Keyboard::Key::I:
        ui.tool = Tool::Interchange;
        break;
    case sf::Keyboard::Key::R:
        cancelInteraction();
        ui.resetConfirm = true;
        break;
    default:
        break;
    }
}
void GameEventsHandler::update(float dt) {
    lineSelector.unlocked = lineHandler.resources.unlockedLines;
    if (ui.toastTime > 0)
        ui.toastTime -= dt;
    if (!ui.title && !ui.help && !ui.resetConfirm && ui.edit == Edit::None && ui.draggedTrain < 0)
        simulation.update(dt);
    else
        simulation.synchronize();
    ui.bestScore = std::max(ui.bestScore, simulation.score);
}
void GameEventsHandler::draw(sf::RenderTarget &target) {
    drawingHandler.draw(target, ui);
}
