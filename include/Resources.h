#pragma once
struct Resources {
    int unlockedLines = 3;
    int engines = 3;
    int carriages = 1;
    int tunnels = 3;
    int interchanges = 1;
};
enum class Upgrade { Line, Carriage, Tunnels, Interchange };
