#include "LineHandler.h"
#include "Curves.h"
#include "WorldGeometry.h"
#include <algorithm>
#include <set>
LineHandler::LineHandler() {
    for (int i = 0; i < 8; ++i)
        lines.emplace_back(getLineColor(i));
}
void LineHandler::reset() {
    trackGeometry.reset();
    lines.clear();
    for (int i = 0; i < 8; ++i)
        lines.emplace_back(getLineColor(i));
    resources = {};
    currentLine = 0;
    message.clear();
}
void LineHandler::setCurrentLine(LineColor c) {
    int i = getLineIndex(c);
    if (i < resources.unlockedLines)
        currentLine = i;
}
int LineHandler::routeTunnels(const std::vector<Station *> &r, bool loop) const {
    if (r.size() < 2)
        return 0;
    int n = 0;
    for (std::size_t i = 0; i < r.size() - 1 + (loop ? 1 : 0); ++i)
        n += World::crossings(getCurve(r[i]->getPosition(), r[(i + 1) % r.size()]->getPosition()),
                              map);
    return n;
}
int LineHandler::tunnelUsage() const {
    int n = 0;
    for (const auto &l : lines)
        n += routeTunnels(l.getStations(), l.isLoop());
    return n;
}
bool LineHandler::validate(int index, const std::vector<Station *> &r, bool loop,
                           std::string &reason) const {
    auto fail = [&](const char *s) {
        reason = s;
        return false;
    };
    if (index < 0 || index >= resources.unlockedLines)
        return fail("This line is still locked.");
    if (r.size() == 1)
        return fail("Connect at least two stations.");
    if (loop && r.size() < 3)
        return fail("A loop needs at least three stations.");
    std::set<int> seen;
    for (auto *s : r)
        if (!s || !seen.insert(s->getId()).second)
            return fail("A line cannot branch or visit a station twice.");
    TrackGeometry candidate(routes(index, r, loop));
    if (candidate.maximumParallelLines > 4)
        return fail("At most four lines may run alongside one another on a shared track section.");
    int used = tunnelUsage() - routeTunnels(lines[index].getStations(), lines[index].isLoop()) +
               routeTunnels(r, loop);
    if (used > resources.tunnels)
        return fail("Not enough tunnels. Shorten a river crossing or earn an upgrade.");
    reason.clear();
    return true;
}
bool LineHandler::setRoute(int index, const std::vector<Station *> &r, bool loop) {
    if (!validate(index, r, loop, message))
        return false;
    auto &l = lines[index];
    if (l.stations == r && l.loop == loop)
        return true;
    l.stations = r;
    l.loop = loop;
    ++l.version;
    trackGeometry.reset();
    return true;
}
std::array<TrackRoute, 8> LineHandler::routes(int replacement, const std::vector<Station *> &route,
                                              bool loop) const {
    std::array<TrackRoute, 8> result;
    for (int i = 0; i < 8; ++i) {
        const auto &stops = i == replacement ? route : lines[i].getStations();
        result[i].loop = i == replacement ? loop : lines[i].isLoop();
        for (auto *station : stops)
            result[i].stops.push_back(station->getPosition());
    }
    return result;
}
const RouteGeometry &LineHandler::geometry(int index) const {
    if (!trackGeometry)
        trackGeometry = std::make_unique<TrackGeometry>(routes());
    return trackGeometry->lines.at(static_cast<std::size_t>(index));
}
std::vector<sf::Vector2f> LineHandler::edgePath(int index, int edgeIndex, bool offset) const {
    if (offset)
        return geometry(index).edges.at(static_cast<std::size_t>(edgeIndex));
    auto [a, b] = lines.at(index).edge(static_cast<std::size_t>(edgeIndex));
    return getCurve(a->getPosition(), b->getPosition());
}
std::array<sf::Vector2f, 3> LineHandler::handle(int index, bool start, float clearance) const {
    const auto &g = geometry(index);
    if (g.path.size() < 2)
        return {};
    auto pose = sampleTrack(g.path, start ? 0 : g.totalLength, false);
    auto direction = pose.tangent * (start ? -1.f : 1.f);
    auto center = pose.position + direction * std::max(TrackStyle::HandleLength, clearance);
    int rank = 0;
    for (int i = 0; i < index; ++i)
        if (lines[i].active() && !lines[i].isLoop()) {
            const auto &other = geometry(i);
            for (int end = 0; end < 2; ++end) {
                auto p = sampleTrack(other.path, end == 0 ? 0 : other.totalLength, false);
                auto d = p.tangent * (end == 0 ? -1.f : 1.f);
                if (length(d - direction) < .01f && length(p.position - pose.position) < 45.f)
                    ++rank;
            }
        }
    center += direction * static_cast<float>(rank) * TrackStyle::Width * 1.3f;
    sf::Vector2f normal{-direction.y, direction.x};
    return {pose.position, center - normal * TrackStyle::HandleHalfWidth,
            center + normal * TrackStyle::HandleHalfWidth};
}
std::optional<SegmentHit> LineHandler::segmentAt(sf::Vector2f p, float tolerance) const {
    std::optional<SegmentHit> hit;
    float best = tolerance;
    // Selected line wins exact ties on shared track.
    for (int step = 0; step < 8; ++step) {
        int i = (currentLine + step) % 8;
        for (std::size_t e = 0; e < lines[i].edgeCount(); ++e) {
            float d = distanceToPath(p, edgePath(i, static_cast<int>(e)));
            if (d < best) {
                best = d;
                hit = SegmentHit{i, static_cast<int>(e)};
            }
        }
    }
    return hit;
}
