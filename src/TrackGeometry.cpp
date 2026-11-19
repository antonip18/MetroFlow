#include "TrackGeometry.h"
#include "Curves.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace {
constexpr float Epsilon = .05f;
float dot(sf::Vector2f a, sf::Vector2f b) {
    return a.x * b.x + a.y * b.y;
}
struct Segment {
    int line, edge, parent;
    sf::Vector2f a, b, direction, normal;
    float lo, hi, baseline, offset = 0;
};
int root(std::vector<Segment> &s, int i) {
    while (s[i].parent != i) {
        s[i].parent = s[s[i].parent].parent;
        i = s[i].parent;
    }
    return i;
}
void append(std::vector<sf::Vector2f> &path, sf::Vector2f p) {
    if (path.empty() || length(path.back() - p) > .001f)
        path.push_back(p);
}
sf::Vector2f joint(const Segment &a, const Segment &b) {
    auto p = a.b + a.normal * a.offset, q = b.a + b.normal * b.offset;
    auto u = a.b - a.a, v = b.b - b.a;
    float cross = crossProduct(u, v);
    if (std::abs(cross) > .01f) {
        auto intersection = p + u * (crossProduct(q - p, v) / cross);
        // Bound the miter at acute reversals.
        if (length(intersection - a.b) < 90.f)
            return intersection;
    }
    return (p + q) * .5f;
}
} // namespace
TrackGeometry::TrackGeometry(const std::array<TrackRoute, 8> &routes) {
    std::vector<Segment> segments;
    std::array<std::vector<int>, 8> order;
    for (int l = 0; l < 8; ++l) {
        const auto &route = routes[l];
        if (route.stops.size() < 2)
            continue;
        std::size_t edges = route.stops.size() - 1 + (route.loop ? 1 : 0);
        lines[l].edges.resize(edges);
        for (std::size_t e = 0; e < edges; ++e) {
            auto p = getCurve(route.stops[e], route.stops[(e + 1) % route.stops.size()]);
            for (std::size_t j = 1; j < p.size(); ++j) {
                auto a = p[j - 1], b = p[j], u = b - a;
                float len = length(u);
                if (len < .001f)
                    continue;
                u /= len;
                if (u.x < -.001f || (std::abs(u.x) < .001f && u.y < 0))
                    u = -u;
                sf::Vector2f normal{-u.y, u.x};
                int id = static_cast<int>(segments.size());
                segments.push_back({l, static_cast<int>(e), id, a, b, u, normal,
                                    std::min(dot(a, u), dot(b, u)), std::max(dot(a, u), dot(b, u)),
                                    dot(a, normal), 0});
                order[l].push_back(id);
            }
        }
    }
    // Geometric overlap, including partial segments with different station IDs.
    for (int i = 0; i < static_cast<int>(segments.size()); ++i)
        for (int j = 0; j < i; ++j) {
            const auto &a = segments[i];
            const auto &b = segments[j];
            if (std::abs(crossProduct(a.direction, b.direction)) > .001f)
                continue;
            if (std::abs(a.baseline - b.baseline) > TrackStyle::Width + 1.f)
                continue;
            float overlap = std::min(a.hi, b.hi) - std::max(a.lo, b.lo);
            if (overlap > Epsilon || (a.line == b.line && overlap >= -Epsilon))
                segments[root(segments, i)].parent = root(segments, j);
        }
    std::vector<std::set<int>> colours(segments.size());
    for (int i = 0; i < static_cast<int>(segments.size()); ++i)
        colours[root(segments, i)].insert(segments[i].line);
    // Preserve the side ordering of shared lanes through mirrored corners.
    std::vector<std::vector<std::pair<int, int>>> neighbours(segments.size());
    for (int l = 0; l < 8; ++l) {
        const auto &ids = order[l];
        if (ids.empty())
            continue;
        std::size_t pairs = ids.size() - 1 + (routes[l].loop ? 1 : 0);
        for (std::size_t j = 0; j < pairs; ++j) {
            int a = ids[j], b = ids[(j + 1) % ids.size()], ra = root(segments, a),
                rb = root(segments, b);
            if (ra == rb)
                continue;
            int shared = 0;
            for (int c : colours[ra])
                if (colours[rb].count(c))
                    ++shared;
            if (shared < 2)
                continue;
            int sa = dot(segments[a].b - segments[a].a, segments[a].direction) > 0 ? 1 : -1;
            int sb = dot(segments[b].b - segments[b].a, segments[b].direction) > 0 ? 1 : -1;
            neighbours[ra].push_back({rb, sa * sb});
            neighbours[rb].push_back({ra, sa * sb});
        }
    }
    std::vector<int> orientation(segments.size(), 0);
    for (int i = 0; i < static_cast<int>(segments.size()); ++i)
        if (root(segments, i) == i && !orientation[i]) {
            orientation[i] = 1;
            std::vector<int> queue{i};
            for (std::size_t j = 0; j < queue.size(); ++j)
                for (auto [next, sign] : neighbours[queue[j]])
                    if (!orientation[next]) {
                        orientation[next] = orientation[queue[j]] * sign;
                        queue.push_back(next);
                    }
        }
    for (int group = 0; group < static_cast<int>(segments.size()); ++group) {
        if (root(segments, group) != group)
            continue;
        std::vector<int> members;
        std::vector<float> cuts;
        float center = 0;
        for (int i = 0; i < static_cast<int>(segments.size()); ++i)
            if (root(segments, i) == group) {
                members.push_back(i);
                center += segments[i].baseline;
                cuts.push_back(segments[i].lo);
                cuts.push_back(segments[i].hi);
            }
        center /= static_cast<float>(members.size());
        std::sort(cuts.begin(), cuts.end());
        // Count simultaneous colours, not the transitive union of a corridor.
        for (std::size_t j = 1; j < cuts.size(); ++j)
            if (cuts[j] - cuts[j - 1] > Epsilon) {
                float mid = (cuts[j] + cuts[j - 1]) * .5f;
                std::set<int> active;
                for (int i : members)
                    if (segments[i].lo < mid && segments[i].hi > mid)
                        active.insert(segments[i].line);
                maximumParallelLines =
                    std::max(maximumParallelLines, static_cast<int>(active.size()));
            }
        for (int i : members) {
            int slot = static_cast<int>(
                std::distance(colours[group].begin(), colours[group].find(segments[i].line)));
            float lane =
                (static_cast<float>(slot) - static_cast<float>(colours[group].size() - 1) * .5f) *
                TrackStyle::LaneSpacing * static_cast<float>(orientation[group]);
            segments[i].offset = center + lane - segments[i].baseline;
        }
    }
    for (int l = 0; l < 8; ++l) {
        const auto &ids = order[l];
        if (ids.empty())
            continue;
        std::vector<sf::Vector2f> joints(ids.size() + 1);
        const auto &first = segments[ids.front()];
        const auto &last = segments[ids.back()];
        joints.front() = first.a + first.normal * first.offset;
        joints.back() = last.b + last.normal * last.offset;
        for (std::size_t i = 1; i < ids.size(); ++i)
            joints[i] = joint(segments[ids[i - 1]], segments[ids[i]]);
        if (routes[l].loop)
            joints.front() = joints.back() = joint(last, first);
        for (std::size_t i = 0; i < ids.size(); ++i) {
            auto &edge = lines[l].edges[segments[ids[i]].edge];
            append(edge, joints[i]);
            append(edge, joints[i + 1]);
        }
        float distance = 0;
        for (const auto &edge : lines[l].edges) {
            lines[l].stopDistances.push_back(distance);
            for (auto p : edge)
                append(lines[l].path, p);
            distance += pathLength(edge);
        }
        if (!routes[l].loop)
            lines[l].stopDistances.push_back(distance);
        lines[l].totalLength = distance;
    }
}
TrackPose sampleTrack(const std::vector<sf::Vector2f> &path, float distance, bool loop) {
    if (path.size() < 2)
        return {path.empty() ? sf::Vector2f{} : path.front(), {1, 0}};
    float total = pathLength(path);
    if (total < .001f)
        return {path.front(), {1, 0}};
    if (loop) {
        distance = std::fmod(distance, total);
        if (distance < 0)
            distance += total;
    }
    for (std::size_t i = 1; i < path.size(); ++i) {
        auto v = path[i] - path[i - 1];
        float len = length(v);
        if (len < .001f)
            continue;
        if (distance <= len || i == path.size() - 1)
            return {path[i - 1] + v * (distance / len), v / len};
        distance -= len;
    }
    return {path.back(), {1, 0}};
}
