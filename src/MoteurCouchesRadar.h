#pragma once

#include <Arduino.h>

#include "LGFX.h"
#include "RadarModeManager.h"

class ConfigurationWebServer;
class AircraftManager;
class GestionnaireMeteo;
class AffichageMeteo;
class AffichageRadar;

class MoteurCouchesRadar
{
public:
    MoteurCouchesRadar(
        ConfigurationWebServer& configuration,
        AircraftManager& avions,
        GestionnaireMeteo& meteo,
        AffichageMeteo& affichageMeteo,
        AffichageRadar& affichageRadar
    );

    void dessiner(
        LGFX_Sprite& tampon,
        RadarMode mode
    ) const;

private:
    static constexpr int TailleEcran = 240;
    static constexpr int CentreEcran = TailleEcran / 2 - 1;

    ConfigurationWebServer& configuration;
    AircraftManager& avions;
    GestionnaireMeteo& meteo;
    AffichageMeteo& coucheMeteo;
    AffichageRadar& radar;

    bool balayageActif() const;

    void dessinerCoucheBalayage(
        LGFX_Sprite& tampon
    ) const;

    void dessinerCoucheFondRadar(
        LGFX_Sprite& tampon
    ) const;

    void dessinerCouchePoi(
        LGFX_Sprite& tampon
    ) const;

    void dessinerCoucheAvions(
        LGFX_Sprite& tampon
    ) const;

    void dessinerCoucheVent(
        LGFX_Sprite& tampon
    ) const;

    void dessinerCoucheNuages(
        LGFX_Sprite& tampon
    ) const;

    void dessinerChargementMeteo(
        LGFX_Sprite& tampon
    ) const;

    void dessinerModeAvions(
        LGFX_Sprite& tampon
    ) const;

    void dessinerModeVent(
        LGFX_Sprite& tampon
    ) const;

    void dessinerModeNuages(
        LGFX_Sprite& tampon
    ) const;
};