#pragma once

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_GC9A01 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.freq_write = 27000000;
            cfg.pin_miso = -1;
            cfg.pin_mosi = 6;   // ton câblage : MOSI/SDA -> GPIO6
            cfg.pin_sclk = 4;   // ton câblage : SCLK -> GPIO4
            cfg.pin_dc = 10;    // ton câblage : DC -> GPIO10
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs = 7;     // ton câblage : CS -> GPIO7
            cfg.pin_rst = 3;    // ton câblage : RST -> GPIO3
            cfg.pin_busy = -1;
            // cfg.rgb_order = true;
            _panel.config(cfg);
        }
        {
            // Pas de pin de rétro-éclairage dédié : BLK est câblé directement en 3V3.
            // On ne configure pas _light / setLight() dans ce cas.
        }
        setPanel(&_panel);
    }
};