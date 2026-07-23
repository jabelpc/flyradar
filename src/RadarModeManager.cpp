#include "RadarModeManager.h"
#include <Preferences.h>

namespace {
static constexpr const char* PrefsNamespace = "config";
static constexpr const char* RadarModeKey = "radarMode";
static RadarMode currentMode = RadarMode::Avions;

bool IsValidRadarModeValue(uint8_t value) {
    return value <= static_cast<uint8_t>(RadarMode::Nuages);
}

RadarMode RadarModeFromValue(uint8_t value) {
    return IsValidRadarModeValue(value) ? static_cast<RadarMode>(value) : RadarMode::Avions;
}

}

namespace RadarModeManager {

void Initialise() {
    Preferences prefs;
    prefs.begin(PrefsNamespace, true);
    const uint8_t stored = prefs.getUChar(RadarModeKey, static_cast<uint8_t>(RadarMode::Avions));
    prefs.end();
    currentMode = RadarModeFromValue(stored);
}

void SetRadarMode(RadarMode mode) {
    const uint8_t value = static_cast<uint8_t>(mode);
    const RadarMode normalized = RadarModeFromValue(value);
    Preferences prefs;
    prefs.begin(PrefsNamespace, false);
    prefs.putUChar(RadarModeKey, static_cast<uint8_t>(normalized));
    prefs.end();
    currentMode = normalized;
}

RadarMode GetRadarMode() {
    return currentMode;
}

void NextRadarMode() {
    const uint8_t nextValue = (static_cast<uint8_t>(currentMode) + 1u) % 3u;
    SetRadarMode(static_cast<RadarMode>(nextValue));
}

const char* ToLabel(RadarMode mode) {
    switch (mode) {
        case RadarMode::Avions: return "Avions";
        case RadarMode::Vent: return "Vent";
        case RadarMode::Nuages: return "Nuages";
        default: return "Avions";
    }
}

const char* ToString(RadarMode mode) {
    switch (mode) {
        case RadarMode::Avions: return "0";
        case RadarMode::Vent: return "1";
        case RadarMode::Nuages: return "2";
        default: return "0";
    }
}

}