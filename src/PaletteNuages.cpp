#include "PaletteNuages.h"

#include "LGFX.h"

namespace
{
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

    float calculerProgression(
        float valeur,
        float debut,
        float fin
    )
    {
        if (fin <= debut)
            return 0.0f;

        return limiter(
            (valeur - debut) / (fin - debut),
            0.0f,
            1.0f
        );
    }
}

uint8_t PaletteNuages::interpolerComposante(
    uint8_t debut,
    uint8_t fin,
    float progression
)
{
    progression = limiter(
        progression,
        0.0f,
        1.0f
    );

    return static_cast<uint8_t>(
        debut
        + (static_cast<float>(fin) - debut)
        * progression
    );
}

uint32_t PaletteNuages::interpolerCouleur(
    uint8_t rougeDebut,
    uint8_t vertDebut,
    uint8_t bleuDebut,
    uint8_t rougeFin,
    uint8_t vertFin,
    uint8_t bleuFin,
    float progression
)
{
    return lgfx::color888(
        interpolerComposante(
            rougeDebut,
            rougeFin,
            progression
        ),
        interpolerComposante(
            vertDebut,
            vertFin,
            progression
        ),
        interpolerComposante(
            bleuDebut,
            bleuFin,
            progression
        )
    );
}

RenduNuage PaletteNuages::obtenirRendu(
    float couverture
)
{
    couverture = limiter(
        couverture,
        0.0f,
        100.0f
    );

    RenduNuage rendu {};

    /*
     * L'opacité est simulée par le tramage :
     * 0   = totalement transparent ;
     * 16  = totalement rempli.
     */
    if (couverture < 5.0f)
    {
        rendu.couleur = lgfx::color888(255, 255, 255);
        rendu.opacite = 0;
        return rendu;
    }

    rendu.opacite = static_cast<uint8_t>(
        limiter(
            couverture * 16.0f / 100.0f,
            1.0f,
            16.0f
        )
    );

    /*
     * 5 à 55 % :
     * blanc légèrement bleuté vers blanc pur.
     */
    if (couverture < 55.0f)
    {
        const float progression =
            calculerProgression(
                couverture,
                5.0f,
                55.0f
            );

        rendu.couleur = interpolerCouleur(
            190, 205, 220,
            255, 255, 255,
            progression
        );

        return rendu;
    }

    /*
     * 55 à 72 % :
     * blanc vers jaune.
     */
    if (couverture < 72.0f)
    {
        const float progression =
            calculerProgression(
                couverture,
                55.0f,
                72.0f
            );

        rendu.couleur = interpolerCouleur(
            255, 255, 255,
            255, 220, 0,
            progression
        );

        return rendu;
    }

    /*
     * 72 à 88 % :
     * jaune vers rouge.
     */
    if (couverture < 88.0f)
    {
        const float progression =
            calculerProgression(
                couverture,
                72.0f,
                88.0f
            );

        rendu.couleur = interpolerCouleur(
            255, 220, 0,
            255, 30, 0,
            progression
        );

        return rendu;
    }

    /*
     * 88 à 100 % :
     * rouge vers violet.
     */
    const float progression =
        calculerProgression(
            couverture,
            88.0f,
            100.0f
        );

    rendu.couleur = interpolerCouleur(
        255, 30, 0,
        170, 0, 255,
        progression
    );

    return rendu;
}