#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include "AffichageRadar.h"
#include "LGFX.h"
#include "WiFiManagerHelpers.h"
#include "ConfigurationWebServer.h"
#include "HttpRequestManager.h"
#include "OpenSkyAuthTokenHandler.h"
#include "AircraftManager.h"
#include "DrawHelpers.h"
#include "ColorTheme.h"
#include "RadarModeManager.h"
#include "models/Aircraft.h"
#include "models/TrackedAircraft.h"
#include "GestionnaireMeteo.h"
#include "AffichageMeteo.h"

// Optional hard-coded Wi-Fi credentials. Leave both blank to skip pre-baking them and use the setup hotspot instead.
const char* preconfiguredWifiSsid = "";
const char* preconfiguredWifiPassword = "";

constexpr int SCREEN_SIZE = 240;
constexpr int SCREEN_SIZE_DIV_2 = (SCREEN_SIZE / 2);

LGFX tft;
LGFX_Sprite backbuffer(&tft);

WiFiManager wm;
ConfigurationWebServer configServer;
HttpRequestManager http;
GestionnaireMeteo gestionnaireMeteo(http);
OpenSkyAuthTokenHandler authHandler(http);

AffichageMeteo affichageMeteo;

//AircraftManager aircraftManager(configServer, authHandler, http, tft);
AffichageRadar affichageRadar;

AircraftManager aircraftManager(
    configServer,
    authHandler,
    http,
    tft,
    affichageRadar
);

static void SetupOTA()
{
  ArduinoOTA.setHostname("flyradar");

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] End");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Authentication Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] Ready");
  Serial.print("[OTA] IP: ");
  Serial.println(WiFi.localIP());
}

void DrawRadarModePlaceholder(
    LGFX_Sprite& backbuffer,
    RadarMode mode,
    RadarTheme theme
)
{
    constexpr int CENTRE = SCREEN_SIZE_DIV_2 - 1;

    affichageRadar.dessinerFond(
      backbuffer,
      aircraftManager.GetDisplayNorth() 
    );

    aircraftManager.DrawSharedPois(backbuffer);

    const uint32_t textColor =
        ThemeBaseColor(theme, 255);

    const char* label =
        mode == RadarMode::Vent
        ? "VENT"
        : "NUAGES";

    backbuffer.setTextSize(2);
    backbuffer.setTextColor(textColor);

    backbuffer.drawCentreString(
        label,
        CENTRE,
        CENTRE - 10
    );

    backbuffer.setTextSize(1);

    backbuffer.drawCentreString(
        gestionnaireMeteo.donneesDisponibles()
            ? "Donnees disponibles"
            : "Chargement...",
        CENTRE,
        CENTRE + 18
    );
}

void setup()
{
  Serial.begin(115200);
  // delay(1000); // avoids immediate serial output being cut off - uncomment if needed

  // initialise LGFX + screen
  tft.init();
  tft.invertDisplay(true);
  // Note : pas de digitalWrite manuel ici. Sur le montage d'origine, le GPIO3
  // pilotait le rétro-éclairage. Ici, le GPIO3 est câblé au RST de l'écran
  // (géré en interne par LovyanGFX via cfg.pin_rst) et le rétro-éclairage
  // (BLK) est câblé directement en 3V3.

  backbuffer.setColorDepth(8);
  backbuffer.createSprite(SCREEN_SIZE, SCREEN_SIZE);

  // establish WiFi connection
  const RadarTheme bootTheme = ParseTheme(configServer.GetStoredString("theme"));
  tft.fillScreen(lgfx::color888(0, 0, 0));
  tft.setTextColor(ThemeBaseColor(bootTheme, 255));
  tft.drawCentreString("Connecting to WiFi...", SCREEN_SIZE / 2, SCREEN_SIZE / 2);

  WiFiManagerHelpers::ConfigureWiFiManager(wm, tft);

  if (strlen(preconfiguredWifiSsid) > 0) {
    WiFi.begin(preconfiguredWifiSsid, preconfiguredWifiPassword);
    WiFi.waitForConnectResult();
  }

  wm.autoConnect(WiFiManagerHelpers::WiFiManagerName);

  // enable OTA updates over WiFi
  SetupOTA();

  // begin background server for configuration
  configServer.Initialise();

  // restore the last radar mode before drawing
  RadarModeManager::Initialise();
  // initialise weather manager 
  gestionnaireMeteo.initialiser();
  // définir la zone 
  const float latitudeCentre =
    configServer.GetStoredString("latitude").toFloat();

  const float longitudeCentre =
    configServer.GetStoredString("longitude").toFloat();

  const float rayonDegres =
    configServer.GetStoredString("radius").toFloat();

  const float rayonKm = rayonDegres * 111.32f;

  gestionnaireMeteo.definirZone(
    latitudeCentre,
    longitudeCentre,
    rayonKm
  );
  
  Serial.println();
  Serial.println("===== Grille météo =====");

  for (uint8_t i = 0; i < gestionnaireMeteo.obtenirNombrePoints(); i++)
  {
    const auto& p = gestionnaireMeteo.obtenirPoint(i);

    Serial.printf(
        "%02u : %.5f  %.5f\n",
        i,
        p.latitude,
        p.longitude
    );
  }

  // initialise aircraft manager
  aircraftManager.Initialise();
}

void loop()
{
  ArduinoOTA.handle();
  aircraftManager.Update();
  // ajout pour le vent
  gestionnaireMeteo.mettreAJourSiNecessaire();
  // draw cycle
  backbuffer.fillScreen(lgfx::color888(0, 0, 0));

  String renderScanlines = configServer.GetStoredString("scanline");
  if (renderScanlines.isEmpty() || renderScanlines == "true") {
    DrawScanLines(backbuffer,
      SCREEN_SIZE_DIV_2 - 1,
      SCREEN_SIZE_DIV_2 - 1,
      SCREEN_SIZE_DIV_2 - 1 + (std::cos(millis() / 3000.0f) * SCREEN_SIZE_DIV_2),
      SCREEN_SIZE_DIV_2 - 1 + (std::sin(millis() / 3000.0f) * SCREEN_SIZE_DIV_2),
      20, 128, 5,
      aircraftManager.GetTheme()
    );
  }

const RadarMode currentMode = RadarModeManager::GetRadarMode();

if (currentMode == RadarMode::Avions)
{
    aircraftManager.Draw(backbuffer);
}
else if (currentMode == RadarMode::Vent)
{
    affichageRadar.dessinerFond(
        backbuffer,
        aircraftManager.GetDisplayNorth()
    );

    aircraftManager.DrawSharedPois(backbuffer);

    gestionnaireMeteo.dessinerVent(
        backbuffer,
        affichageRadar
    );
}
else if (currentMode == RadarMode::Nuages)
{
    affichageRadar.dessinerFond(
        backbuffer,
        aircraftManager.GetDisplayNorth()
    );

    affichageMeteo.dessinerNuages(
        backbuffer,
        gestionnaireMeteo,
        affichageRadar
    );

    aircraftManager.DrawSharedPois(backbuffer);

    if (!gestionnaireMeteo.donneesDisponibles())
    {
        constexpr int CENTRE =
            SCREEN_SIZE_DIV_2 - 1;

        backbuffer.setTextSize(1);

        backbuffer.setTextColor(
            ThemeBaseColor(
                aircraftManager.GetTheme(),
                255
            )
        );

        backbuffer.drawCentreString(
            "Chargement...",
            CENTRE,
            CENTRE + 18
        );
    }
}

  backbuffer.pushSprite(0, 0);
}

