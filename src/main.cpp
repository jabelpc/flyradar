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
#include "MoteurCouchesRadar.h"

// Identifiants Wi-Fi facultatifs.
// Laisser les deux champs vides pour utiliser le point d'accès de configuration.
const char* preconfiguredWifiSsid = "";
const char* preconfiguredWifiPassword = "";

constexpr int SCREEN_SIZE = 240;
constexpr int SCREEN_SIZE_DIV_2 = SCREEN_SIZE / 2;

LGFX tft;
LGFX_Sprite backbuffer(&tft);

WiFiManager wm;
ConfigurationWebServer configServer;
HttpRequestManager http;

GestionnaireMeteo gestionnaireMeteo(http);
OpenSkyAuthTokenHandler authHandler(http);

AffichageMeteo affichageMeteo;
AffichageRadar affichageRadar;

AircraftManager aircraftManager(
    configServer,
    authHandler,
    http,
    tft,
    affichageRadar
);

MoteurCouchesRadar moteurCouchesRadar(
    configServer,
    aircraftManager,
    gestionnaireMeteo,
    affichageMeteo,
    affichageRadar
);

static void SetupOTA()
{
    ArduinoOTA.setHostname("flyradar");

    ArduinoOTA.onStart([]()
    {
        Serial.println("[OTA] Start");
    });

    ArduinoOTA.onEnd([]()
    {
        Serial.println("[OTA] End");
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("[OTA] Error[%u]: ", error);

        if (error == OTA_AUTH_ERROR)
        {
            Serial.println("Authentication Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            Serial.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            Serial.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            Serial.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            Serial.println("End Failed");
        }
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

    /*
     * Initialisation de l'écran.
     */
    tft.init();
    tft.invertDisplay(true);

    /*
     * Sur ce montage, le GPIO3 est relié à RST.
     * Il est géré par LovyanGFX avec cfg.pin_rst.
     * Le rétroéclairage BLK est directement relié au 3V3.
     */
    backbuffer.setColorDepth(8);
    backbuffer.createSprite(
        SCREEN_SIZE,
        SCREEN_SIZE
    );

    /*
     * Écran de connexion Wi-Fi.
     */
    const RadarTheme bootTheme =
        ParseTheme(
            configServer.GetStoredString("theme")
        );

    tft.fillScreen(
        lgfx::color888(0, 0, 0)
    );

    tft.setTextColor(
        ThemeBaseColor(bootTheme, 255)
    );

    tft.drawCentreString(
        "Connecting to WiFi...",
        SCREEN_SIZE / 2,
        SCREEN_SIZE / 2
    );

    WiFiManagerHelpers::ConfigureWiFiManager(
        wm,
        tft
    );

    /*
     * Utilisation des identifiants intégrés si présents.
     */
    if (strlen(preconfiguredWifiSsid) > 0)
    {
        WiFi.begin(
            preconfiguredWifiSsid,
            preconfiguredWifiPassword
        );

        WiFi.waitForConnectResult();
    }

    /*
     * Sinon, utilisation du portail WiFiManager.
     */
    wm.autoConnect(
        WiFiManagerHelpers::WiFiManagerName
    );

    /*
     * Activation des mises à jour OTA.
     */
    SetupOTA();

    /*
     * Démarrage du serveur de configuration.
     */
    configServer.Initialise();

    /*
     * Restauration du dernier mode radar sélectionné.
     */
    RadarModeManager::Initialise();

    /*
     * Initialisation de la météo.
     */
    gestionnaireMeteo.initialiser();

    /*
     * Définition de la zone météo à partir
     * de la configuration du radar.
     */
    const float latitudeCentre =
        configServer
            .GetStoredString("latitude")
            .toFloat();

    const float longitudeCentre =
        configServer
            .GetStoredString("longitude")
            .toFloat();

    const float rayonDegres =
        configServer
            .GetStoredString("radius")
            .toFloat();

    const float rayonKm =
        rayonDegres * 111.32f;

    gestionnaireMeteo.definirZone(
        latitudeCentre,
        longitudeCentre,
        rayonKm
    );

    /*
     * Affichage de la grille météo dans le moniteur série.
     */
    Serial.println();
    Serial.println("===== Grille météo =====");

    for (
        uint8_t i = 0;
        i < gestionnaireMeteo.obtenirNombrePoints();
        i++
    )
    {
        const auto& point =
            gestionnaireMeteo.obtenirPoint(i);

        Serial.printf(
            "%02u : %.5f  %.5f\n",
            i,
            point.latitude,
            point.longitude
        );
    }

    /*
     * Initialisation du gestionnaire d'avions.
     */
    aircraftManager.Initialise();
}

void loop()
{
    /*
     * Gestion des services.
     */
    ArduinoOTA.handle();

    aircraftManager.Update();

    gestionnaireMeteo.mettreAJourSiNecessaire();

    /*
     * Lecture du mode sélectionné.
     */
    const RadarMode mode =
        RadarModeManager::GetRadarMode();

    /*
     * Construction complète de l'image
     * par le moteur de couches.
     */
    moteurCouchesRadar.dessiner(
        backbuffer,
        mode
    );

    /*
     * Envoi de l'image vers l'écran.
     */
    backbuffer.pushSprite(0, 0);
}