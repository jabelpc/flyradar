#include "AffichageMeteo.h"
#include "PaletteNuages.h"
#include <cmath>

namespace
{
    constexpr int TailleEcran = 240;
    constexpr int CentreRadar = TailleEcran / 2 - 1;
    constexpr int RayonRadar = TailleEcran / 2 - 2;

    // Taille des petits blocs utilisés pour le rendu.
    // Plus la valeur est faible, plus le rendu est précis,
    // mais plus il demande de calculs.
    constexpr int TailleBloc = 3;

    constexpr uint8_t MatriceTramage[4][4] =
    {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 }
    };

    float limiter(
        float valeur,
        float minimum,
        float maximum
    )
    {
        if (valeur < minimum)
            return minimum;

        if (valeur > maximum)
            return maximum;

        return valeur;
    }

    bool pointDansCercle(
        int x,
        int y
    )
    {
        const int dx = x - CentreRadar;
        const int dy = y - CentreRadar;

        return dx * dx + dy * dy
            <= RayonRadar * RayonRadar;
    }
}

float AffichageMeteo::interpoler(
    float hautGauche,
    float hautDroite,
    float basGauche,
    float basDroite,
    float positionX,
    float positionY
) const
{
    const float valeurHaute =
        hautGauche
        + (hautDroite - hautGauche) * positionX;

    const float valeurBasse =
        basGauche
        + (basDroite - basGauche) * positionX;

    return valeurHaute
        + (valeurBasse - valeurHaute) * positionY;
}

void AffichageMeteo::dessinerNuages(
    LGFX_Sprite& backbuffer,
    const GestionnaireMeteo& meteo,
    const AffichageRadar& radar
) const
{
    if (!meteo.donneesDisponibles())
        return;
    //ajout temp
    static unsigned long dernierAffichage = 0;

if (millis() - dernierAffichage > 5000)
{
    dernierAffichage = millis();

    float minimum = 100.0f;
    float maximum = 0.0f;
    float total = 0.0f;
    uint8_t nombreValide = 0;

    for (
        uint8_t i = 0;
        i < GestionnaireMeteo::NombrePoints;
        i++
    )
    {
        const PointMeteo& point =
            meteo.obtenirPoint(i);

        if (!point.valide)
            continue;

        minimum = min(
            minimum,
            point.couvertureNuageuse
        );

        maximum = max(
            maximum,
            point.couvertureNuageuse
        );

        total += point.couvertureNuageuse;
        nombreValide++;
    }

    Serial.printf(
        "[Nuages] Valides=%u Min=%.1f Max=%.1f Moyenne=%.1f\n",
        nombreValide,
        minimum,
        maximum,
        nombreValide > 0
            ? total / nombreValide
            : 0.0f
    );
}

    // La grille est organisée de gauche à droite,
    // puis de haut en bas.
    const PointMeteo& pointHautGauche =
        meteo.obtenirPoint(0);

    const PointMeteo& pointBasDroite =
        meteo.obtenirPoint(
            GestionnaireMeteo::NombrePoints - 1
        );

    auto [gauche, haut] =
        radar.projeterCoordonnees(
            pointHautGauche.latitude,
            pointHautGauche.longitude
        );

    auto [droite, bas] =
        radar.projeterCoordonnees(
            pointBasDroite.latitude,
            pointBasDroite.longitude
        );

    if (gauche > droite)
    {
        const int temporaire = gauche;
        gauche = droite;
        droite = temporaire;
    }

    if (haut > bas)
    {
        const int temporaire = haut;
        haut = bas;
        bas = temporaire;
    }

    const float largeur =
        static_cast<float>(droite - gauche);

    const float hauteur =
        static_cast<float>(bas - haut);

    if (largeur <= 0.0f || hauteur <= 0.0f)
        return;

    for (
        int y = haut;
        y <= bas;
        y += TailleBloc
    )
    {
        for (
            int x = gauche;
            x <= droite;
            x += TailleBloc
        )
        {
            const int centreBlocX =
                x + TailleBloc / 2;

            const int centreBlocY =
                y + TailleBloc / 2;

            if (
                !pointDansCercle(
                    centreBlocX,
                    centreBlocY
                )
            )
            {
                continue;
            }

            float positionGrilleX =
                ((centreBlocX - gauche) / largeur)
                * (GestionnaireMeteo::TailleGrille - 1);

            float positionGrilleY =
                ((centreBlocY - haut) / hauteur)
                * (GestionnaireMeteo::TailleGrille - 1);

            positionGrilleX = limiter(
                positionGrilleX,
                0.0f,
                GestionnaireMeteo::TailleGrille - 1.001f
            );

            positionGrilleY = limiter(
                positionGrilleY,
                0.0f,
                GestionnaireMeteo::TailleGrille - 1.001f
            );

            const uint8_t colonne =
                static_cast<uint8_t>(
                    floorf(positionGrilleX)
                );

            const uint8_t ligne =
                static_cast<uint8_t>(
                    floorf(positionGrilleY)
                );

            const float fractionX =
                positionGrilleX - colonne;

            const float fractionY =
                positionGrilleY - ligne;

            const uint8_t indexHautGauche =
                ligne
                * GestionnaireMeteo::TailleGrille
                + colonne;

            const uint8_t indexHautDroite =
                indexHautGauche + 1;

            const uint8_t indexBasGauche =
                indexHautGauche
                + GestionnaireMeteo::TailleGrille;

            const uint8_t indexBasDroite =
                indexBasGauche + 1;

            const PointMeteo& hautGauche =
                meteo.obtenirPoint(indexHautGauche);

            const PointMeteo& hautDroite =
                meteo.obtenirPoint(indexHautDroite);

            const PointMeteo& basGauche =
                meteo.obtenirPoint(indexBasGauche);

            const PointMeteo& basDroite =
                meteo.obtenirPoint(indexBasDroite);

            if (
                !hautGauche.valide
                || !hautDroite.valide
                || !basGauche.valide
                || !basDroite.valide
            )
            {
                continue;
            }

            const float couverture =
                interpoler(
                    hautGauche.couvertureNuageuse,
                    hautDroite.couvertureNuageuse,
                    basGauche.couvertureNuageuse,
                    basDroite.couvertureNuageuse,
                    fractionX,
                    fractionY
                );

            //if (couverture < 5.0f)
            //    continue;
            const RenduNuage rendu =
                PaletteNuages::obtenirRendu(couverture);

            if (rendu.opacite == 0)
                continue;

            const uint8_t seuilTramage =
                MatriceTramage[
                    (y / TailleBloc) % 4
                ][
                    (x / TailleBloc) % 4
                ];

            if (rendu.opacite <= seuilTramage)
                continue;

            const uint32_t couleur = rendu.couleur;

            backbuffer.fillRect(
                x,
                y,
                TailleBloc,
                TailleBloc,
                couleur
            );
        }
    }
}