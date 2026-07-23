#pragma once

#include <Arduino.h>

enum class RadarMode : uint8_t {
    Avions = 0,
    Vent = 1,
    Nuages = 2
};

namespace RadarModeManager {

void Initialise();
void SetRadarMode(RadarMode mode);
RadarMode GetRadarMode();
void NextRadarMode();
const char* ToLabel(RadarMode mode);
const char* ToString(RadarMode mode);

}