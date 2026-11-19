#pragma once
#include "Station.h"
#include <string>
#include <vector>
enum class Tool { None, Engine, Carriage, Interchange };
enum class Edit { None, Start, End, Insert, Replace };
struct UiState {
    bool title = true, help = false, resetConfirm = false;
    sf::Vector2f mouse{};
    Tool tool = Tool::None;
    Edit edit = Edit::None;
    int editLine = -1, editIndex = -1, draggedTrain = -1, lastStation = -1;
    std::vector<Station *> preview;
    bool previewLoop = false, moved = false, fromHandle = false;
    sf::Vector2f press{};
    std::string toast;
    float toastTime = 0;
    int bestScore = 0;
};
namespace UiLayout {
inline sf::FloatRect maps() {
    return {{48, 98}, {295, 40}};
}
inline sf::FloatRect mapCard(int i) {
    return {{410.f + 315.f * static_cast<float>(i), 365}, {295, 133}};
}
inline sf::FloatRect resource(int i) {
    return {{48.f + 146.f * static_cast<float>(i), 806.f}, {134.f, 52.f}};
}
inline sf::FloatRect pause() {
    return {{1100, 98}, {76, 40}};
}
inline sf::FloatRect speed() {
    return {{1186, 98}, {76, 40}};
}
inline sf::FloatRect help() {
    return {{1272, 98}, {116, 40}};
}
inline sf::FloatRect removeLine() {
    return {{1308, 810}, {80, 44}};
}
inline sf::FloatRect upgrade(int i) {
    return {{250.f + 240.f * static_cast<float>(i), 438}, {220, 150}};
}
inline sf::FloatRect play() {
    return {{520, 533}, {400, 64}};
}
inline sf::FloatRect secondary() {
    return {{520, 617}, {400, 44}};
}
} // namespace UiLayout
