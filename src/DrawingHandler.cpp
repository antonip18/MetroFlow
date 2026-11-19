#include "DrawingHandler.h"
#include "Curves.h"
#include "ShapeDrawer.h"
#include "WorldGeometry.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace {
const sf::Color Paper{246, 245, 237}, Ink{35, 58, 52}, Muted{111, 128, 117}, Border{215, 220, 210},
    Water{193, 219, 222}, Accent{225, 100, 68};
void box(sf::RenderTarget &t, sf::FloatRect r, sf::Color fill,
         sf::Color border = sf::Color::Transparent, float outline = 0) {
    sf::RectangleShape s(r.size);
    s.setPosition(r.position);
    s.setFillColor(fill);
    s.setOutlineColor(border);
    s.setOutlineThickness(outline);
    t.draw(s);
}
void circle(sf::RenderTarget &t, sf::Vector2f p, float r, sf::Color c) {
    sf::CircleShape s(r, 48);
    s.setOrigin({r, r});
    s.setPosition(p);
    s.setFillColor(c);
    t.draw(s);
}
void ring(sf::RenderTarget &t, sf::Vector2f p, float r, float fraction, sf::Color c, float width) {
    std::vector<sf::Vector2f> points;
    int count = std::max(1, static_cast<int>(fraction * 80));
    for (int i = 0; i <= count; ++i) {
        float a =
            -1.5707963f + 6.2831853f * fraction * static_cast<float>(i) / static_cast<float>(count);
        points.push_back(p + sf::Vector2f{r * std::cos(a), r * std::sin(a)});
    }
    stroke(t, points, c, width);
}
} // namespace
DrawingHandler::DrawingHandler(StationHandler &s, LineHandler &l, LineSelector &selector,
                               Simulation &sim, const std::filesystem::path &fontPath)
    : stationHandler(s), lineHandler(l), lineSelector(selector), simulation(sim) {
    if (!font.openFromFile(fontPath))
        throw std::runtime_error("Cannot load font: " + fontPath.string() +
                                 ". Keep assets next to the executable.");
}
void DrawingHandler::text(sf::RenderTarget &t, const std::string &value, sf::Vector2f p,
                          unsigned size, sf::Color color, bool bold) const {
    sf::Text label(font, value, size);
    label.setPosition(p);
    label.setFillColor(color);
    if (bold)
        label.setStyle(sf::Text::Bold);
    t.draw(label);
}
void DrawingHandler::centeredText(sf::RenderTarget &t, const std::string &value, sf::FloatRect area,
                                  unsigned size, sf::Color color, bool bold) const {
    sf::Text label(font, value, size);
    label.setFillColor(color);
    if (bold)
        label.setStyle(sf::Text::Bold);
    // Font bearings are not zero: centre the actual glyph bounds, not the font size.
    const auto bounds = label.getLocalBounds();
    auto position = area.position + (area.size - bounds.size) * .5f - bounds.position;
    // Whole-pixel placement keeps the font crisp (at most half a pixel off-centre).
    label.setPosition({std::round(position.x), std::round(position.y)});
    t.draw(label);
}
void DrawingHandler::map(sf::RenderTarget &t) const {
    box(t, {{0, 0}, {1440, 900}}, Paper);
    for (float x = 64; x < 1400; x += 32)
        for (float y = 180; y < 740; y += 32)
            circle(t, {x, y}, .9f, sf::Color(220, 225, 215));
    for (const auto &river : World::waterways(lineHandler.map)) {
        stroke(t, river, sf::Color(222, 235, 229), World::riverWidth(lineHandler.map) + 16);
        stroke(t, river, Water, World::riverWidth(lineHandler.map));
        auto shine = river;
        for (auto &point : shine) {
            if (lineHandler.map == World::MapId::River)
                point.x -= 10;
            else
                point.y -= 8;
        }
        stroke(t, shine, sf::Color(215, 233, 232), 2);
    }
    if (lineHandler.map == World::MapId::River) {
        text(t, "WEST BANK", {65, 185}, 11, Muted, true);
        text(t, "EAST BANK", {1228, 185}, 11, Muted, true);
        text(t, "R I V E R   A V O N", {735, 710}, 10, sf::Color(104, 144, 148));
    } else {
        text(t, "NORTH QUARTER", {65, 185}, 11, Muted, true);
        text(t, "CANAL ISLANDS", {1210, 185}, 11, Muted, true);
        text(t, "SOUTH QUARTER", {65, 712}, 11, Muted, true);
    }
}
void DrawingHandler::network(sf::RenderTarget &t, const UiState &ui) const {
    // Paint all bridge underlays first, so they cannot erase a neighbouring lane.
    for (int i = 0; i < 8; ++i)
        for (std::size_t e = 0; e < lineHandler.at(i).edgeCount(); ++e)
            if (World::crossings(lineHandler.edgePath(i, static_cast<int>(e), false),
                                 lineHandler.map) > 0)
                stroke(t, lineHandler.edgePath(i, static_cast<int>(e)), Paper,
                       TrackStyle::Width + 4);
    for (int i = 0; i < 8; ++i) {
        const auto &line = lineHandler.at(i);
        for (std::size_t e = 0; e < line.edgeCount(); ++e) {
            auto color = getSFMLColor(line.getLineColor());
            if (ui.editLine == i && ui.edit != Edit::None)
                color.a = 80;
            stroke(t, lineHandler.edgePath(i, static_cast<int>(e)), color, TrackStyle::Width);
        }
        if (line.active() && !line.isLoop())
            for (bool start : {true, false}) {
                auto h = simulation.endpointHandle(i, start);
                auto center = (h[1] + h[2]) * .5f;
                auto color = getSFMLColor(line.getLineColor());
                stroke(t, {h[0], center}, color, TrackStyle::Width);
                stroke(t, {h[1], h[2]}, color, TrackStyle::HandleThickness);
            }
    }
    if (ui.edit != Edit::None && !ui.preview.empty()) {
        std::string reason;
        bool valid = ui.preview.size() < 2 ||
                     lineHandler.validate(ui.editLine, ui.preview, ui.previewLoop, reason);
        auto color = valid ? getSFMLColor(getLineColor(ui.editLine)) : sf::Color(205, 53, 54);
        color.a = 210;
        for (std::size_t i = 1; i < ui.preview.size(); ++i)
            stroke(t, getCurve(ui.preview[i - 1]->getPosition(), ui.preview[i]->getPosition()),
                   color, 8);
        if (ui.previewLoop)
            stroke(t, getCurve(ui.preview.back()->getPosition(), ui.preview.front()->getPosition()),
                   color, 8);
        else if (ui.edit == Edit::Start || ui.edit == Edit::End)
            stroke(t,
                   getCurve((ui.edit == Edit::Start ? ui.preview.front() : ui.preview.back())
                                ->getPosition(),
                            ui.mouse),
                   color, 4);
        if (ui.edit == Edit::Insert) {
            auto a = ui.preview[ui.editIndex]->getPosition(),
                 b = ui.preview[(ui.editIndex + 1) % ui.preview.size()]->getPosition();
            stroke(t, getCurve(a, ui.mouse), color, 4);
            stroke(t, getCurve(ui.mouse, b), color, 4);
        }
        if (ui.edit == Edit::Replace) {
            circle(t, ui.mouse, 22, sf::Color(color.r, color.g, color.b, 55));
            stroke(t, {ui.preview[ui.editIndex]->getPosition(), ui.mouse}, color, 2);
        }
    }
    for (const auto &s : stationHandler.getStations()) {
        const auto p = s.getPosition();
        if (s.interchange) {
            circle(t, p, 25, Paper);
            ring(t, p, 25, 1, Ink, 2);
        }
        if (s.overcrowding > 0) {
            ring(t, p, 31, 1, sf::Color(233, 219, 205), 5);
            ring(t, p, 31, std::min(1.f, s.overcrowding / Simulation::OvercrowdingLimit),
                 sf::Color(214, 78, 64), 5);
        }
        s.draw(t);
        text(t, std::to_string(s.getId() + 1), p + sf::Vector2f{-5, 24}, 10, Muted);
        int n = 0;
        for (const auto &passenger : s.passengers) {
            if (n >= 18)
                break;
            float dx = 31.f + static_cast<float>(n % 6) * 12.f,
                  dy = -17.f + static_cast<float>(n / 6) * 13.f;
            symbol(t, passenger.getDestination(), p + sf::Vector2f{dx, dy}, 3.6f, Ink, Ink, .6f);
            ++n;
        }
        if (s.passengers.size() > 18)
            text(t, "+" + std::to_string(s.passengers.size() - 18), p + sf::Vector2f{32, 24}, 11,
                 Accent, true);
    }
    for (const auto &train : simulation.getTrains()) {
        auto p = visualTrainPosition(train, lineHandler);
        auto poses = trainPoses(train, lineHandler);
        if (poses.size() != static_cast<std::size_t>(train.carriages + 1))
            continue;
        for (int car = train.carriages; car >= 0; --car) {
            const auto &pose = poses[static_cast<std::size_t>(car)];
            auto position = pose.position, direction = pose.tangent;
            float angle = std::atan2(direction.y, direction.x) * 180.f / 3.14159265f;
            sf::RectangleShape body({TrackStyle::CarLength, TrackStyle::CarWidth});
            body.setOrigin({TrackStyle::CarLength * .5f, TrackStyle::CarWidth * .5f});
            body.setPosition(position);
            body.setRotation(sf::degrees(angle));
            body.setFillColor(getSFMLColor(getLineColor(train.line)));
            body.setOutlineColor(Paper);
            body.setOutlineThickness(1.5f);
            t.draw(body);
            sf::RectangleShape cabin({16, 7});
            cabin.setOrigin({8, 3.5f});
            cabin.setPosition(position);
            cabin.setRotation(sf::degrees(angle));
            cabin.setFillColor(Paper);
            t.draw(cabin);
            for (int j = 0; j < 6; ++j)
                if (car * 6 + j < static_cast<int>(train.passengers.size()))
                    circle(t,
                           position + direction * (static_cast<float>(j % 3) - 1) * 5.f +
                               getNormal({0, 0}, direction) * (j < 3 ? -2.f : 2.f),
                           1.5f, Ink);
        }
        if (ui.draggedTrain == train.id) {
            ring(t, p, 24, 1, Ink, 2);
            stroke(t, {p, ui.mouse}, sf::Color(60, 90, 75, 100), 2);
        }
    }
    if (!ui.title && !ui.help && !simulation.gameOver && ui.edit == Edit::None &&
        ui.tool == Tool::None) {
        if (auto *s = stationHandler.getStationAt(ui.mouse.x, ui.mouse.y)) {
            auto p = s->getPosition();
            float x = std::clamp(p.x - 90.f, 50.f, 1180.f);
            box(t, {{x, p.y - 76}, {205, 36}}, Ink);
            centeredText(t,
                         "STATION " + std::to_string(s->getId() + 1) + "   " +
                             std::to_string(s->passengers.size()) + " / " +
                             std::to_string(s->capacity()),
                         {{x, p.y - 76}, {205, 36}}, 12, Paper, true);
        }
    }
    if (ui.tool != Tool::None) {
        circle(t, ui.mouse, 20, sf::Color(35, 58, 52, 170));
        centeredText(t,
                     ui.tool == Tool::Engine     ? "E"
                     : ui.tool == Tool::Carriage ? "W"
                                                 : "I",
                     {ui.mouse - sf::Vector2f{20, 20}, {40, 40}}, 17, Paper, true);
    }
}
void DrawingHandler::hud(sf::RenderTarget &t, const UiState &ui) const {
    box(t, {{0, 0}, {1440, 157}}, Paper);
    box(t, {{48, 154}, {1340, 1}}, Border);
    circle(t, {62, 48}, 13, Ink);
    circle(t, {62, 48}, 5, Paper);
    stroke(t, {{62, 61}, {62, 78}, {81, 78}}, Ink, 4);
    text(t, "METROFLOW", {91, 24}, 32, Ink, true);
    text(t, "A CITY IN MOTION", {94, 67}, 11, Muted, true);
    box(t, UiLayout::maps(), Paper, Border, 1);
    centeredText(t, std::string(World::mapName(lineHandler.map)) + "  /  MAPS", UiLayout::maps(),
                 13, Muted, true);
    text(t, "WEEK " + std::to_string(simulation.week), {463, 30}, 13, Muted, true);
    static const char *days[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
    float phase =
        std::fmod(simulation.elapsed, Simulation::WeekDuration) / Simulation::WeekDuration;
    text(t, days[std::min(6, static_cast<int>(phase * 7))], {463, 51}, 25, Ink, true);
    box(t, {{463, 94}, {181, 4}}, Border);
    box(t, {{463, 94}, {181 * phase, 4}}, Ink);
    text(t, "Next week: locomotive + upgrade", {463, 110}, 11, Muted);
    text(t, "DELIVERED", {744, 30}, 13, Muted, true);
    text(t, std::to_string(simulation.score), {744, 49}, 34, Ink, true);
    text(t, "BEST  " + std::to_string(ui.bestScore), {920, 64}, 12, Muted);
    text(t, simulation.paused ? "PLANNING MODE" : "NETWORK LIVE", {1100, 37}, 13,
         simulation.paused ? Accent : Ink, true);
    auto button = [&](sf::FloatRect r, const std::string &label, bool active) {
        bool hovered = r.contains(ui.mouse);
        box(t, r, active ? Ink : hovered ? sf::Color(232, 235, 226) : Paper, Border, 1);
        centeredText(t, label, r, 13, active ? Paper : Ink, true);
    };
    button(UiLayout::pause(), simulation.paused ? "PLAY" : "PAUSE", simulation.paused);
    button(UiLayout::speed(), std::to_string(simulation.speed) + "x", false);
    button(UiLayout::help(), "?  GUIDE", false);
    box(t, {{32, 773}, {1376, 96}}, sf::Color(239, 241, 232), Border, 1);
    text(t, "RESERVES", {48, 782}, 11, Muted, true);
    text(t, "Drag to deploy", {145, 782}, 11, Muted);
    std::string labels[] = {"ENGINES", "CARRIAGES", "TUNNELS", "HUBS"};
    int counts[] = {simulation.freeEngines(), simulation.freeCarriages(),
                    lineHandler.resources.tunnels - lineHandler.tunnelUsage(),
                    simulation.freeInterchanges()};
    for (int i = 0; i < 4; ++i) {
        auto r = UiLayout::resource(i);
        bool active = (i == 0 && ui.tool == Tool::Engine) ||
                      (i == 1 && ui.tool == Tool::Carriage) ||
                      (i == 3 && ui.tool == Tool::Interchange);
        box(t, r, active ? Ink : Paper, Border, 1);
        auto color = active ? Paper : Ink;
        centeredText(t, std::to_string(counts[i]), {r.position, {42, r.size.y}}, 25, color, true);
        centeredText(t, labels[i], {r.position + sf::Vector2f{42, 0}, {r.size.x - 42, r.size.y}},
                     10, color, true);
    }
    box(t, {{654, 793}, {1, 64}}, Border);
    text(t, "LINES  /  " + std::to_string(lineHandler.resources.unlockedLines) + " OF 8",
         {746, 782}, 11, Muted, true);
    lineSelector.draw(t);
    for (int i = 0; i < 8; ++i) {
        auto p = lineSelector.getCirclePosition(i);
        centeredText(t, std::to_string(i + 1), {p - sf::Vector2f{18, 18}, {36, 36}}, 13,
                     i < lineHandler.resources.unlockedLines ? Paper : Muted, true);
        bool has = false;
        for (const auto &train : simulation.getTrains())
            if (train.line == i)
                has = true;
        if (lineHandler.at(i).active() && !has)
            circle(t, p + sf::Vector2f{0, 25}, 3, Accent);
    }
    button(UiLayout::removeLine(), "CLEAR", false);
    std::string status =
        "Drag stations to connect. Drag track to insert a stop. Right-click to remove.";
    if (ui.tool == Tool::Engine)
        status = "ENGINE  /  Drop on a route. Drag an existing train to move it; drop on ENGINES "
                 "to reclaim.";
    if (ui.tool == Tool::Carriage)
        status = "CARRIAGE  /  Drop onto a locomotive. Each carriage adds 6 seats. Shift + "
                 "right-click returns one.";
    if (ui.tool == Tool::Interchange)
        status = "INTERCHANGE  /  Drop onto a station to increase waiting capacity from 8 to 20.";
    if (ui.edit != Edit::None)
        status = "EDITING  /  Release to apply. Escape to cancel. The simulation is held while "
                 "dragging.";
    if (ui.toastTime > 0)
        status = ui.toast;
    text(t, status, {48, 743}, 13, ui.toastTime > 0 ? Accent : Muted);
    text(t, "SPACE  pause     TAB  speed     1-8  select line     H  guide     R  restart",
         {48, 880}, 11, Muted);
    text(t, "SEED " + std::to_string(simulation.seed), {1281, 880}, 11, Muted);
}
void DrawingHandler::modal(sf::RenderTarget &t, const UiState &ui) const {
    if (!ui.title && !ui.help && !ui.resetConfirm && !simulation.choosingUpgrade &&
        !simulation.gameOver)
        return;
    box(t, {{0, 0}, {1440, 900}}, sf::Color(24, 43, 38, 100));
    auto primary = [&](const std::string &value) {
        box(t, UiLayout::play(), Ink);
        centeredText(t, value, UiLayout::play(), 22, Paper, true);
    };
    if (ui.title) {
        box(t, {{350, 216}, {740, 421}}, Paper, Border, 1);
        text(t, "METROFLOW   /   TWO CITIES TO CONNECT", {410, 250}, 12, Muted, true);
        text(t, "Choose your city.", {410, 281}, 39, Ink, true);
        text(t, "Different waterways. Different connections.", {410, 332}, 16, Muted);
        for (int i = 0; i < 2; ++i) {
            auto r = UiLayout::mapCard(i);
            auto map = static_cast<World::MapId>(i);
            bool selected = lineHandler.map == map;
            box(t, r, selected ? sf::Color(232, 237, 224) : Paper, selected ? Ink : Border,
                selected ? 2.f : 1.f);
            auto miniature = [&](sf::Vector2f p) {
                return r.position + sf::Vector2f{14 + (p.x - 48) * .185f, 8 + (p.y - 155) * .105f};
            };
            for (const auto &water : World::waterways(map)) {
                std::vector<sf::Vector2f> points;
                for (auto p : water)
                    points.push_back(miniature(p));
                stroke(t, points, Water, 6);
            }
            for (auto p : World::initialStations(map))
                circle(t, miniature(p), 2.7f, Ink);
            centeredText(t, std::to_string(i + 1) + "  " + World::mapName(map),
                         {r.position + sf::Vector2f{0, 83}, {r.size.x, 25}}, 14, Ink, true);
            centeredText(t, i == 0 ? "A river between two banks." : "Two canals. Three districts.",
                         {r.position + sf::Vector2f{0, 109}, {r.size.x, 18}}, 11, Muted);
        }
        primary("START A NEW NETWORK");
    } else if (ui.resetConfirm) {
        box(t, {{350, 270}, {740, 430}}, Paper);
        text(t, "Start a fresh network?", {405, 321}, 35, Ink, true);
        text(t, "Your current network will be cleared. Your best score stays.", {405, 397}, 17,
             Muted);
        primary("RESTART  /  ENTER");
        centeredText(t, "Keep building  /  Escape", UiLayout::secondary(), 16, Ink);
    } else if (ui.help) {
        box(t, {{230, 190}, {980, 530}}, Paper);
        text(t, "THE NETWORK GUIDE", {275, 226}, 31, Ink, true);
        const std::pair<const char *, const char *> rows[] = {
            {"BUILD", "Select 1-8 or a colour. Drag from a station to another station."},
            {"EXTEND / TRIM",
             "Drag a T-bar outwards to extend; over its terminal stop to remove it."},
            {"INSERT / MOVE",
             "Drag track over new stops. Drag an interior stop onto track to reorder."},
            {"LOOP / REMOVE", "Join both ends. Right-click track to open a loop or trim a route."},
            {"RESOURCES",
             "Drag reserves onto routes, trains or stations. Drag trains to reassign."},
            {"RECOVER",
             "Right-click a train to return it; Shift + right-click returns one carriage."},
            {"PASSENGERS", "Symbols indicate destinations. Riders transfer between served lines."},
            {"PRESSURE", "A red ring fills above capacity. Resolve it within 28 game seconds."},
            {"TIME", "Space: pause. Tab: 1x / 2x / 4x. Weekly upgrades pause time."}};
        for (int i = 0; i < 9; ++i) {
            float y = 294.f + static_cast<float>(i) * 38.f;
            text(t, rows[i].first, {275, y}, 12, Ink, true);
            text(t, rows[i].second, {455, y}, 13, Muted);
        }
        text(t, "Click anywhere or press H / Escape to return. Time is paused here.", {275, 671},
             14, Accent);
    } else if (simulation.gameOver) {
        box(t, {{350, 230}, {740, 470}}, Paper);
        text(t, "SERVICE INTERRUPTED", {410, 274}, 13, Accent, true);
        text(t, "A city worth connecting.", {410, 315}, 35, Ink, true);
        text(t, std::to_string(simulation.score) + " passengers delivered", {410, 383}, 30, Ink,
             true);
        text(t,
             "Station " + std::to_string(simulation.failedStation + 1) +
                 " stayed overcrowded for too long.",
             {410, 446}, 17, Muted);
        text(t,
             "Week " + std::to_string(simulation.week) + "   /   Best " +
                 std::to_string(ui.bestScore),
             {410, 479}, 15, Muted);
        primary("BUILD AGAIN");
    } else if (simulation.choosingUpgrade) {
        box(t, {{210, 245}, {1020, 402}}, Paper);
        text(t, "WEEK " + std::to_string(simulation.week) + "  /  CITY INVESTMENT", {250, 281}, 13,
             Muted, true);
        text(t, "Room to grow.", {250, 317}, 40, Ink, true);
        text(t, "One locomotive has arrived. Choose one more upgrade.", {250, 379}, 17, Muted);
        const char *names[] = {"A NEW LINE", "A CARRIAGE", "TWO TUNNELS", "AN INTERCHANGE"};
        const char *desc[] = {"Connect another route", "Add 6 seats to a train",
                              "Cross the river twice", "Raise capacity to 20"};
        for (int i = 0; i < 4; ++i) {
            auto r = UiLayout::upgrade(i);
            bool locked = i == 0 && lineHandler.resources.unlockedLines >= 8;
            box(t, r, r.contains(ui.mouse) && !locked ? sf::Color(226, 233, 219) : Paper, Border,
                1);
            centeredText(t, "0" + std::to_string(i + 1),
                         {r.position + sf::Vector2f{0, 16}, {r.size.x, 40}}, 24,
                         locked ? Muted : Accent, true);
            centeredText(t, names[i], {r.position + sf::Vector2f{0, 68}, {r.size.x, 30}}, 14, Ink,
                         true);
            centeredText(t, locked ? "All lines unlocked" : desc[i],
                         {r.position + sf::Vector2f{0, 104}, {r.size.x, 26}}, 12, Muted);
        }
        text(t, "Choose a card or press 1-4. Your network is paused.", {250, 612}, 12, Muted);
    }
}
void DrawingHandler::draw(sf::RenderTarget &target, const UiState &ui) const {
    map(target);
    network(target, ui);
    hud(target, ui);
    modal(target, ui);
}
