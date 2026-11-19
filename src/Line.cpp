#include "Line.h"
Line::Line(LineColor lineColor) : lineColor(lineColor) {}
LineColor Line::getLineColor() const {
    return lineColor;
}
bool Line::isEmpty() const {
    return stations.empty();
}
std::size_t Line::edgeCount() const {
    return stations.size() < 2 ? 0 : stations.size() - 1 + (loop ? 1 : 0);
}
std::pair<Station *, Station *> Line::edge(std::size_t i) const {
    return {stations.at(i), stations.at((i + 1) % stations.size())};
}
int Line::indexOf(int id) const {
    for (std::size_t i = 0; i < stations.size(); ++i)
        if (stations[i]->getId() == id)
            return static_cast<int>(i);
    return -1;
}
