#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#include "LGFX.h"
#include "WiFiManagerHelpers.h"
#include "ConfigurationWebServer.h"
#include "HttpRequestManager.h"
#include "OpenSkyAuthTokenHandler.h"
#include "AircraftManager.h"
#include "DrawHelpers.h"
#include "ColorTheme.h"
#include "models/Aircraft.h"
#include "models/TrackedAircraft.h"

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
OpenSkyAuthTokenHandler authHandler(http);

AircraftManager aircraftManager(configServer, authHandler, http, tft);

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

  // initialise aircraft manager
  aircraftManager.Initialise();
}

void loop()
{
  ArduinoOTA.handle();
  aircraftManager.Update();

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

  aircraftManager.Draw(backbuffer);
  backbuffer.pushSprite(0, 0);
}

