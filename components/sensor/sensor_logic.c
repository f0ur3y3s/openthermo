/**
 * @file  sensor_logic.c
 * @brief Pure sensor logic. See sensor_logic.h.
 */
#include "sensor_logic.h"

#include <stddef.h>
#include <string.h>

#define CRC8_POLY 0x31U
#define CRC8_INIT 0xFFU
#define CRC8_MSB  0x80U
#define BITS      8U

#define RAW_FULL_SCALE 65535
#define RAW_HALF_SCALE 32767 // added before dividing, to round

// SHT4x datasheet: T(F) = -49 + 315 * raw / 65535, RH = -6 + 125 * raw / 65535.
// In tenths, the products stay below 2^31 for any 16-bit raw value.
#define TEMP_OFFSET_F10 (-490)
#define TEMP_SPAN_F10   3150
#define RH_OFFSET_X10   (-60)
#define RH_SPAN_X10     1250
#define RH_MAX_X10      1000

#define MILLI 1000

// The lag's time constant as a signed value for the step arithmetic.
#define TAU_MS ((int64_t)SENSOR_SELF_HEAT_TAU_MS)

static int16_t sensor_saturate_i16(int32_t value)
{
    int32_t clamped = value;

    if (clamped < INT16_MIN)
    {
        clamped = INT16_MIN;
    }
    else if (clamped > INT16_MAX)
    {
        clamped = INT16_MAX;
    }
    else
    {
        // In range.
    }

    // Clamped to the int16_t range just above.
    return (int16_t)clamped;
}

// Scales a 16-bit raw value onto offset + span * raw / 65535, rounded.
static int32_t sensor_scale(uint16_t raw, int32_t offset, int32_t span)
{
    return offset + (((span * (int32_t)raw) + RAW_HALF_SCALE) / RAW_FULL_SCALE);
}

uint8_t sensor_crc8(uint8_t const * p_data, uint32_t len)
{
    uint8_t  crc      = CRC8_INIT;
    uint32_t byte_idx = 0U;
    uint32_t bit_idx  = 0U;

    if (NULL == p_data)
    {
        goto done;
    }

    for (byte_idx = 0U; byte_idx < len; byte_idx++)
    {
        crc ^= p_data[byte_idx];
        for (bit_idx = 0U; bit_idx < BITS; bit_idx++)
        {
            // Shifting a uint8_t left promotes to int; the mask keeps 8 bits.
            if (0U != (crc & CRC8_MSB))
            {
                crc = (uint8_t)(((uint32_t)crc << 1U) ^ CRC8_POLY);
            }
            else
            {
                crc = (uint8_t)((uint32_t)crc << 1U);
            }
        }
    }

done:
    return crc;
}

bool sensor_temp_plausible(int16_t temp_f10)
{
    return ((temp_f10 >= SENSOR_PLAUSIBLE_MIN_F10) &&
            (temp_f10 <= SENSOR_PLAUSIBLE_MAX_F10));
}

bool sensor_decode(uint8_t const * p_frame, int16_t * p_temp_f10,
                   int16_t * p_rh_x10)
{
    bool     b_ok   = false;
    uint16_t raw_t  = 0U;
    uint16_t raw_rh = 0U;
    int32_t  rh     = 0;
    int16_t  temp   = 0;

    if ((NULL == p_frame) || (NULL == p_temp_f10) || (NULL == p_rh_x10))
    {
        goto done;
    }
    if ((sensor_crc8(&p_frame[0], 2U) != p_frame[2]) ||
        (sensor_crc8(&p_frame[3], 2U) != p_frame[5]))
    {
        goto done;
    }

    // Two bytes assembled big-endian always fit 16 bits.
    raw_t  = (uint16_t)(((uint32_t)p_frame[0] << BITS) | p_frame[1]);
    raw_rh = (uint16_t)(((uint32_t)p_frame[3] << BITS) | p_frame[4]);

    rh = sensor_scale(raw_rh, RH_OFFSET_X10, RH_SPAN_X10);
    if (rh < 0)
    {
        rh = 0;
    }
    else if (rh > RH_MAX_X10)
    {
        rh = RH_MAX_X10;
    }
    else
    {
        // In range.
    }

    // -490..2660 for any raw value, so well inside int16_t.
    temp = sensor_saturate_i16(
        sensor_scale(raw_t, TEMP_OFFSET_F10, TEMP_SPAN_F10));
    if (!sensor_temp_plausible(temp))
    {
        goto done;
    }

    *p_temp_f10 = temp;
    *p_rh_x10   = sensor_saturate_i16(rh);
    b_ok        = true;

done:
    return b_ok;
}

void sensor_logic_init(sensor_logic_t * p_logic, uint64_t now_ms)
{
    if (NULL == p_logic)
    {
        goto done;
    }

    memset(p_logic, 0, sizeof(*p_logic));
    p_logic->last_heat_ms = now_ms;

done:
    return;
}

void sensor_logic_reading(sensor_logic_t * p_logic, int16_t temp_f10,
                          int16_t rh_x10, uint64_t now_ms)
{
    if (NULL == p_logic)
    {
        goto done;
    }

    if (p_logic->b_have_reading)
    {
        if ((now_ms < p_logic->last_good_ms) ||
            ((now_ms - p_logic->last_good_ms) >= SENSOR_FAULT_MS))
        {
            // The fault was declared (or the clock is not to be trusted):
            // recover only after an unbroken run from here.
            p_logic->b_recovering  = true;
            p_logic->good_since_ms = now_ms;
        }
        else if ((now_ms - p_logic->last_good_ms) > SENSOR_GAP_MS)
        {
            p_logic->good_since_ms = now_ms; // a missed read breaks the run
        }
        else if (p_logic->b_recovering &&
                 ((now_ms - p_logic->good_since_ms) >= SENSOR_RECOVER_MS))
        {
            p_logic->b_recovering = false; // recovered
        }
        else
        {
            // The run continues.
        }
    }
    else
    {
        p_logic->good_since_ms = now_ms; // first reading: no recovery needed
    }

    p_logic->b_have_reading = true;
    p_logic->last_good_ms   = now_ms;
    p_logic->raw_f10        = temp_f10;
    p_logic->rh_x10         = rh_x10;

done:
    return;
}

void sensor_logic_self_heat(sensor_logic_t * p_logic, uint8_t relays_on,
                            int16_t per_relay_f10, uint64_t now_ms)
{
    int64_t  target = 0;
    int64_t  step   = 0;
    uint64_t dt_ms  = 0U;

    if (NULL == p_logic)
    {
        goto done;
    }

    target = (int64_t)relays_on * per_relay_f10 * MILLI;
    if (now_ms > p_logic->last_heat_ms)
    {
        dt_ms = now_ms - p_logic->last_heat_ms;
    }
    p_logic->last_heat_ms = now_ms;

    if (dt_ms >= SENSOR_SELF_HEAT_TAU_MS)
    {
        // A gap longer than the time constant: assume it has settled.
        // At most 4 * 32767 * 1000, which fits int32_t.
        p_logic->heat_milli_f10 = (int32_t)target;
    }
    else
    {
        // Each step covers dt/tau of the distance to target, rounded away
        // from zero so the lag reaches target exactly instead of stalling
        // once the step truncates to nothing. With dt < tau the step never
        // exceeds the distance, so it cannot overshoot.
        // dt_ms < tau (600000), so both casts to int64_t are lossless.
        step = (target - p_logic->heat_milli_f10) * (int64_t)dt_ms;
        if (step >= 0)
        {
            step = (step + TAU_MS - 1) / TAU_MS;
        }
        else
        {
            step = (step - (TAU_MS - 1)) / TAU_MS;
        }
        // The sum lies between the old value and target, both int32_t.
        p_logic->heat_milli_f10 += (int32_t)step;
    }

done:
    return;
}

bool sensor_logic_valid(sensor_logic_t const * p_logic, uint64_t now_ms)
{
    bool b_valid = false;

    if ((NULL != p_logic) && p_logic->b_have_reading &&
        (now_ms >= p_logic->last_good_ms))
    {
        b_valid = ((now_ms - p_logic->last_good_ms) < SENSOR_FAULT_MS);
        if (b_valid && p_logic->b_recovering)
        {
            b_valid =
                ((now_ms >= p_logic->good_since_ms) &&
                 ((now_ms - p_logic->good_since_ms) >= SENSOR_RECOVER_MS));
        }
    }

    return b_valid;
}

int16_t sensor_logic_heat_f10(sensor_logic_t const * p_logic)
{
    int32_t heat = 0;

    if (NULL != p_logic)
    {
        heat = p_logic->heat_milli_f10;
        heat = (heat >= 0) ? ((heat + (MILLI / 2)) / MILLI)
                           : ((heat - (MILLI / 2)) / MILLI);
    }

    return sensor_saturate_i16(heat);
}

int16_t sensor_logic_room_f10(sensor_logic_t const * p_logic, int16_t cal_f10)
{
    int32_t room = 0;

    if (NULL != p_logic)
    {
        room = (int32_t)p_logic->raw_f10 - sensor_logic_heat_f10(p_logic) +
               cal_f10;
    }

    return sensor_saturate_i16(room);
}
