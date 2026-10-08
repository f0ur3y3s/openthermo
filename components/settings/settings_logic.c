/**
 * @file  settings_logic.c
 * @brief Pure settings logic. See settings_logic.h.
 */
#include "settings_logic.h"

#include "hvac_logic.h"
#include "settings_v1.h"
#include <stddef.h>
#include <string.h>

#define DEFAULT_HEAT_SP_F10       680 // 68.0 F
#define DEFAULT_COOL_SP_F10       760 // 76.0 F
#define DEFAULT_FAN_PURGE_S       60U
#define DEFAULT_DISPLAY_TIMEOUT_S 30U
#define DEFAULT_SCREEN_OFF_S      600U // 10 min
#define DEFAULT_BRIGHTNESS        200U

// The blob layout is the on-flash format: pin it (rule 10).
_Static_assert(sizeof(settings_t) == 18U, "settings_t v2 layout changed");
_Static_assert(offsetof(settings_t, version) == 0U, "version moved");
_Static_assert(offsetof(settings_t, heat_sp_f10) == 2U, "heat_sp moved");
_Static_assert(offsetof(settings_t, cool_sp_f10) == 4U, "cool_sp moved");
_Static_assert(offsetof(settings_t, cal_offset_f10) == 6U, "cal moved");
_Static_assert(offsetof(settings_t, fan_purge_s) == 8U, "purge moved");
_Static_assert(offsetof(settings_t, display_timeout_s) == 10U, "dim moved");
_Static_assert(offsetof(settings_t, screen_off_s) == 12U, "off moved");
_Static_assert(offsetof(settings_t, mode) == 14U, "mode moved");
_Static_assert(offsetof(settings_t, fan) == 15U, "fan moved");
_Static_assert(offsetof(settings_t, units) == 16U, "units moved");
_Static_assert(offsetof(settings_t, brightness) == 17U, "brightness moved");

// The frozen v1 layout must stay exactly as v1 firmware wrote it.
_Static_assert(sizeof(settings_v1_t) == 16U, "settings_v1_t changed");
_Static_assert(offsetof(settings_v1_t, display_timeout_s) == 10U,
               "v1 dim moved");
_Static_assert(offsetof(settings_v1_t, mode) == 12U, "v1 mode moved");
_Static_assert(offsetof(settings_v1_t, brightness) == 15U,
               "v1 brightness moved");
_Static_assert(DEFAULT_SCREEN_OFF_S <= SETTINGS_SCREEN_OFF_MAX,
               "default screen-off out of range");

// The defaults must themselves pass the checks below.
_Static_assert((DEFAULT_HEAT_SP_F10 + HVAC_AUTO_DEADBAND_F10) <=
                   DEFAULT_COOL_SP_F10,
               "default setpoints break the deadband");
_Static_assert(DEFAULT_FAN_PURGE_S <= HVAC_FAN_PURGE_MAX_S,
               "default purge out of range");
_Static_assert(HVAC_MODE_COUNT <= 255, "mode must fit uint8_t");

void settings_logic_defaults(settings_t * p_cfg)
{
    if (NULL == p_cfg)
    {
        goto done;
    }

    memset(p_cfg, 0, sizeof(*p_cfg));
    p_cfg->version           = SETTINGS_VERSION;
    p_cfg->heat_sp_f10       = DEFAULT_HEAT_SP_F10;
    p_cfg->cool_sp_f10       = DEFAULT_COOL_SP_F10;
    p_cfg->cal_offset_f10    = 0;
    p_cfg->fan_purge_s       = DEFAULT_FAN_PURGE_S;
    p_cfg->display_timeout_s = DEFAULT_DISPLAY_TIMEOUT_S;
    p_cfg->screen_off_s      = DEFAULT_SCREEN_OFF_S;
    p_cfg->mode              = (uint8_t)HVAC_MODE_OFF;    // 0 fits
    p_cfg->fan               = (uint8_t)HVAC_FAN_AUTO;    // 0 fits
    p_cfg->units             = (uint8_t)SETTINGS_UNITS_F; // 0 fits
    p_cfg->brightness        = DEFAULT_BRIGHTNESS;

done:
    return;
}

uint32_t settings_logic_sanitise(settings_t * p_cfg)
{
    uint32_t   fixed = 0U;
    settings_t dflt  = { 0 };

    if (NULL == p_cfg)
    {
        goto done;
    }

    settings_logic_defaults(&dflt);
    p_cfg->version = SETTINGS_VERSION;

    if (!hvac_setpoints_valid(p_cfg->heat_sp_f10, p_cfg->cool_sp_f10))
    {
        p_cfg->heat_sp_f10 = dflt.heat_sp_f10;
        p_cfg->cool_sp_f10 = dflt.cool_sp_f10;
        fixed++;
    }
    if ((p_cfg->cal_offset_f10 < -SETTINGS_CAL_LIMIT_F10) ||
        (p_cfg->cal_offset_f10 > SETTINGS_CAL_LIMIT_F10))
    {
        p_cfg->cal_offset_f10 = dflt.cal_offset_f10;
        fixed++;
    }
    if (p_cfg->fan_purge_s > HVAC_FAN_PURGE_MAX_S)
    {
        p_cfg->fan_purge_s = dflt.fan_purge_s;
        fixed++;
    }
    if (p_cfg->display_timeout_s > SETTINGS_DISPLAY_TIMEOUT_MAX)
    {
        p_cfg->display_timeout_s = dflt.display_timeout_s;
        fixed++;
    }
    if (p_cfg->screen_off_s > SETTINGS_SCREEN_OFF_MAX)
    {
        p_cfg->screen_off_s = dflt.screen_off_s;
        fixed++;
    }
    if (p_cfg->mode >= (uint8_t)HVAC_MODE_COUNT) // asserted to fit above
    {
        p_cfg->mode = dflt.mode;
        fixed++;
    }
    if (p_cfg->fan >= (uint8_t)HVAC_FAN_COUNT) // 2 fits
    {
        p_cfg->fan = dflt.fan;
        fixed++;
    }
    if (p_cfg->units >= (uint8_t)SETTINGS_UNITS_COUNT) // 2 fits
    {
        p_cfg->units = dflt.units;
        fixed++;
    }
    if (p_cfg->brightness < SETTINGS_BRIGHTNESS_MIN)
    {
        p_cfg->brightness = dflt.brightness;
        fixed++;
    }

done:
    return fixed;
}

uint32_t settings_logic_from_blob(void const * p_blob, size_t len,
                                  settings_t * p_out)
{
    uint32_t      found   = 0U;
    uint16_t      version = 0U;
    settings_t    cfg     = { 0 };
    settings_v1_t v1      = { 0 };

    if ((NULL == p_blob) || (NULL == p_out) || (len < sizeof(version)))
    {
        goto done;
    }

    // The version is the first field of every layout. memcpy, because the
    // blob need not be aligned for any of them.
    (void)memcpy(&version, p_blob, sizeof(version));

    if ((SETTINGS_VERSION == version) && (sizeof(cfg) == len))
    {
        (void)memcpy(&cfg, p_blob, sizeof(cfg));
        found = SETTINGS_VERSION;
    }
    else if ((SETTINGS_V1_VERSION == version) && (sizeof(v1) == len))
    {
        (void)memcpy(&v1, p_blob, sizeof(v1));
        settings_logic_defaults(&cfg); // fields new since v1
        cfg.heat_sp_f10       = v1.heat_sp_f10;
        cfg.cool_sp_f10       = v1.cool_sp_f10;
        cfg.cal_offset_f10    = v1.cal_offset_f10;
        cfg.fan_purge_s       = v1.fan_purge_s;
        cfg.display_timeout_s = v1.display_timeout_s;
        cfg.mode              = v1.mode;
        cfg.fan               = v1.fan;
        cfg.units             = v1.units;
        cfg.brightness        = v1.brightness;
        found                 = SETTINGS_V1_VERSION;
    }
    else
    {
        // Unknown version or size: the caller uses the defaults.
    }

    if (0U != found)
    {
        *p_out = cfg;
    }

done:
    return found;
}
