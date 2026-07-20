#pragma once

#include <LovyanGFX.hpp>
#include <algorithm>

// Thèmes de couleurs disponibles pour l'affichage du radar.
enum class RadarTheme
{
    Green,     // vert radar militaire classique (thème d'origine)
    Cyan,      // bleu/cyan façon HUD moderne
    Amber,     // ambre/orange façon vieux radar phosphore
    Altitude   // couleur des avions dégradée selon leur altitude
};

inline RadarTheme ParseTheme(const String& s)
{
    if (s == "cyan")     return RadarTheme::Cyan;
    if (s == "amber")    return RadarTheme::Amber;
    if (s == "altitude") return RadarTheme::Altitude;
    return RadarTheme::Green;
}

inline const char* ThemeToString(RadarTheme theme)
{
    switch (theme) {
        case RadarTheme::Cyan:     return "cyan";
        case RadarTheme::Amber:    return "amber";
        case RadarTheme::Altitude: return "altitude";
        case RadarTheme::Green:
        default:                   return "green";
    }
}

// Couleur de base utilisée pour les éléments d'interface communs
// (cercles du radar, balayage, texte) selon le thème choisi.
// `brightness` (0-255) permet de faire varier l'intensité, par ex. pour les
// anneaux intérieurs plus sombres ou le dégradé du balayage.
inline uint32_t ThemeBaseColor(RadarTheme theme, uint8_t brightness = 255)
{
    switch (theme) {
        case RadarTheme::Cyan:
            return lgfx::color888(0, brightness, brightness);
        case RadarTheme::Amber:
            return lgfx::color888(brightness, (uint8_t)(brightness * 0.6f), 0);
        case RadarTheme::Altitude:
            // Le thème "altitude" garde une interface neutre légèrement
            // bleutée ; ce sont les avions qui portent la couleur.
            return lgfx::color888((uint8_t)(brightness * 0.55f), (uint8_t)(brightness * 0.75f), brightness);
        case RadarTheme::Green:
        default:
            return lgfx::color888(0, brightness, 0);
    }
}

// Couleur d'un avion selon le thème choisi. Pour les thèmes unis, renvoie la
// couleur de base à pleine intensité. Pour le thème "altitude", renvoie un
// dégradé rouge (bas) -> orange -> vert -> bleu (haut) selon l'altitude
// barométrique en mètres (plafond de croisière typique ~12000m).
inline uint32_t AircraftColor(RadarTheme theme, float altitudeMeters, const String& callsign = String())
{
    if (theme == RadarTheme::Altitude && callsign.length() > 0) {
        String cs = callsign;
        cs.trim();            // remove leading/trailing spaces
        cs.toUpperCase();     // normalize case
        if (cs.startsWith("SAMU"))
            return lgfx::color888(255, 0, 0);
    }

    if (theme != RadarTheme::Altitude)
        return ThemeBaseColor(theme, 255);

    struct Stop { float pos; uint8_t r, g, b; };
    static const Stop stops[4] = {
        {0.00f, 255,  60,  40},  // basse altitude - rouge
        {0.33f, 255, 170,  30},  // orange
        {0.66f,  60, 220,  90},  // vert
        {1.00f,  70, 170, 255},  // haute altitude - bleu
    };

    const float t = std::clamp(altitudeMeters / 12000.0f, 0.0f, 1.0f);

    for (int i = 0; i < 3; ++i) {
        if (t >= stops[i].pos && t <= stops[i + 1].pos) {
            const float localT = (t - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            const uint8_t r = stops[i].r + (uint8_t)((stops[i + 1].r - stops[i].r) * localT);
            const uint8_t g = stops[i].g + (uint8_t)((stops[i + 1].g - stops[i].g) * localT);
            const uint8_t b = stops[i].b + (uint8_t)((stops[i + 1].b - stops[i].b) * localT);
            return lgfx::color888(r, g, b);
        }
    }
    return lgfx::color888(stops[3].r, stops[3].g, stops[3].b);
}
