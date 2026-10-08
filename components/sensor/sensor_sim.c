/**
 * @file  sensor_sim.c
 * @brief A simulated room for bench testing. See sensor_sim.h.
 */
#include "sensor_sim.h"

#include <stddef.h>

#define MILLI     1000
#define MS_PER_MI 60000

// Signed copies of the time constants, for the step arithmetic.
#define PERIOD_MS ((int64_t)SENSOR_SIM_PERIOD_MS)
#define TAU_MS    ((int64_t)SENSOR_SIM_LOSS_TAU_MS)

static int16_t sensor_sim_round(int64_t milli_f10)
{
    int64_t f10 = (milli_f10 >= 0) ? ((milli_f10 + (MILLI / 2)) / MILLI)
                                   : ((milli_f10 - (MILLI / 2)) / MILLI);

    if (f10 > INT16_MAX)
    {
        f10 = INT16_MAX;
    }
    else if (f10 < INT16_MIN)
    {
        f10 = INT16_MIN;
    }
    else
    {
        // In range.
    }

    // Clamped to the int16_t range just above.
    return (int16_t)f10;
}

// A rate in tenths F per minute, applied for dt_ms, in thousandths of 0.1 F.
static int64_t sensor_sim_rate(int32_t f10_per_min, int64_t dt_ms)
{
    return ((int64_t)f10_per_min * MILLI * dt_ms) / MS_PER_MI;
}

void sensor_sim_init(sensor_sim_t * p_sim, uint64_t now_ms)
{
    if (NULL != p_sim)
    {
        p_sim->temp_milli_f10 = (int64_t)SENSOR_SIM_START_F10 * MILLI;
        p_sim->last_ms        = now_ms;
    }
}

int16_t sensor_sim_outdoor_f10(uint64_t now_ms)
{
    int64_t const span =
        SENSOR_SIM_OUTDOOR_MAX_F10 - SENSOR_SIM_OUTDOOR_MIN_F10;
    int64_t const half = PERIOD_MS / 2;
    // The remainder is below PERIOD_MS, so it fits int64_t.
    int64_t const phase = (int64_t)(now_ms % SENSOR_SIM_PERIOD_MS);
    int64_t       rise  = 0;

    if (phase < half)
    {
        rise = (span * phase) / half;
    }
    else
    {
        rise = span - ((span * (phase - half)) / half);
    }

    // rise is within 0..span, so the sum stays within the min..max limits.
    return (int16_t)(SENSOR_SIM_OUTDOOR_MIN_F10 + rise);
}

int16_t sensor_sim_step(sensor_sim_t * p_sim, bool b_y1, bool b_o, bool b_w,
                        uint64_t now_ms)
{
    int16_t temp_f10 = SENSOR_SIM_START_F10;
    int64_t dt_ms    = 0;
    int64_t outdoor  = 0;
    int64_t delta    = 0;

    if (NULL == p_sim)
    {
        goto done;
    }

    if (now_ms > p_sim->last_ms)
    {
        // Capped at SENSOR_SIM_MAX_STEP_MS below, so it fits int64_t.
        dt_ms = (int64_t)(now_ms - p_sim->last_ms);
    }
    if (dt_ms > (int64_t)SENSOR_SIM_MAX_STEP_MS)
    {
        dt_ms = (int64_t)SENSOR_SIM_MAX_STEP_MS;
    }
    p_sim->last_ms = now_ms;

    outdoor = (int64_t)sensor_sim_outdoor_f10(now_ms) * MILLI;
    delta   = ((outdoor - p_sim->temp_milli_f10) * dt_ms) / TAU_MS;

    if (b_y1 && !b_o)
    {
        delta += sensor_sim_rate(SENSOR_SIM_HEAT_F10_PER_MIN, dt_ms);
    }
    if (b_y1 && b_o)
    {
        delta -= sensor_sim_rate(SENSOR_SIM_COOL_F10_PER_MIN, dt_ms);
    }
    if (b_w)
    {
        delta += sensor_sim_rate(SENSOR_SIM_AUX_F10_PER_MIN, dt_ms);
    }

    p_sim->temp_milli_f10 += delta;
    temp_f10 = sensor_sim_round(p_sim->temp_milli_f10);

done:
    return temp_f10;
}
