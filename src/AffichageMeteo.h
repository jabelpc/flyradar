#pragma once

#include <Arduino.h>

#include "LGFX.h"
#include "AffichageRadar.h"
#include "GestionnaireMeteo.h"

class AffichageMeteo
{
public:
    void dessinerNuages(
        LGFX_Sprite& backbuffer,
        const GestionnaireMeteo& meteo,
        const AffichageRadar& radar
    ) const;

private:
    float interpoler(
        float hautGauche,
        float hautDroite,
        float basGauche,
        float basDroite,
        float positionX,
        float positionY
    ) const;
};