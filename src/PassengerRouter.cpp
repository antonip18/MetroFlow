#include "PassengerRouter.h"
#include "Curves.h"
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
namespace {
constexpr float Infinity = 1e12f;
constexpr int Layers = 9;
} // namespace
void PassengerRouter::rebuild(const StationHandler &stations, const LineHandler &lines,
                              const std::array<bool, 8> &served) {
    const int n = static_cast<int>(stations.getStations().size()) * Layers;
    graph.assign(static_cast<std::size_t>(n), {});
    auto add = [&](int a, int b, float c) { graph[a].push_back({b, c}); };
    for (int l = 0; l < 8; ++l)
        if (served[l]) {
            const auto &line = lines.at(l);
            for (auto *s : line.getStations()) {
                add(s->getId() * Layers + 8, s->getId() * Layers + l, 0.f);
                add(s->getId() * Layers + l, s->getId() * Layers + 8,
                    2.5f); // discourage unnecessary transfers
            }
            for (std::size_t e = 0; e < line.edgeCount(); ++e) {
                auto [a, b] = line.edge(e);
                float cost =
                    pathLength(getCurve(a->getPosition(), b->getPosition())) / Speed + Dwell;
                add(a->getId() * Layers + l, b->getId() * Layers + l, cost);
                if (!line.isLoop())
                    add(b->getId() * Layers + l, a->getId() * Layers + l, cost);
            }
        }
    std::vector<std::vector<Edge>> reverse(static_cast<std::size_t>(n));
    for (int u = 0; u < n; ++u)
        for (auto e : graph[u])
            reverse[e.to].push_back({u, e.cost});
    for (int sh = 0; sh < ShapeCount; ++sh) {
        auto &d = distances[sh];
        d.assign(static_cast<std::size_t>(n), Infinity);
        using Item = std::pair<float, int>;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
        for (const auto &s : stations.getStations())
            if (static_cast<int>(s.getShape()) == sh)
                for (int l = 0; l < Layers; ++l) {
                    int u = s.getId() * Layers + l;
                    d[u] = 0;
                    queue.emplace(0, u);
                }
        while (!queue.empty()) {
            auto [cost, u] = queue.top();
            queue.pop();
            if (cost > d[u] + .0001f)
                continue;
            for (auto e : reverse[u])
                if (cost + e.cost < d[e.to]) {
                    d[e.to] = cost + e.cost;
                    queue.emplace(d[e.to], e.to);
                }
        }
    }
}
float PassengerRouter::distance(int station, Shape shape, int line) const {
    const auto &d = distances[static_cast<int>(shape)];
    int u = station * Layers + line;
    return u >= 0 && u < static_cast<int>(d.size()) ? d[u] : Infinity;
}
bool PassengerRouter::optimalEdge(int from, int to, Shape shape, int line, int current) const {
    float best = distance(from, shape, current);
    if (best >= Infinity / 2)
        return false;
    int u = from * Layers + line;
    if (u < 0 || u >= static_cast<int>(graph.size()))
        return false;
    for (auto e : graph[u])
        if (e.to == to * Layers + line)
            return e.cost + distance(to, shape, line) <= best + .01f;
    return false;
}
bool PassengerRouter::board(int a, int b, Shape s, int l) const {
    return optimalEdge(a, b, s, l, 8);
}
bool PassengerRouter::stay(int a, int b, Shape s, int l) const {
    return optimalEdge(a, b, s, l, l);
}
