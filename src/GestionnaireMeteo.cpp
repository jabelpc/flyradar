#include "GestionnaireMeteo.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <cmath>

GestionnaireMeteo::GestionnaireMeteo(
    HttpRequestManager& gestionnaireHttp
)
    : http(gestionnaireHttp)
{
}

void GestionnaireMeteo::initialiser()
{
    invaliderDonnees();

    dateDernierSucces = 0;
    dateDernierEssai = 0;

    Serial.println("[Meteo] Gestionnaire initialisé");
}

void GestionnaireMeteo::definirZone(float latitudeCentre,
                                    float longitudeCentre,
                                    float rayonKm)
{
    if (!zoneModifiee(latitudeCentre, longitudeCentre, rayonKm))
        return;

    centreLatitude = latitudeCentre;
    centreLongitude = longitudeCentre;
    rayonZoneKm = rayonKm;

    zoneInitialisee = true;

    calculerGrille();
    invaliderDonnees();

    // Autorise une récupération immédiate après un changement de zone.
    dateDernierEssai = 0;

    Serial.printf(
        "[Meteo] Zone définie : %.5f, %.5f, rayon %.1f km\n",
        centreLatitude,
        centreLongitude,
        rayonZoneKm
    );
}

void GestionnaireMeteo::mettreAJourSiNecessaire()
{
    if (!zoneInitialisee || demandeEnCours)
        return;

    const unsigned long maintenant = millis();

    if (donneesValides)
    {
        const unsigned long ageDonnees =
            maintenant - dateDernierSucces;

        if (ageDonnees < IntervalleMiseAJourMs)
            return;
    }
    else if (dateDernierEssai != 0)
    {
        const unsigned long tempsDepuisDernierEssai =
            maintenant - dateDernierEssai;

        if (tempsDepuisDernierEssai < DelaiNouvelleTentativeMs)
            return;
    }

    demanderMiseAJour();
}

bool GestionnaireMeteo::demanderMiseAJour()
{
    if (!zoneInitialisee)
    {
        Serial.println("[Meteo] Zone non initialisée");
        return false;
    }

    if (demandeEnCours)
    {
        Serial.println("[Meteo] Une demande est déjà en cours");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[Meteo] Wi-Fi indisponible");
        dateDernierEssai = millis();
        return false;
    }

    demandeEnCours = true;
    dateDernierEssai = millis();

    String latitudes;
    String longitudes;

    latitudes.reserve(300);
    longitudes.reserve(300);

    for (uint8_t i = 0; i < NombrePoints; i++)
    {
        if (i > 0)
        {
            latitudes += ",";
            longitudes += ",";
        }

        latitudes += String(points[i].latitude, 5);
        longitudes += String(points[i].longitude, 5);
    }

    const std::vector<std::pair<String, String>> parametres = {
        {
            "latitude",
            latitudes
        },
        {
            "longitude",
            longitudes
        },
        {
            "current",
            "wind_speed_10m,wind_direction_10m,cloud_cover"
        },
        {
            "wind_speed_unit",
            "kmh"
        }
    };

    Serial.println("[Meteo] Demande Open-Meteo");
    Serial.printf(
        "[Meteo] Mémoire libre avant requête : %u octets\n",
        ESP.getFreeHeap()
    );

    const HttpResult resultat = http.Get(
        "https://api.open-meteo.com/v1/forecast",
        parametres
    );

    if (!resultat.success)
    {
        Serial.print("[Meteo] Erreur de connexion : ");
        Serial.println(resultat.errorMessage);

        demandeEnCours = false;
        return false;
    }

    if (resultat.statusCode != 200)
    {
        Serial.printf(
            "[Meteo] Code HTTP incorrect : %d\n",
            resultat.statusCode
        );

        demandeEnCours = false;
        return false;
    }

    if (resultat.response.isEmpty())
    {
        Serial.println("[Meteo] Réponse vide");

        demandeEnCours = false;
        return false;
    }

    Serial.printf(
        "[Meteo] Taille de la réponse : %u octets\n",
        static_cast<unsigned int>(resultat.response.length())
    );

    JsonDocument document;

    const DeserializationError erreur =
        deserializeJson(document, resultat.response);

    if (erreur)
    {
        Serial.print("[Meteo] JSON incorrect : ");
        Serial.println(erreur.c_str());

        demandeEnCours = false;
        return false;
    }

    if (!document.is<JsonArray>())
    {
        Serial.println(
            "[Meteo] La réponse JSON n'est pas un tableau"
        );

        demandeEnCours = false;
        return false;
    }

    JsonArray resultats = document.as<JsonArray>();

    if (resultats.size() != NombrePoints)
    {
        Serial.printf(
            "[Meteo] Nombre de résultats incorrect : %u au lieu de %u\n",
            static_cast<unsigned int>(resultats.size()),
            static_cast<unsigned int>(NombrePoints)
        );

        demandeEnCours = false;
        return false;
    }

    uint8_t nombreValides = 0;

    for (uint8_t i = 0; i < NombrePoints; i++)
    {
        JsonObject resultatPoint =
            resultats[i].as<JsonObject>();

        if (resultatPoint.isNull())
        {
            points[i].valide = false;
            continue;
        }

        JsonObject donneesActuelles =
            resultatPoint["current"].as<JsonObject>();

        if (donneesActuelles.isNull()
            || donneesActuelles["wind_speed_10m"].isNull()
            || donneesActuelles["wind_direction_10m"].isNull()
            || donneesActuelles["cloud_cover"].isNull())
        {
            points[i].valide = false;
            continue;
        }

        const float vitesseVent =
            donneesActuelles["wind_speed_10m"].as<float>();

        const float directionVent =
            donneesActuelles["wind_direction_10m"].as<float>();

        const float couvertureNuageuse =
            donneesActuelles["cloud_cover"].as<float>();

        const bool vitesseCorrecte =
            vitesseVent >= 0.0f
            && vitesseVent <= 500.0f;

        const bool directionCorrecte =
            directionVent >= 0.0f
            && directionVent <= 360.0f;

        const bool nuagesCorrects =
            couvertureNuageuse >= 0.0f
            && couvertureNuageuse <= 100.0f;

        if (!vitesseCorrecte
            || !directionCorrecte
            || !nuagesCorrects)
        {
            points[i].valide = false;
            continue;
        }

        points[i].vitesseVent = vitesseVent;
        points[i].directionVent = directionVent;
        points[i].couvertureNuageuse = couvertureNuageuse;
        points[i].valide = true;

        nombreValides++;
    }

    donneesValides = nombreValides > 0;
    demandeEnCours = false;

    if (donneesValides)
        dateDernierSucces = millis();

    Serial.printf(
        "[Meteo] Points valides : %u/%u\n",
        static_cast<unsigned int>(nombreValides),
        static_cast<unsigned int>(NombrePoints)
    );

    Serial.printf(
        "[Meteo] Mémoire libre après analyse : %u octets\n",
        ESP.getFreeHeap()
    );

    return donneesValides;
}

bool GestionnaireMeteo::donneesDisponibles() const
{
    return donneesValides;
}

bool GestionnaireMeteo::miseAJourEnCours() const
{
    return demandeEnCours;
}

const PointMeteo& GestionnaireMeteo::obtenirPoint(
    uint8_t index
) const
{
    // Protection en cas d'indice incorrect.
    if (index >= NombrePoints)
        return points[0];

    return points[index];
}

uint8_t GestionnaireMeteo::obtenirNombrePoints() const
{
    return NombrePoints;
}

unsigned long GestionnaireMeteo::obtenirAgeDonnees() const
{
    if (!donneesValides)
        return 0;

    return millis() - dateDernierSucces;
}

void GestionnaireMeteo::invaliderDonnees()
{
    donneesValides = false;
    demandeEnCours = false;

    for (auto& point : points)
    {
        point.vitesseVent = 0.0f;
        point.directionVent = 0.0f;
        point.couvertureNuageuse = 0.0f;
        point.valide = false;
    }
}

bool GestionnaireMeteo::zoneModifiee(float latitudeCentre,
                                     float longitudeCentre,
                                     float rayonKm) const
{
    constexpr float epsilon = 0.0001f;

    return !zoneInitialisee
        || fabs(latitudeCentre - centreLatitude) > epsilon
        || fabs(longitudeCentre - centreLongitude) > epsilon
        || fabs(rayonKm - rayonZoneKm) > epsilon;
}

void GestionnaireMeteo::calculerGrille()
{
    constexpr float degresParKmLatitude =
        1.0f / KilometresParDegreLatitude;

    constexpr float FacteurHorizontal = 0.60f;
    constexpr float FacteurVertical = 0.80f;

    const float rayonHorizontalKm =
        rayonZoneKm * FacteurHorizontal;

    const float rayonVerticalKm =
        rayonZoneKm * FacteurVertical;

    const float pasHorizontalKm =
        (rayonHorizontalKm * 2.0f) / (TailleGrille - 1);

    const float pasVerticalKm =
        (rayonVerticalKm * 2.0f) / (TailleGrille - 1);

    float cosLatitude = cos(radians(centreLatitude));

    // Évite une division par une valeur proche de zéro
    // pour une utilisation éventuelle près des pôles.
    if (fabs(cosLatitude) < 0.01f)
    {
        cosLatitude =
            cosLatitude < 0.0f ? -0.01f : 0.01f;
    }

    const float degresParKmLongitude =
        1.0f
        / (KilometresParDegreLatitude * cosLatitude);

    uint8_t index = 0;

    for (uint8_t y = 0; y < TailleGrille; y++)
    {
        const float decalageNordSudKm =
            rayonVerticalKm - (y * pasVerticalKm);

        for (uint8_t x = 0; x < TailleGrille; x++)
        {
            const float decalageEstOuestKm =
                -rayonHorizontalKm + (x * pasHorizontalKm);

            points[index].latitude =
                centreLatitude
                + (decalageNordSudKm
                   * degresParKmLatitude);

            points[index].longitude =
                centreLongitude
                + (decalageEstOuestKm
                   * degresParKmLongitude);

            points[index].vitesseVent = 0.0f;
            points[index].directionVent = 0.0f;
            points[index].couvertureNuageuse = 0.0f;
            points[index].valide = false;

            index++;
        }
    }

    Serial.printf(
        "[Meteo] Grille créée : %u points\n",
        static_cast<unsigned int>(NombrePoints)
    );
}

namespace
{
    void dessinerFleche(
        LGFX_Sprite& backbuffer,
        int x,
        int y,
        float directionDegres,
        float vitesseVent,
        uint32_t couleur
    )
    {
        // Longueur comprise entre 10 et 18 pixels
        const float longueur =
            constrain(
                10.0f + vitesseVent * 0.15f,
                10.0f,
                18.0f
            );

        constexpr float longueurPointe = 5.0f;
        constexpr float largeurPointe = 3.5f;

        // Open-Meteo indique la provenance du vent.
        // La flèche montre ici la direction vers laquelle il souffle.
        const float angle =
            radians(directionDegres + 180.0f);

        const float dx = sin(angle);
        const float dy = -cos(angle);

        // Vecteur perpendiculaire, utilisé pour épaissir le trait
        const float px = -dy;
        const float py = dx;

        // La flèche est centrée sur le point météo
        const float demiLongueur = longueur * 0.5f;

        const float debutX = x - dx * demiLongueur;
        const float debutY = y - dy * demiLongueur;

        const float pointeX = x + dx * demiLongueur;
        const float pointeY = y + dy * demiLongueur;

        // Le trait s’arrête avant la pointe
        const float baseX =
            pointeX - dx * longueurPointe;

        const float baseY =
            pointeY - dy * longueurPointe;

        // Trait principal épaissi
        backbuffer.drawLine(
            static_cast<int>(debutX),
            static_cast<int>(debutY),
            static_cast<int>(baseX),
            static_cast<int>(baseY),
            couleur
        );

        backbuffer.drawLine(
            static_cast<int>(debutX + px),
            static_cast<int>(debutY + py),
            static_cast<int>(baseX + px),
            static_cast<int>(baseY + py),
            couleur
        );

        // Pointe triangulaire pleine
        const float gaucheX =
            baseX + px * largeurPointe;

        const float gaucheY =
            baseY + py * largeurPointe;

        const float droiteX =
            baseX - px * largeurPointe;

        const float droiteY =
            baseY - py * largeurPointe;

        backbuffer.fillTriangle(
            static_cast<int>(pointeX),
            static_cast<int>(pointeY),
            static_cast<int>(gaucheX),
            static_cast<int>(gaucheY),
            static_cast<int>(droiteX),
            static_cast<int>(droiteY),
            couleur
        );

        // Petit point à l’origine de la mesure
        backbuffer.fillCircle(
            x,
            y,
            1,
            couleur
        );
    }
}

void GestionnaireMeteo::dessinerVent(
    LGFX_Sprite& backbuffer,
    const AffichageRadar& radar
) const
{
    for (uint8_t index = 0; index < NombrePoints; index++)
    {
        const PointMeteo& point = points[index];

        if (!point.valide)
            continue;

        const uint8_t ligne = index / TailleGrille;
        const uint8_t colonne = index % TailleGrille;

        const bool estUnCoin =
            (ligne == 0 || ligne == TailleGrille - 1)
            &&
            (colonne == 0 || colonne == TailleGrille - 1);

        if (estUnCoin)
            continue;

        auto [x, y] = radar.projeterCoordonnees(
            point.latitude,
            point.longitude
        );

        if (!radar.pointDansEcran(x, y))
            continue;

        uint32_t couleur;

        if (point.vitesseVent < 15.0f)
            couleur = lgfx::color888(0, 220, 0);
        else if (point.vitesseVent < 30.0f)
            couleur = lgfx::color888(255, 220, 0);
        else if (point.vitesseVent < 50.0f)
            couleur = lgfx::color888(255, 140, 0);
        else
            couleur = lgfx::color888(255, 0, 0);

        dessinerFleche(
            backbuffer,
            x,
            y,
            point.directionVent,
            point.vitesseVent,
            couleur
        );
    }
}