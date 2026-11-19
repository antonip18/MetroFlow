//
// Created by Antoni on 25.09.2026.
//

#include "LineColor.h"

sf::Color getSFMLColor(LineColor lineColor) {
    switch (lineColor) {
    case LineColor::RED:
        return sf::Color(237, 28, 36);
    case LineColor::BLUE:
        return sf::Color(0, 114, 188);
    case LineColor::GREEN:
        return sf::Color(0, 166, 81);
    case LineColor::YELLOW:
        return sf::Color(255, 194, 14);
    case LineColor::PURPLE:
        return sf::Color(146, 39, 143);
    case LineColor::CYAN:
        return sf::Color(0, 174, 239);
    case LineColor::ORANGE:
        return sf::Color(255, 127, 39);
    case LineColor::DARK_RED:
        return sf::Color(136, 0, 21);
    }
    return sf::Color::White;
}

LineColor getLineColor(int lineNumber) {
    switch (lineNumber) {
    case 0:
        return LineColor::RED;
    case 1:
        return LineColor::BLUE;
    case 2:
        return LineColor::GREEN;
    case 3:
        return LineColor::YELLOW;
    case 4:
        return LineColor::PURPLE;
    case 5:
        return LineColor::CYAN;
    case 6:
        return LineColor::ORANGE;
    case 7:
        return LineColor::DARK_RED;
    }
    return LineColor::DARK_RED;
}

int getLineIndex(LineColor lineColor) {
    switch (lineColor) {
    case LineColor::RED:
        return 0;
    case LineColor::BLUE:
        return 1;
    case LineColor::GREEN:
        return 2;
    case LineColor::YELLOW:
        return 3;
    case LineColor::PURPLE:
        return 4;
    case LineColor::CYAN:
        return 5;
    case LineColor::ORANGE:
        return 6;
    case LineColor::DARK_RED:
        return 7;
    }
    return 7;
}