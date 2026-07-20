#include "AircraftManager.h"

constexpr int SCREEN_SIZE = 240;
constexpr int SCREEN_SIZE_DIV_2 = (SCREEN_SIZE / 2);

#include <ArduinoJson.h>

void AircraftManager::Initialise()
{
    // get centre point + radius
    lat = configServer.GetStoredString("latitude").toDouble();
    lon = configServer.GetStoredString("longitude").toDouble();
    rad = configServer.GetStoredString("radius").toDouble();

    // load circle radii
    const String circle1Str = configServer.GetStoredString("circle1");
    const String circle2Str = configServer.GetStoredString("circle2");
    const String circle3Str = configServer.GetStoredString("circle3");
    circleRadii[0] = circle1Str.isEmpty() ? 10.0f : circle1Str.toFloat();
    circleRadii[1] = circle2Str.isEmpty() ? 30.0f : circle2Str.toFloat();
    circleRadii[2] = circle3Str.isEmpty() ? 50.0f : circle3Str.toFloat();

    // load POI configuration
    for (int i = 0; i < 3; ++i) {
        const String latKey = String("poi") + String(i + 1) + String("-latitude");
        const String lonKey = String("poi") + String(i + 1) + String("-longitude");
        const String labelKey = String("poi") + String(i + 1) + String("-name");
        const String enabledKey = String("poi") + String(i + 1) + String("-enabled");
        const String latStr = configServer.GetStoredString(latKey.c_str());
        const String lonStr = configServer.GetStoredString(lonKey.c_str());
        const String labelStr = configServer.GetStoredString(labelKey.c_str());
        const String enabledStr = configServer.GetStoredString(enabledKey.c_str());

        const bool hasCoordinates = !latStr.isEmpty() && !lonStr.isEmpty();
        pois[i].enabled = enabledStr.isEmpty() ? hasCoordinates : (enabledStr == "true");
        pois[i].enabled = pois[i].enabled && hasCoordinates;
        pois[i].lat = latStr.isEmpty() ? 0.0 : latStr.toDouble();
        pois[i].lon = lonStr.isEmpty() ? 0.0 : lonStr.toDouble();
        pois[i].label = labelStr;
    }

    // configuration
    const String renderText = configServer.GetStoredString("infotext");
    const String renderTris = configServer.GetStoredString("triangle");
    const String renderNorth = configServer.GetStoredString("north");
    if (!renderText.isEmpty()) displayInfoText = renderText == "true" ? true : false;
    if (!renderTris.isEmpty()) displayTriangles = renderTris == "true" ? true : false;
    if (!renderNorth.isEmpty()) displayNorth = renderNorth == "true" ? true : false;

    const String themeName = configServer.GetStoredString("theme");
    theme = ParseTheme(themeName);

    // calculate how often we can call OpenSky API before being rate limited
    constexpr int MS_PER_DAY = 24 * 60 * 60 * 1000;
    constexpr int ANONYMOUS_TOKENS_PER_DAY = 400;
    constexpr int AUTHED_TOKENS_PER_DAY = 4000;
    constexpr int TOKEN_BUFFER = 3;
    int dailyRequestBudget = ANONYMOUS_TOKENS_PER_DAY - TOKEN_BUFFER; // non-authed tokens minus buffer

    const String token = authHandler.GetValidToken(configServer.GetStoredString("opensky-id"), configServer.GetStoredString("opensky-secret"));
    if (!token.isEmpty())
        dailyRequestBudget = AUTHED_TOKENS_PER_DAY - TOKEN_BUFFER; // authed tokens minus buffer

    fetchInterval = MS_PER_DAY / dailyRequestBudget;
}

void AircraftManager::Update()
{
    unsigned long now = millis();

    // fetch cycle
    if (now - lastFetch >= fetchInterval) {
        lastFetch = now;

        // auth
        const String token = authHandler.GetValidToken(
            configServer.GetStoredString("opensky-id"),
            configServer.GetStoredString("opensky-secret")
        );

        std::vector<std::pair<String, String>> headers = {};
        if (!token.isEmpty()) headers.push_back({ "Authorization", "Bearer " + token });

        // request
        HttpResult result = http.Get(
            "https://opensky-network.org/api/states/all",
            {
              {"lamin", String(lat - rad)},
              {"lamax", String(lat + rad)},
              {"lomin", String(lon - rad)},
              {"lomax", String(lon + rad)}
            },
            headers
        );

        // If request failed, skip this update
        if (!result.success) {
            Serial.print("[WARN] OpenSky API request failed: ");
            Serial.println(result.errorMessage);
            return;
        }

        // track
        JsonDocument doc;
        deserializeJson(doc, result.response);
        auto aircraft = JsonParser::ParseArray<Aircraft>(doc["states"]);
        now = millis(); // override with post-parse timestamp

        for (auto& ac : aircraft) {
            auto it = trackedAircraft.find(ac.icao24);
            if (it == trackedAircraft.end())
                trackedAircraft.emplace(ac.icao24, TrackedAircraft{ ac, now });
            else
                it->second.Update(ac, now);
        }

        // remove any planes that disappeared from the feed
        for (auto it = trackedAircraft.begin(); it != trackedAircraft.end(); ) {
            bool aircraftPresent = std::any_of(aircraft.begin(), aircraft.end(), [&](const Aircraft& ac) { return ac.icao24 == it->first; });
            if (!aircraftPresent)
                it = trackedAircraft.erase(it);
            else
                ++it;
        }
    }
}

void AircraftManager::Draw(LGFX_Sprite& backbuffer)
{
    DrawRadarCircles(backbuffer);
    
    if (displayNorth)
        DrawNorthArrow(backbuffer);

    DrawPois(backbuffer);

    for (auto& [icao, tracked] : trackedAircraft) {
        if (tracked.state.onGround) continue;

        tracked.Tick();
        auto [predLat, predLon] = tracked.GetDisplayPosition();
        auto [x, y] = ProjectCoordinateToScreen(predLat, predLon);

        const uint32_t aircraftColor = AircraftColor(theme, tracked.state.baroAltitude);

        if (displayInfoText)
            DrawAircraftInfo(backbuffer, x, y, tracked, aircraftColor);

        if (displayTriangles)
            DrawAircraftTriangle(backbuffer, x, y, tracked, aircraftColor);
        else
            backbuffer.fillCircle(x, y, 3, aircraftColor);
    }
}

void AircraftManager::DrawRadarCircles(LGFX_Sprite& backbuffer) const
{
    constexpr int CENTRE = SCREEN_SIZE_DIV_2 - 1;
    constexpr int OUTER = SCREEN_SIZE_DIV_2 - 1;

    backbuffer.drawCircle(CENTRE, CENTRE, OUTER, ThemeBaseColor(theme, 200));

    if (rad > 0.0f) {
        const float kmPerDegree = 111.32f;
        const float totalRangeKm = rad * kmPerDegree;
        const uint32_t markerColor = lgfx::color888(150, 150, 150);
        const uint32_t labelColor = lgfx::color888(190, 190, 190);

        auto drawMarker = [&](float km) {
            if (totalRangeKm < km)
                return;
            const float radiusRatio = km / totalRangeKm;
            const int markerRadius = static_cast<int>(OUTER * radiusRatio);
            if (markerRadius <= 0 || markerRadius >= OUTER)
                return;
            const String valueLabel = (km == (int)km) ? String((int)km) : String(km, 1);
            backbuffer.drawCircle(CENTRE, CENTRE, markerRadius, markerColor);
            backbuffer.setTextSize(1);
            backbuffer.setTextColor(labelColor);
            const int textX = CENTRE + markerRadius + 2;
            const int textY = CENTRE - 6;
            const int lineHeight = tft.fontHeight();
            backbuffer.drawString(valueLabel, textX, textY);
            backbuffer.drawString("km", textX, textY + lineHeight);
        };

        for (int i = 0; i < 3; ++i)
            if (circleRadii[i] > 0.0f)
                drawMarker(circleRadii[i]);
    }
}

void AircraftManager::DrawNorthArrow(LGFX_Sprite& backbuffer) const
{
    constexpr int CENTRE = SCREEN_SIZE_DIV_2 - 1;
    constexpr int ARROW_DISTANCE = 80;
    constexpr int ARROW_SIZE = 8;
    
    // Position at top center
    const int arrowX = CENTRE;
    const int arrowY = CENTRE - ARROW_DISTANCE;
    
    const uint32_t arrowColor = lgfx::color888(200, 200, 200);
    
    // Draw arrow pointing up (north)
    // Tip of arrow
    const int tipX = arrowX;
    const int tipY = arrowY - ARROW_SIZE;
    
    // Base of arrow
    const int baseLeftX = arrowX - ARROW_SIZE / 2;
    const int baseRightX = arrowX + ARROW_SIZE / 2;
    const int baseY = arrowY;
    
    // Draw filled triangle (arrow head)
    backbuffer.fillTriangle(tipX, tipY, baseLeftX, baseY, baseRightX, baseY, arrowColor);
    
    // Draw "N" label
    backbuffer.setTextSize(1);
    backbuffer.setTextColor(arrowColor);
    backbuffer.drawString("N", arrowX - 2, arrowY + ARROW_SIZE + 2);
}

void AircraftManager::DrawPois(LGFX_Sprite& backbuffer) const
{
    const uint32_t poiColor = lgfx::color888(255, 100, 180);
    backbuffer.setTextSize(1);
    backbuffer.setTextColor(poiColor);

    for (const auto& poi : pois) {
        if (!poi.enabled)
            continue;

        auto [x, y] = ProjectCoordinateToScreen(poi.lat, poi.lon);
        backbuffer.fillCircle(x, y, 4, poiColor);

        if (!poi.label.isEmpty()) {
            const int textWidth = poi.label.length() * 6;
            int labelX = x - textWidth / 2;
            if (labelX < 0) labelX = 0;
            const int labelY = y + 6;
            backbuffer.drawString(poi.label, labelX, labelY);
        }
    }
}

std::pair<int, int> AircraftManager::ProjectCoordinateToScreen(float predLat, float predLon) const
{
    const float dLon = predLon - lon;
    const float dLat = predLat - lat;

    const float normLon = (dLon + rad) / (2.0f * rad);
    const float normLat = (dLat + rad) / (2.0f * rad);

    const int x = static_cast<int>(normLon * SCREEN_SIZE);
    const int y = static_cast<int>(SCREEN_SIZE - (normLat * SCREEN_SIZE));

    return { x, y };
}

void AircraftManager::DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, uint32_t color) const
{
    const int lineHeight = tft.fontHeight() + 1;

    backbuffer.setTextSize(1);
    backbuffer.setTextColor(color);
    backbuffer.drawString(tracked.state.callsign, x + 5, y + 5);
    backbuffer.drawString(String((int)tracked.state.velocity) + "m/s", x + 5, y + 5 + lineHeight);
    backbuffer.drawString(String((int)tracked.state.baroAltitude) + "m", x + 5, y + 5 + lineHeight * 2);
}

void AircraftManager::DrawAircraftTriangle(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked, uint32_t color) const
{
    const float dx = std::sin(radians(tracked.state.trueTrack));
    const float dy = -std::cos(radians(tracked.state.trueTrack));
    const float px = -dy;
    const float py = dx;

    constexpr float TRIANGLE_LENGTH = 6.0f;
    constexpr float TRIANGLE_WIDTH = 3.0f;

    const float tipX = x + dx * TRIANGLE_LENGTH;
    const float tipY = y + dy * TRIANGLE_LENGTH;
    const float leftX = x - dx * TRIANGLE_LENGTH * 0.5f + px * TRIANGLE_WIDTH * 0.5f;
    const float leftY = y - dy * TRIANGLE_LENGTH * 0.5f + py * TRIANGLE_WIDTH * 0.5f;
    const float rightX = x - dx * TRIANGLE_LENGTH * 0.5f - px * TRIANGLE_WIDTH * 0.5f;
    const float rightY = y - dy * TRIANGLE_LENGTH * 0.5f - py * TRIANGLE_WIDTH * 0.5f;

    backbuffer.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, color);
}