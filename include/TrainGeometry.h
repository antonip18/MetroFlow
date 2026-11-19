#pragma once
#include "LineHandler.h"
#include "Train.h"
std::vector<TrackPose> trainPoses(const Train &train, const LineHandler &lines);
sf::Vector2f visualTrainPosition(const Train &train, const LineHandler &lines);
