#include "AffichageRadar.h"

void AffichageRadar::definirZone(double nouvelleLatitudeCentre,
                                 double nouvelleLongitudeCentre,
                                 double nouveauRayonDegres)
{
    latitudeCentre = nouvelleLatitudeCentre;
    longitudeCentre = nouvelleLongitudeCentre;

    if (nouveauRayonDegres > 0.0)
        rayonDegres = nouveauRayonDegres;
}

void AffichageRadar::definirTheme(RadarTheme nouveauTheme)
{
    theme = nouveauTheme;
}

void AffichageRadar::definirCercles(float rayon1Km,
                                    float rayon2Km,
                                    float rayon3Km)
{
    rayonsCerclesKm[0] = rayon1Km;
    rayonsCerclesKm[1] = rayon2Km;
    rayonsCerclesKm[2] = rayon3Km;
}

void AffichageRadar::dessinerFond(LGFX_Sprite& tampon,
                                  bool afficherNord) const
{
    dessinerCercles(tampon);

    if (afficherNord)
        dessinerFlecheNord(tampon);
}

std::pair<int, int> AffichageRadar::projeterCoordonnees(
    double latitude,
    double longitude
) const
{
    if (rayonDegres <= 0.0)
        return {CentreEcran, CentreEcran};

    const double differenceLongitude =
        longitude - longitudeCentre;

    const double differenceLatitude =
        latitude - latitudeCentre;

    const double longitudeNormalisee =
        (differenceLongitude + rayonDegres)
        / (2.0 * rayonDegres);

    const double latitudeNormalisee =
        (differenceLatitude + rayonDegres)
        / (2.0 * rayonDegres);

    const int x = static_cast<int>(
        longitudeNormalisee * TailleEcran
    );

    const int y = static_cast<int>(
        TailleEcran
        - (latitudeNormalisee * TailleEcran)
    );

    return {x, y};
}

bool AffichageRadar::pointDansEcran(int x, int y) const
{
    if (x < 0 || x >= TailleEcran)
        return false;

    if (y < 0 || y >= TailleEcran)
        return false;

    const int differenceX = x - CentreEcran;
    const int differenceY = y - CentreEcran;

    const int distanceCarree =
        differenceX * differenceX
        + differenceY * differenceY;

    const int rayonCarre =
        RayonExterieur * RayonExterieur;

    return distanceCarree <= rayonCarre;
}

double AffichageRadar::obtenirLatitudeCentre() const
{
    return latitudeCentre;
}

double AffichageRadar::obtenirLongitudeCentre() const
{
    return longitudeCentre;
}

double AffichageRadar::obtenirRayonDegres() const
{
    return rayonDegres;
}

void AffichageRadar::dessinerCercles(
    LGFX_Sprite& tampon
) const
{
    tampon.drawCircle(
        CentreEcran,
        CentreEcran,
        RayonExterieur,
        ThemeBaseColor(theme, 200)
    );

    if (rayonDegres <= 0.0)
        return;

    constexpr float KilometresParDegre = 111.32f;

    const float porteeTotaleKm =
        static_cast<float>(rayonDegres)
        * KilometresParDegre;

    if (porteeTotaleKm <= 0.0f)
        return;

    const uint32_t couleurCercle =
        lgfx::color888(150, 150, 150);

    const uint32_t couleurTexte =
        lgfx::color888(190, 190, 190);

    for (uint8_t i = 0; i < 3; i++)
    {
        const float distanceKm = rayonsCerclesKm[i];

        if (distanceKm <= 0.0f)
            continue;

        if (distanceKm >= porteeTotaleKm)
            continue;

        const float rapport =
            distanceKm / porteeTotaleKm;

        const int rayonCercle =
            static_cast<int>(
                RayonExterieur * rapport
            );

        if (rayonCercle <= 0
            || rayonCercle >= RayonExterieur)
        {
            continue;
        }

        tampon.drawCircle(
            CentreEcran,
            CentreEcran,
            rayonCercle,
            couleurCercle
        );

        tampon.setTextSize(1);
        tampon.setTextColor(couleurTexte);

        const String etiquette =
            distanceKm == static_cast<int>(distanceKm)
            ? String(static_cast<int>(distanceKm))
            : String(distanceKm, 1);

        const int positionX =
            CentreEcran + rayonCercle + 2;

        const int positionY =
            CentreEcran - 6;

        tampon.drawString(
            etiquette,
            positionX,
            positionY
        );

        tampon.drawString(
            "km",
            positionX,
            positionY + tampon.fontHeight()
        );
    }
}

void AffichageRadar::dessinerFlecheNord(
    LGFX_Sprite& tampon
) const
{
    constexpr int DistanceFleche = 80;
    constexpr int TailleFleche = 8;

    const int positionX = CentreEcran;
    const int positionY =
        CentreEcran - DistanceFleche;

    const uint32_t couleur =
        lgfx::color888(200, 200, 200);

    const int pointeX = positionX;
    const int pointeY =
        positionY - TailleFleche;

    const int baseGaucheX =
        positionX - TailleFleche / 2;

    const int baseDroiteX =
        positionX + TailleFleche / 2;

    tampon.fillTriangle(
        pointeX,
        pointeY,
        baseGaucheX,
        positionY,
        baseDroiteX,
        positionY,
        couleur
    );

    tampon.setTextSize(1);
    tampon.setTextColor(couleur);

    tampon.drawString(
        "N",
        positionX - 2,
        positionY + TailleFleche + 2
    );
}