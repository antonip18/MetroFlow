#pragma once
#include "LineHandler.h"
#include "StationHandler.h"
#include <array>
class PassengerRouter {
  public:
    static constexpr float Speed = 100.f;
    static constexpr float Dwell = .7f;
    void rebuild(const StationHandler &stations, const LineHandler &lines,
                 const std::array<bool, 8> &served);
    bool board(int from, int to, Shape shape, int line) const;
    bool stay(int from, int to, Shape shape, int line) const;
    float distance(int station, Shape shape, int line = 8) const;

  private:
    struct Edge {
        int to;
        float cost;
    };
    std::vector<std::vector<Edge>> graph;
    std::array<std::vector<float>, ShapeCount> distances;
    bool optimalEdge(int from, int to, Shape shape, int line, int current) const;
};
