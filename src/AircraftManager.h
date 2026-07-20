#pragma once

#include <map>

#include "models/TrackedAircraft.h"
#include "ConfigurationWebServer.h"
#include "OpenSkyAuthTokenHandler.h"
#include "LGFX.h"
#include "ColorTheme.h"

class AircraftManager
{
private:
    struct Poi {
        double lat = 0.0;
        double lon = 0.0;
        String label;
        bool enabled = false;
    };

    double lat = 0.0;
    double lon = 0.0;
    double rad = 0.2;
    float circleRadii[3] = {10.0f, 30.0f, 50.0f};
    Poi pois[3];
    std::map<String, TrackedAircraft> trackedAircraft;

    bool displayInfoText = true;
    bool displayTriangles = true;
    bool displayNorth = true;
    RadarTheme theme = RadarTheme::Green;

    unsigned long fetchInterval = 0;
    unsigned long lastFetch = 999999;

    ConfigurationWebServer& configServer;
    OpenSkyAuthTokenHandler& authHandler;
    HttpRequestManager& http;
    LGFX& tft;

    void DrawRadarCircles(LGFX_Sprite& backbuffer) const;
    void DrawNorthArrow(LGFX_Sprite& backbuffer) const;
    void DrawPois(LGFX_Sprite& backbuffer) const;
    std::pair<int, int> ProjectCoordinateToScreen(float predLat, float predLon) const;
    void DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, uint32_t color) const;
    void DrawAircraftTriangle(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, uint32_t color) const;

public:
    AircraftManager(ConfigurationWebServer& config, OpenSkyAuthTokenHandler& auth, HttpRequestManager& httpManager, LGFX& tftGfx)
        : configServer(config), authHandler(auth), http(httpManager), tft(tftGfx)
    {
    }
    ~AircraftManager() = default;

    void Initialise();
    void Update();
    void Draw(LGFX_Sprite& backbuffer);
    RadarTheme GetTheme() const { return theme; }
};