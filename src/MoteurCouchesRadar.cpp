#include "MoteurCouchesRadar.h"

#include <cmath>

#include "ConfigurationWebServer.h"
#include "AircraftManager.h"
#include "GestionnaireMeteo.h"
#include "AffichageMeteo.h"
#include "AffichageRadar.h"
#include "DrawHelpers.h"
#include "ColorTheme.h"

MoteurCouchesRadar::MoteurCouchesRadar(
    ConfigurationWebServer& configuration,
    AircraftManager& avions,
    GestionnaireMeteo& meteo,
    AffichageMeteo& affichageMeteo,
    AffichageRadar& affichageRadar
)
    : configuration(configuration),
      avions(avions),
      meteo(meteo),
      coucheMeteo(affichageMeteo),
      radar(affichageRadar)
{
}

bool MoteurCouchesRadar::balayageActif() const
{
    const String valeur =
        configuration.GetStoredString("scanline");

    return valeur.isEmpty()
        || valeur == "true";
}

void MoteurCouchesRadar::dessinerCoucheBalayage(
    LGFX_Sprite& tampon
) const
{
    if (!balayageActif())
        return;

    const float angle =
        millis() / 3000.0f;

    const int extremiteX =
        CentreEcran
        + static_cast<int>(
            std::cos(angle)
            * TailleEcran / 2
        );

    const int extremiteY =
        CentreEcran
        + static_cast<int>(
            std::sin(angle)
            * TailleEcran / 2
        );

    DrawScanLines(
        tampon,
        CentreEcran,
        CentreEcran,
        extremiteX,
        extremiteY,
        20,
        128,
        5,
        avions.GetTheme()
    );
}

void MoteurCouchesRadar::dessinerCoucheFondRadar(
    LGFX_Sprite& tampon
) const
{
    radar.dessinerFond(
        tampon,
        avions.GetDisplayNorth()
    );
}

void MoteurCouchesRadar::dessinerCouchePoi(
    LGFX_Sprite& tampon
) const
{
    avions.DrawSharedPois(tampon);
}

void MoteurCouchesRadar::dessinerCoucheAvions(
    LGFX_Sprite& tampon
) const
{
    avions.Draw(tampon);
}

void MoteurCouchesRadar::dessinerCoucheVent(
    LGFX_Sprite& tampon
) const
{
    meteo.dessinerVent(
        tampon,
        radar
    );
}

void MoteurCouchesRadar::dessinerCoucheNuages(
    LGFX_Sprite& tampon
) const
{
    coucheMeteo.dessinerNuages(
        tampon,
        meteo,
        radar
    );
}

void MoteurCouchesRadar::dessinerChargementMeteo(
    LGFX_Sprite& tampon
) const
{
    if (meteo.donneesDisponibles())
        return;

    tampon.setTextSize(1);

    tampon.setTextColor(
        ThemeBaseColor(
            avions.GetTheme(),
            255
        )
    );

    tampon.drawCentreString(
        "Chargement...",
        CentreEcran,
        CentreEcran + 18
    );
}

void MoteurCouchesRadar::dessinerModeAvions(
    LGFX_Sprite& tampon
) const
{
    /*
     * On conserve l'ordre historique du mode avions.
     *
     * AircraftManager::Draw() dessine actuellement
     * le radar, les POI et les avions.
     */
    dessinerCoucheBalayage(tampon);
    dessinerCoucheAvions(tampon);
}

void MoteurCouchesRadar::dessinerModeVent(
    LGFX_Sprite& tampon
) const
{
    /*
     * Ordre des couches :
     *
     * 1. balayage ;
     * 2. fond radar ;
     * 3. POI ;
     * 4. flèches du vent.
     */
    dessinerCoucheBalayage(tampon);
    dessinerCoucheFondRadar(tampon);
    dessinerCouchePoi(tampon);
    dessinerCoucheVent(tampon);
    dessinerChargementMeteo(tampon);
}

void MoteurCouchesRadar::dessinerModeNuages(
    LGFX_Sprite& tampon
) const
{
    /*
     * Ordre des couches :
     *
     * 1. nuages en arrière-plan ;
     * 2. balayage ;
     * 3. cercles et nord ;
     * 4. POI ;
     * 5. message éventuel.
     */
    dessinerCoucheNuages(tampon);
    dessinerCoucheBalayage(tampon);
    dessinerCoucheFondRadar(tampon);
    dessinerCouchePoi(tampon);
    dessinerChargementMeteo(tampon);
}

void MoteurCouchesRadar::dessiner(
    LGFX_Sprite& tampon,
    RadarMode mode
) const
{
    /*
     * Le moteur est désormais responsable
     * de l'effacement du tampon.
     */
    tampon.fillScreen(
        lgfx::color888(0, 0, 0)
    );

    switch (mode)
    {
        case RadarMode::Avions:
            dessinerModeAvions(tampon);
            break;

        case RadarMode::Vent:
            dessinerModeVent(tampon);
            break;

        case RadarMode::Nuages:
            dessinerModeNuages(tampon);
            break;

        default:
            dessinerModeAvions(tampon);
            break;
    }
}