#include "AircraftManager.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <cmath>

void AircraftManager::Initialise()
{
    // Centre et rayon de la zone
    lat = configServer.GetStoredString("latitude").toDouble();
    lon = configServer.GetStoredString("longitude").toDouble();
    rad = configServer.GetStoredString("radius").toDouble();

    // Rayons des cercles
    const String circle1Str =
        configServer.GetStoredString("circle1");

    const String circle2Str =
        configServer.GetStoredString("circle2");

    const String circle3Str =
        configServer.GetStoredString("circle3");

    circleRadii[0] =
        circle1Str.isEmpty()
        ? 10.0f
        : circle1Str.toFloat();

    circleRadii[1] =
        circle2Str.isEmpty()
        ? 30.0f
        : circle2Str.toFloat();

    circleRadii[2] =
        circle3Str.isEmpty()
        ? 50.0f
        : circle3Str.toFloat();

    // Chargement des points d’intérêt
    for (int i = 0; i < 3; ++i)
    {
        const String latKey =
            String("poi")
            + String(i + 1)
            + String("-latitude");

        const String lonKey =
            String("poi")
            + String(i + 1)
            + String("-longitude");

        const String labelKey =
            String("poi")
            + String(i + 1)
            + String("-name");

        const String enabledKey =
            String("poi")
            + String(i + 1)
            + String("-enabled");

        const String latStr =
            configServer.GetStoredString(
                latKey.c_str()
            );

        const String lonStr =
            configServer.GetStoredString(
                lonKey.c_str()
            );

        const String labelStr =
            configServer.GetStoredString(
                labelKey.c_str()
            );

        const String enabledStr =
            configServer.GetStoredString(
                enabledKey.c_str()
            );

        const bool hasCoordinates =
            !latStr.isEmpty()
            && !lonStr.isEmpty();

        pois[i].enabled =
            enabledStr.isEmpty()
            ? hasCoordinates
            : enabledStr == "true";

        pois[i].enabled =
            pois[i].enabled
            && hasCoordinates;

        pois[i].lat =
            latStr.isEmpty()
            ? 0.0
            : latStr.toDouble();

        pois[i].lon =
            lonStr.isEmpty()
            ? 0.0
            : lonStr.toDouble();

        pois[i].label = labelStr;
    }

    // Options d’affichage
    const String renderText =
        configServer.GetStoredString("infotext");

    const String renderTris =
        configServer.GetStoredString("triangle");

    const String renderNorth =
        configServer.GetStoredString("north");

    if (!renderText.isEmpty())
        displayInfoText = renderText == "true";

    if (!renderTris.isEmpty())
        displayTriangles = renderTris == "true";

    if (!renderNorth.isEmpty())
        displayNorth = renderNorth == "true";

    const String themeName =
        configServer.GetStoredString("theme");

    theme = ParseTheme(themeName);

    // Configuration du composant commun d’affichage
    affichageRadar.definirZone(
        lat,
        lon,
        rad
    );

    affichageRadar.definirTheme(
        theme
    );

    affichageRadar.definirCercles(
        circleRadii[0],
        circleRadii[1],
        circleRadii[2]
    );

    // Calcul de l’intervalle d’interrogation OpenSky
    constexpr unsigned long MS_PER_DAY =
        24UL * 60UL * 60UL * 1000UL;

    constexpr int ANONYMOUS_TOKENS_PER_DAY = 400;
    constexpr int AUTHED_TOKENS_PER_DAY = 4000;
    constexpr int TOKEN_BUFFER = 3;

    int dailyRequestBudget =
        ANONYMOUS_TOKENS_PER_DAY
        - TOKEN_BUFFER;

    const String token =
        authHandler.GetValidToken(
            configServer.GetStoredString(
                "opensky-id"
            ),
            configServer.GetStoredString(
                "opensky-secret"
            )
        );

    if (!token.isEmpty())
    {
        dailyRequestBudget =
            AUTHED_TOKENS_PER_DAY
            - TOKEN_BUFFER;
    }

    fetchInterval =
        MS_PER_DAY
        / dailyRequestBudget;
}

void AircraftManager::Update()
{
    unsigned long now = millis();

    if (now - lastFetch < fetchInterval)
        return;

    lastFetch = now;

    const String token =
        authHandler.GetValidToken(
            configServer.GetStoredString(
                "opensky-id"
            ),
            configServer.GetStoredString(
                "opensky-secret"
            )
        );

    std::vector<std::pair<String, String>>
        headers;

    if (!token.isEmpty())
    {
        headers.push_back(
            {
                "Authorization",
                "Bearer " + token
            }
        );
    }

    const HttpResult result =
        http.Get(
            "https://opensky-network.org/api/states/all",
            {
                {
                    "lamin",
                    String(lat - rad)
                },
                {
                    "lamax",
                    String(lat + rad)
                },
                {
                    "lomin",
                    String(lon - rad)
                },
                {
                    "lomax",
                    String(lon + rad)
                }
            },
            headers
        );

    if (!result.success)
    {
        Serial.print(
            "[WARN] OpenSky API request failed: "
        );

        Serial.println(
            result.errorMessage
        );

        return;
    }

    JsonDocument doc;

    const DeserializationError erreur =
        deserializeJson(
            doc,
            result.response
        );

    if (erreur)
    {
        Serial.print(
            "[WARN] OpenSky JSON incorrect : "
        );

        Serial.println(
            erreur.c_str()
        );

        return;
    }

    auto aircraft =
        JsonParser::ParseArray<Aircraft>(
            doc["states"]
        );

    now = millis();

    for (auto& ac : aircraft)
    {
        auto it =
            trackedAircraft.find(
                ac.icao24
            );

        if (it == trackedAircraft.end())
        {
            trackedAircraft.emplace(
                ac.icao24,
                TrackedAircraft{
                    ac,
                    now
                }
            );
        }
        else
        {
            it->second.Update(
                ac,
                now
            );
        }
    }

    for (
        auto it = trackedAircraft.begin();
        it != trackedAircraft.end();
    )
    {
        const bool aircraftPresent =
            std::any_of(
                aircraft.begin(),
                aircraft.end(),
                [&](const Aircraft& ac)
                {
                    return ac.icao24
                        == it->first;
                }
            );

        if (!aircraftPresent)
        {
            it =
                trackedAircraft.erase(
                    it
                );
        }
        else
        {
            ++it;
        }
    }
}

void AircraftManager::Draw(
    LGFX_Sprite& backbuffer
)
{
    affichageRadar.dessinerFond(
        backbuffer,
        displayNorth
    );

    DrawPois(
        backbuffer
    );

    for (
        auto& [icao, tracked]
        : trackedAircraft
    )
    {
        if (tracked.state.onGround)
            continue;

        tracked.Tick();

        auto [predLat, predLon] =
            tracked.GetDisplayPosition();

        auto [x, y] =
            affichageRadar
                .projeterCoordonnees(
                    predLat,
                    predLon
                );

        if (
            !affichageRadar
                .pointDansEcran(
                    x,
                    y
                )
        )
        {
            continue;
        }

        const uint32_t aircraftColor =
            AircraftColor(
                theme,
                tracked.state.baroAltitude
            );

        if (displayInfoText)
        {
            DrawAircraftInfo(
                backbuffer,
                x,
                y,
                tracked,
                aircraftColor
            );
        }

        if (displayTriangles)
        {
            DrawAircraftTriangle(
                backbuffer,
                x,
                y,
                tracked,
                aircraftColor
            );
        }
        else
        {
            backbuffer.fillCircle(
                x,
                y,
                3,
                aircraftColor
            );
        }
    }
}

void AircraftManager::DrawPois(
    LGFX_Sprite& backbuffer
) const
{
    const uint32_t poiColor =
        lgfx::color888(
            255,
            100,
            180
        );

    backbuffer.setTextSize(1);
    backbuffer.setTextColor(poiColor);

    for (const auto& poi : pois)
    {
        if (!poi.enabled)
            continue;

        auto [x, y] =
            affichageRadar
                .projeterCoordonnees(
                    poi.lat,
                    poi.lon
                );

        if (
            !affichageRadar
                .pointDansEcran(
                    x,
                    y
                )
        )
        {
            continue;
        }

        backbuffer.fillCircle(
            x,
            y,
            4,
            poiColor
        );

        if (poi.label.isEmpty())
            continue;

        const int textWidth =
            poi.label.length()
            * 6;

        int labelX =
            x
            - textWidth / 2;

        if (labelX < 0)
            labelX = 0;

        const int labelY =
            y + 6;

        backbuffer.drawString(
            poi.label,
            labelX,
            labelY
        );
    }
}

void AircraftManager::DrawAircraftInfo(
    LGFX_Sprite& backbuffer,
    int x,
    int y,
    const TrackedAircraft& tracked,
    uint32_t color
) const
{
    const int lineHeight =
        tft.fontHeight()
        + 1;

    backbuffer.setTextSize(1);
    backbuffer.setTextColor(color);

    backbuffer.drawString(
        tracked.state.callsign,
        x + 5,
        y + 5
    );

    backbuffer.drawString(
        String(
            static_cast<int>(
                tracked.state.velocity
            )
        )
        + "m/s",
        x + 5,
        y + 5 + lineHeight
    );

    backbuffer.drawString(
        String(
            static_cast<int>(
                tracked.state.baroAltitude
            )
        )
        + "m",
        x + 5,
        y + 5 + lineHeight * 2
    );
}

void AircraftManager::DrawAircraftTriangle(
    LGFX_Sprite& backbuffer,
    int x,
    int y,
    const TrackedAircraft& tracked,
    uint32_t color
) const
{
    const float dx =
        std::sin(
            radians(
                tracked.state.trueTrack
            )
        );

    const float dy =
        -std::cos(
            radians(
                tracked.state.trueTrack
            )
        );

    const float px = -dy;
    const float py = dx;

    constexpr float TRIANGLE_LENGTH =
        6.0f;

    constexpr float TRIANGLE_WIDTH =
        3.0f;

    const float tipX =
        x
        + dx * TRIANGLE_LENGTH;

    const float tipY =
        y
        + dy * TRIANGLE_LENGTH;

    const float leftX =
        x
        - dx
          * TRIANGLE_LENGTH
          * 0.5f
        + px
          * TRIANGLE_WIDTH
          * 0.5f;

    const float leftY =
        y
        - dy
          * TRIANGLE_LENGTH
          * 0.5f
        + py
          * TRIANGLE_WIDTH
          * 0.5f;

    const float rightX =
        x
        - dx
          * TRIANGLE_LENGTH
          * 0.5f
        - px
          * TRIANGLE_WIDTH
          * 0.5f;

    const float rightY =
        y
        - dy
          * TRIANGLE_LENGTH
          * 0.5f
        - py
          * TRIANGLE_WIDTH
          * 0.5f;

    backbuffer.fillTriangle(
        tipX,
        tipY,
        leftX,
        leftY,
        rightX,
        rightY,
        color
    );
}