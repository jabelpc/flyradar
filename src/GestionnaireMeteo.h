#pragma once

#include <Arduino.h>

#include "HttpRequestManager.h"
#include "AffichageRadar.h"
#include "LGFX.h"

struct PointMeteo
{
    float latitude = 0.0f;
    float longitude = 0.0f;

    float vitesseVent = 0.0f;
    float directionVent = 0.0f;
    float couvertureNuageuse = 0.0f;

    bool valide = false;
};

class GestionnaireMeteo
{
public:
    static constexpr uint8_t TailleGrille = 5;
    static constexpr uint8_t NombrePoints = TailleGrille * TailleGrille;

    explicit GestionnaireMeteo(HttpRequestManager& gestionnaireHttp);

    void initialiser();

    void definirZone(float latitudeCentre,
                     float longitudeCentre,
                     float rayonKm);

    void mettreAJourSiNecessaire();
    bool demanderMiseAJour();

    bool donneesDisponibles() const;
    bool miseAJourEnCours() const;

    const PointMeteo& obtenirPoint(uint8_t index) const;
    uint8_t obtenirNombrePoints() const;

    unsigned long obtenirAgeDonnees() const;

    void dessinerVent(
        LGFX_Sprite& backbuffer,
        const AffichageRadar& radar
    ) const;

private:
    void calculerGrille();
    void invaliderDonnees();

    bool zoneModifiee(float latitudeCentre,
                      float longitudeCentre,
                      float rayonKm) const;

    HttpRequestManager& http;

    PointMeteo points[NombrePoints];

    float centreLatitude = 0.0f;
    float centreLongitude = 0.0f;
    float rayonZoneKm = 0.0f;

    bool zoneInitialisee = false;
    bool demandeEnCours = false;
    bool donneesValides = false;

    unsigned long dateDernierSucces = 0;
    unsigned long dateDernierEssai = 0;

    static constexpr unsigned long IntervalleMiseAJourMs =
        15UL * 60UL * 1000UL;

    static constexpr unsigned long DelaiNouvelleTentativeMs =
        60UL * 1000UL;

    static constexpr float KilometresParDegreLatitude = 111.32f;
};