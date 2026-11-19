#include "TrainGeometry.h"
#include "Curves.h"
std::vector<TrackPose> trainPoses(const Train &train, const LineHandler &lines) {
    const auto &line = lines.at(train.line);
    const auto &geometry = lines.geometry(train.line);
    int station = line.indexOf(train.from);
    if (station < 0 || geometry.path.size() < 2)
        return {};
    float center = geometry.stopDistances[static_cast<std::size_t>(station)] +
                   static_cast<float>(train.direction) * train.distance;
    std::vector<TrackPose> poses;
    for (int car = 0; car <= train.carriages; ++car) {
        float offset = (static_cast<float>(train.carriages) * .5f - static_cast<float>(car)) *
                       TrackStyle::CarSpacing;
        float distance = center + static_cast<float>(train.direction) * offset;
        auto pose = sampleTrack(geometry.path, distance, line.isLoop());
        // The two bogies align each car with its own section of rail at bends.
        auto front =
            sampleTrack(geometry.path, distance + TrackStyle::CarLength * .35f, line.isLoop())
                .position;
        auto back =
            sampleTrack(geometry.path, distance - TrackStyle::CarLength * .35f, line.isLoop())
                .position;
        auto v = front - back;
        if (length(v) > .001f)
            pose.tangent = v / length(v);
        pose.tangent *= static_cast<float>(train.direction);
        poses.push_back(pose);
    }
    return poses;
}
sf::Vector2f visualTrainPosition(const Train &train, const LineHandler &lines) {
    const auto &line = lines.at(train.line);
    const auto &g = lines.geometry(train.line);
    int station = line.indexOf(train.from);
    if (station < 0)
        return train.position;
    return sampleTrack(g.path,
                       g.stopDistances[station] +
                           static_cast<float>(train.direction) * train.distance,
                       line.isLoop())
        .position;
}
