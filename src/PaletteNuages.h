#pragma once

#include <Arduino.h>

struct RenduNuage
{
    uint32_t couleur;
    uint8_t opacite;
};

class PaletteNuages
{
public:
    static RenduNuage obtenirRendu(float couverture);

private:
    static uint8_t interpolerComposante(
        uint8_t debut,
        uint8_t fin,
        float progression
    );

    static uint32_t interpolerCouleur(
        uint8_t rougeDebut,
        uint8_t vertDebut,
        uint8_t bleuDebut,
        uint8_t rougeFin,
        uint8_t vertFin,
        uint8_t bleuFin,
        float progression
    );
};