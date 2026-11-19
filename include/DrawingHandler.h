#pragma once
#include "LineHandler.h"
#include "LineSelector.h"
#include "Simulation.h"
#include "StationHandler.h"
#include "TrainGeometry.h"
#include "UiState.h"
#include <filesystem>
class DrawingHandler {
  public:
    DrawingHandler(StationHandler &, LineHandler &, LineSelector &, Simulation &,
                   const std::filesystem::path &fontPath);
    void draw(sf::RenderTarget &target, const UiState &ui) const;

  private:
    StationHandler &stationHandler;
    LineHandler &lineHandler;
    LineSelector &lineSelector;
    Simulation &simulation;
    sf::Font font;
    void text(sf::RenderTarget &, const std::string &, sf::Vector2f, unsigned, sf::Color,
              bool bold = false) const;
    void centeredText(sf::RenderTarget &, const std::string &, sf::FloatRect, unsigned, sf::Color,
                      bool bold = false) const;
    void map(sf::RenderTarget &) const;
    void network(sf::RenderTarget &, const UiState &) const;
    void hud(sf::RenderTarget &, const UiState &) const;
    void modal(sf::RenderTarget &, const UiState &) const;
};
