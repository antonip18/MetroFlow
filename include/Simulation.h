#pragma once
#include "LineHandler.h"
#include "PassengerRouter.h"
#include "RandomShapeGenerator.h"
#include "StationHandler.h"
#include "Train.h"
#include <array>
#include <string>
class Simulation {
  public:
    Simulation(StationHandler &stationHandler, LineHandler &lineHandler);
    void reset(unsigned seed = 2026, World::MapId map = World::MapId::River);
    std::array<sf::Vector2f, 3> endpointHandle(int line, bool start) const;
    void update(float deltaTime);
    void synchronize();
    bool addTrain(int line);
    bool moveTrain(int trainId, int line);
    bool addCarriage(int trainId);
    bool reclaimTrain(int trainId);
    bool reclaimCarriage(int trainId);
    bool upgradeStation(int stationId);
    bool chooseUpgrade(Upgrade upgrade);
    void seedDemo();
    int freeEngines() const;
    int freeCarriages() const;
    int freeInterchanges() const;
    const std::vector<Train> &getTrains() const { return trains; }
    const PassengerRouter &getRouter() const { return router; }
    int score = 0, generated = 0, week = 1;
    float elapsed = 0;
    bool paused = false, gameOver = false, choosingUpgrade = false;
    int speed = 1;
    bool spawning = true;
    int failedStation = -1;
    std::string message;
    unsigned seed = 2026;
    static constexpr float WeekDuration = 90.f;
    static constexpr float OvercrowdingLimit = 28.f;

  private:
    StationHandler &stationHandler;
    LineHandler &lineHandler;
    RandomShapeGenerator random{2026};
    PassengerRouter router;
    std::vector<Train> trains;
    std::array<int, 8> seenRevision{};
    std::array<bool, 8> wasActive{};
    int nextTrainId = 1;
    float stationTimer = 0, passengerTimer = 0;
    void rebuildRouter();
    void tick(float dt);
    void generateNewStations(float dt);
    void generatePassengers(float dt);
    void updateTrain(Train &train, float dt);
    void serve(Train &train, Station &station);
    void depart(Train &train);
    void unload(Train &train);
    Train *findTrain(int id);
    int closestStation(sf::Vector2f position) const;
};
