#pragma once

#include <Arduino.h>
#include <utility>

#include "LGFX.h"
#include "ColorTheme.h"

class AffichageRadar
{
public:
    static constexpr int TailleEcran = 240;
    static constexpr int CentreEcran = TailleEcran / 2 - 1;
    static constexpr int RayonExterieur = TailleEcran / 2 - 1;

    AffichageRadar() = default;

    void definirZone(double latitudeCentre,
                     double longitudeCentre,
                     double rayonDegres);

    void definirTheme(RadarTheme nouveauTheme);

    void definirCercles(float rayon1Km,
                        float rayon2Km,
                        float rayon3Km);

    void dessinerFond(LGFX_Sprite& tampon,
                      bool afficherNord = true) const;

    std::pair<int, int> projeterCoordonnees(
        double latitude,
        double longitude
    ) const;

    bool pointDansEcran(int x, int y) const;

    double obtenirLatitudeCentre() const;
    double obtenirLongitudeCentre() const;
    double obtenirRayonDegres() const;

private:
    void dessinerCercles(LGFX_Sprite& tampon) const;
    void dessinerFlecheNord(LGFX_Sprite& tampon) const;

    double latitudeCentre = 0.0;
    double longitudeCentre = 0.0;
    double rayonDegres = 0.2;

    float rayonsCerclesKm[3] = {
        10.0f,
        30.0f,
        50.0f
    };

    RadarTheme theme = RadarTheme::Green;
};