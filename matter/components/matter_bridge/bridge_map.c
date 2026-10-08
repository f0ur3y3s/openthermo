/**
 * @file  bridge_map.c
 * @brief Pure thermostat <-> Matter value mapping. See bridge_map.h.
 */
#include "bridge_map.h"

#include "hvac_logic.h"
#include <stddef.h>

#define F10_FREEZING    320 // 32.0 F
#define C100_PER_F10    50  // 1 F10 step = 50/9 C100
#define F10_PER_C100    9
#define F10_PER_F       10 // the thermostat's step in F: one degree
#define C100_PER_HALF_C 50 // its step in C: half a degree

// num / den, rounded to nearest with halves away from zero. den > 0.
static int32_t bridge_div_round(int32_t num, int32_t den)
{
    int32_t result = 0;

    if (num >= 0)
    {
        result = (num + (den / 2)) / den;
    }
    else
    {
        result = (num - (den / 2)) / den;
    }

    return result;
}

static int16_t bridge_saturate_i16(int32_t value)
{
    int32_t clamped = value;

    if (clamped > INT16_MAX)
    {
        clamped = INT16_MAX;
    }
    else if (clamped < INT16_MIN)
    {
        clamped = INT16_MIN;
    }
    else
    {
        // In range.
    }

    // Clamped to the int16_t range just above.
    return (int16_t)clamped;
}

int16_t bridge_map_f10_to_c100(int16_t f10)
{
    return bridge_saturate_i16(bridge_div_round(
        ((int32_t)f10 - F10_FREEZING) * C100_PER_F10, F10_PER_C100));
}

int16_t bridge_map_c100_to_f10(int16_t c100)
{
    return bridge_saturate_i16(
        bridge_div_round((int32_t)c100 * F10_PER_C100, C100_PER_F10) +
        F10_FREEZING);
}

int16_t bridge_map_setpoint_from_c100(int16_t c100, bool b_celsius)
{
    int16_t f10 = 0;

    if (b_celsius)
    {
        f10 = bridge_map_c100_to_f10(bridge_saturate_i16(
            bridge_div_round(c100, C100_PER_HALF_C) * C100_PER_HALF_C));
    }
    else
    {
        // Straight to whole degrees F, rounding once: going through tenths
        // first would round twice and could tip a value just under half a
        // degree off onto the next degree.
        // F = (c100 * 9 / 50 + 320) / 10 = (c100 * 9 + 320 * 50) / 500.
        f10 = bridge_saturate_i16(
            bridge_div_round(((int32_t)c100 * F10_PER_C100) +
                                 (F10_FREEZING * C100_PER_F10),
                             C100_PER_F10 * F10_PER_F) *
            F10_PER_F);
    }

    return f10;
}

void bridge_map_setpoint_write(int16_t c100, bool b_celsius, bool b_heat,
                               int16_t * p_heat_f10, int16_t * p_cool_f10)
{
    if ((NULL == p_heat_f10) || (NULL == p_cool_f10))
    {
        goto done;
    }

    if (b_heat)
    {
        *p_heat_f10 = bridge_map_setpoint_from_c100(c100, b_celsius);
    }
    else
    {
        *p_cool_f10 = bridge_map_setpoint_from_c100(c100, b_celsius);
    }
    // The snap alone can land just outside a range in Celsius (15.56 C
    // snaps to 15.5 C, 59.9 F); this clamps it and keeps the deadband.
    hvac_setpoints_apply(p_heat_f10, p_cool_f10, b_heat);

done:
    return;
}

void bridge_map_limits(bridge_limits_t * p_limits)
{
    if (NULL != p_limits)
    {
        // Exactly as a reported setpoint converts: the conversion is
        // monotonic, so every setpoint inside the thermostat's ranges
        // reports inside these, and a write inside them comes back inside
        // the ranges after bridge_map_setpoint_write().
        p_limits->min_heat_c100 = bridge_map_f10_to_c100(HVAC_HEAT_SP_MIN_F10);
        p_limits->max_heat_c100 = bridge_map_f10_to_c100(HVAC_HEAT_SP_MAX_F10);
        p_limits->min_cool_c100 = bridge_map_f10_to_c100(HVAC_COOL_SP_MIN_F10);
        p_limits->max_cool_c100 = bridge_map_f10_to_c100(HVAC_COOL_SP_MAX_F10);
    }
}

bool bridge_map_attr_fixed(uint32_t cluster_id, uint32_t attribute_id)
{
    bool b_fixed = false;

    if (BRIDGE_CLUSTER_THERMOSTAT == cluster_id)
    {
        b_fixed = ((BRIDGE_ATTR_MIN_HEAT_LIMIT == attribute_id) ||
                   (BRIDGE_ATTR_MAX_HEAT_LIMIT == attribute_id) ||
                   (BRIDGE_ATTR_MIN_COOL_LIMIT == attribute_id) ||
                   (BRIDGE_ATTR_MAX_COOL_LIMIT == attribute_id) ||
                   (BRIDGE_ATTR_MIN_DEADBAND == attribute_id) ||
                   (BRIDGE_ATTR_CONTROL_SEQUENCE == attribute_id));
    }
    else if (BRIDGE_CLUSTER_ON_OFF == cluster_id)
    {
        b_fixed = (BRIDGE_ATTR_START_UP_ON_OFF == attribute_id);
    }
    else
    {
        // Nothing else is fixed.
    }

    return b_fixed;
}

uint8_t bridge_map_system_mode(uint8_t hvac_mode)
{
    uint8_t system_mode = BRIDGE_SYSTEM_MODE_OFF;

    switch (hvac_mode)
    {
        case HVAC_MODE_HEAT:
        case HVAC_MODE_EHEAT:
            system_mode = BRIDGE_SYSTEM_MODE_HEAT;
            break;
        case HVAC_MODE_COOL:
            system_mode = BRIDGE_SYSTEM_MODE_COOL;
            break;
        case HVAC_MODE_AUTO:
            system_mode = BRIDGE_SYSTEM_MODE_AUTO;
            break;
        default:
            // Off, or a value the thermostat would treat as Off.
            break;
    }

    return system_mode;
}

bool bridge_map_mode_from_system(uint8_t system_mode, uint8_t current_mode,
                                 uint8_t * p_mode)
{
    bool    b_ok = true;
    uint8_t mode = (uint8_t)HVAC_MODE_OFF; // 0 fits

    if (NULL == p_mode)
    {
        b_ok = false;
        goto done;
    }

    // The enumerators are all below 5, so every cast below is lossless.
    switch (system_mode)
    {
        case BRIDGE_SYSTEM_MODE_OFF:
            mode = (uint8_t)HVAC_MODE_OFF;
            break;
        case BRIDGE_SYSTEM_MODE_AUTO:
            mode = (uint8_t)HVAC_MODE_AUTO;
            break;
        case BRIDGE_SYSTEM_MODE_COOL:
            mode = (uint8_t)HVAC_MODE_COOL;
            break;
        case BRIDGE_SYSTEM_MODE_HEAT:
            // E-heat ignores Heat: only its own switch turns it off.
            mode = ((uint8_t)HVAC_MODE_EHEAT == current_mode)
                       ? (uint8_t)HVAC_MODE_EHEAT
                       : (uint8_t)HVAC_MODE_HEAT;
            break;
        case BRIDGE_SYSTEM_MODE_EHEAT:
            mode = (uint8_t)HVAC_MODE_EHEAT;
            break;
        default:
            b_ok = false; // precooling, fan only, sleep and the rest
            break;
    }

    if (b_ok)
    {
        *p_mode = mode;
    }

done:
    return b_ok;
}

uint8_t bridge_map_mode_from_eheat(bool b_on, uint8_t current_mode,
                                   uint8_t restore_mode)
{
    uint8_t mode = current_mode;

    // The enumerators are all below 5, so the casts are lossless.
    if (b_on)
    {
        mode = (uint8_t)HVAC_MODE_EHEAT;
    }
    else if ((uint8_t)HVAC_MODE_EHEAT == current_mode)
    {
        mode = (uint8_t)HVAC_MODE_HEAT;
        if (((uint8_t)HVAC_MODE_OFF == restore_mode) ||
            ((uint8_t)HVAC_MODE_COOL == restore_mode) ||
            ((uint8_t)HVAC_MODE_AUTO == restore_mode))
        {
            mode = restore_mode;
        }
    }
    else
    {
        // Already not in e-heat: nothing to undo.
    }

    return mode;
}

uint8_t bridge_map_fan_mode(uint8_t hvac_fan)
{
    return ((uint8_t)HVAC_FAN_ON == hvac_fan) ? BRIDGE_FAN_MODE_HIGH
                                              : BRIDGE_FAN_MODE_AUTO;
}

uint8_t bridge_map_fan_from_mode(uint8_t fan_mode)
{
    uint8_t fan = (uint8_t)HVAC_FAN_AUTO; // 0 fits

    switch (fan_mode)
    {
        case BRIDGE_FAN_MODE_LOW:
        case BRIDGE_FAN_MODE_MEDIUM:
        case BRIDGE_FAN_MODE_HIGH:
        case BRIDGE_FAN_MODE_ON:
            fan = (uint8_t)HVAC_FAN_ON; // 1 fits
            break;
        default:
            // Off, Auto, Smart: the blower follows the calls.
            break;
    }

    return fan;
}

uint16_t bridge_map_running_state(bool b_y1, bool b_g, bool b_o, bool b_w)
{
    uint16_t state = 0U;

    if ((b_y1 && !b_o) || b_w)
    {
        state |= BRIDGE_RUNNING_HEAT;
    }
    if (b_y1 && b_o)
    {
        state |= BRIDGE_RUNNING_COOL;
    }
    if (b_g)
    {
        state |= BRIDGE_RUNNING_FAN;
    }
    if (b_y1 && b_w)
    {
        state |= BRIDGE_RUNNING_HEAT_STAGE2;
    }

    return state;
}
