#pragma once
#include "DrawingHandler.h"
class GameEventsHandler {
  public:
    explicit GameEventsHandler(const std::filesystem::path &fontPath);
    void mouseMoved(float x, float y);
    void mouseReleased();
    void mousePressed(float x, float y, bool right = false, bool shift = false);
    void keyPressed(sf::Keyboard::Key key);
    void cancelInteraction();
    void update(float time);
    void draw(sf::RenderTarget &target);
    StationHandler &stations() { return stationHandler; }
    LineHandler &lines() { return lineHandler; }
    Simulation &game() { return simulation; }
    UiState &state() { return ui; }

  private:
    StationHandler stationHandler;
    LineHandler lineHandler;
    LineSelector lineSelector;
    Simulation simulation;
    DrawingHandler drawingHandler;
    UiState ui;
    int trainAt(sf::Vector2f position) const;
    void notify(const std::string &message);
    void selectLine(int i);
    void applyTool();
    void reset();
    void removeAt(sf::Vector2f position, bool shift);
    void beginEdit(int line, Edit edit, int index);
    bool commit(int line, std::vector<Station *> route, bool loop);
};
