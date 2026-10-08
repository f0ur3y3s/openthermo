/**
 * @file  hvac_logic.c
 * @brief Pure thermostat logic. See hvac_logic.h for the rules and how each
 *        one is read.
 *
 * One step runs in this order:
 *   1. a fault (no valid temperature) turns everything off and stops there;
 *   2. the demand: which call the room asks for, with hysteresis and the
 *      auto changeover wait;
 *   3. the minimum run, which can keep a satisfied heat or cool call alive;
 *   4. the outputs for the call, where the compressor waits on the
 *      minimum-off time and on the reversing valve, and aux is staged;
 *   5. G, from the call, the fan setting and the purge.
 */
#include "hvac_logic.h"

#include <stddef.h>
#include <string.h>

#define MS_PER_S 1000ULL

// The setpoint limits must leave room for the deadband in both directions,
// or hvac_setpoints_apply() could not always restore it.
_Static_assert(HVAC_HEAT_SP_MAX_F10 + HVAC_AUTO_DEADBAND_F10 <=
                   HVAC_COOL_SP_MAX_F10,
               "heat max + deadband must fit under cool max");
_Static_assert(HVAC_COOL_SP_MIN_F10 - HVAC_AUTO_DEADBAND_F10 >=
                   HVAC_HEAT_SP_MIN_F10,
               "cool min - deadband must stay above heat min");

// Time since `since`, or 0 if the clock appears to have gone backwards.
static uint64_t hvac_elapsed(uint64_t now_ms, uint64_t since_ms)
{
    uint64_t elapsed = 0U;

    if (now_ms > since_ms)
    {
        elapsed = now_ms - since_ms;
    }

    return elapsed;
}

static int16_t hvac_clamp_i16(int32_t value, int16_t min, int16_t max)
{
    int32_t clamped = value;

    if (clamped < min)
    {
        clamped = min;
    }
    else if (clamped > max)
    {
        clamped = max;
    }
    else
    {
        // Already in range.
    }

    // Clamped between two int16_t limits, so it narrows losslessly.
    return (int16_t)clamped;
}

static bool hvac_is_heating(hvac_call_t call)
{
    return ((HVAC_CALL_HEAT == call) || (HVAC_CALL_EHEAT == call));
}

// True if the mode still allows a compressor call in this direction.
static bool hvac_mode_permits(hvac_mode_t mode, hvac_call_t call)
{
    bool b_permits = false;

    switch (call)
    {
        case HVAC_CALL_HEAT:
            b_permits = ((HVAC_MODE_HEAT == mode) || (HVAC_MODE_AUTO == mode));
            break;
        case HVAC_CALL_COOL:
            b_permits = ((HVAC_MODE_COOL == mode) || (HVAC_MODE_AUTO == mode));
            break;
        default:
            // Neither "no call" nor e-heat runs the compressor.
            break;
    }

    return b_permits;
}

// Heating starts at setpoint - hysteresis and ends at setpoint + hysteresis.
static bool hvac_heat_wanted(bool b_active, int16_t temp_f10, int16_t sp_f10)
{
    bool b_wanted = false;

    if (b_active)
    {
        b_wanted = (temp_f10 < (sp_f10 + HVAC_HYST_F10));
    }
    else
    {
        b_wanted = (temp_f10 <= (sp_f10 - HVAC_HYST_F10));
    }

    return b_wanted;
}

// Cooling starts at setpoint + hysteresis and ends at setpoint - hysteresis.
static bool hvac_cool_wanted(bool b_active, int16_t temp_f10, int16_t sp_f10)
{
    bool b_wanted = false;

    if (b_active)
    {
        b_wanted = (temp_f10 > (sp_f10 - HVAC_HYST_F10));
    }
    else
    {
        b_wanted = (temp_f10 >= (sp_f10 + HVAC_HYST_F10));
    }

    return b_wanted;
}

// Brings out-of-range inputs back to something safe to act on.
static void hvac_sanitise(hvac_input_t * p_in)
{
    // The enums are compared as unsigned so a negative value is caught too;
    // both are small non-negative constants, so the casts are lossless.
    if ((uint32_t)p_in->mode >= (uint32_t)HVAC_MODE_COUNT)
    {
        p_in->mode = HVAC_MODE_OFF;
    }
    if ((uint32_t)p_in->fan >= (uint32_t)HVAC_FAN_COUNT)
    {
        p_in->fan = HVAC_FAN_AUTO;
    }
    if (p_in->fan_purge_s > HVAC_FAN_PURGE_MAX_S)
    {
        p_in->fan_purge_s = HVAC_FAN_PURGE_MAX_S;
    }

    p_in->heat_sp_f10 = hvac_clamp_i16(p_in->heat_sp_f10, HVAC_HEAT_SP_MIN_F10,
                                       HVAC_HEAT_SP_MAX_F10);
    p_in->cool_sp_f10 = hvac_clamp_i16(p_in->cool_sp_f10, HVAC_COOL_SP_MIN_F10,
                                       HVAC_COOL_SP_MAX_F10);
}

// Auto: keep serving the current direction until it is satisfied; start a
// new call in the other direction only after the changeover idle time.
static hvac_call_t hvac_demand_auto(hvac_state_t const * p_state,
                                    hvac_input_t const * p_in)
{
    hvac_call_t want      = HVAC_CALL_NONE;
    hvac_call_t previous  = p_state->last_call;
    bool        b_settled = false;
    bool        b_heat    = false;
    bool        b_cool    = false;

    if (HVAC_CALL_HEAT == p_state->call)
    {
        b_heat = hvac_heat_wanted(true, p_in->temp_f10, p_in->heat_sp_f10);
        want   = b_heat ? HVAC_CALL_HEAT : HVAC_CALL_NONE;
        goto done;
    }
    if (HVAC_CALL_COOL == p_state->call)
    {
        b_cool = hvac_cool_wanted(true, p_in->temp_f10, p_in->cool_sp_f10);
        want   = b_cool ? HVAC_CALL_COOL : HVAC_CALL_NONE;
        goto done;
    }

    // No compressor call. A call still running (e-heat, after a mode
    // change) is the previous direction, and the system is not yet idle.
    if (HVAC_CALL_NONE != p_state->call)
    {
        previous = p_state->call;
    }
    else
    {
        b_settled = (hvac_elapsed(p_in->now_ms, p_state->idle_since_ms) >=
                     HVAC_CHANGEOVER_IDLE_MS);
    }

    b_heat = hvac_heat_wanted(false, p_in->temp_f10, p_in->heat_sp_f10);
    b_cool = hvac_cool_wanted(false, p_in->temp_f10, p_in->cool_sp_f10);

    // Both at once only happens with inverted setpoints: serve neither.
    if (b_heat && !b_cool && (b_settled || (HVAC_CALL_COOL != previous)))
    {
        want = HVAC_CALL_HEAT;
    }
    else if (b_cool && !b_heat && (b_settled || !hvac_is_heating(previous)))
    {
        want = HVAC_CALL_COOL;
    }
    else
    {
        // Satisfied, waiting out the changeover, or setpoints inverted.
    }

done:
    return want;
}

static hvac_call_t hvac_demand(hvac_state_t const * p_state,
                               hvac_input_t const * p_in)
{
    hvac_call_t want = HVAC_CALL_NONE;
    hvac_call_t call = p_state->call;
    int16_t     temp = p_in->temp_f10;
    int16_t     heat = p_in->heat_sp_f10;
    int16_t     cool = p_in->cool_sp_f10;

    switch (p_in->mode)
    {
        case HVAC_MODE_HEAT:
            if (hvac_heat_wanted(HVAC_CALL_HEAT == call, temp, heat))
            {
                want = HVAC_CALL_HEAT;
            }
            break;
        case HVAC_MODE_COOL:
            if (hvac_cool_wanted(HVAC_CALL_COOL == call, temp, cool))
            {
                want = HVAC_CALL_COOL;
            }
            break;
        case HVAC_MODE_EHEAT:
            if (hvac_heat_wanted(HVAC_CALL_EHEAT == call, temp, heat))
            {
                want = HVAC_CALL_EHEAT;
            }
            break;
        case HVAC_MODE_AUTO:
            want = hvac_demand_auto(p_state, p_in);
            break;
        case HVAC_MODE_OFF:
        default:
            // No call.
            break;
    }

    return want;
}

// True while a running heat or cool call must continue for minimum run:
// whether the room is satisfied or the mode now asks for the other direction.
// Only Off, emergency heat and a fault (handled before demand) end it early.
static bool hvac_min_run_holds(hvac_state_t const * p_state,
                               hvac_input_t const * p_in)
{
    return (p_state->out.b_y1 &&
            ((HVAC_CALL_HEAT == p_state->call) ||
             (HVAC_CALL_COOL == p_state->call)) &&
            (HVAC_MODE_OFF != p_in->mode) && (HVAC_MODE_EHEAT != p_in->mode) &&
            (hvac_elapsed(p_in->now_ms, p_state->y1_edge_ms) <
             HVAC_MIN_RUN_HOLD_MS));
}

static void hvac_change_call(hvac_state_t * p_state, hvac_call_t want,
                             hvac_input_t const * p_in)
{
    if (HVAC_CALL_NONE != p_state->call)
    {
        p_state->last_call     = p_state->call;
        p_state->idle_since_ms = p_in->now_ms;
        p_state->purge_until_ms =
            p_in->now_ms + ((uint64_t)p_in->fan_purge_s * MS_PER_S);
    }

    p_state->call  = want;
    p_state->b_aux = false;
}

// Compressor for a heat (b_want_o false) or cool (true) call. O changes only
// once Y1 has been off for the minimum-off time, and Y1 never runs against
// the wrong valve position.
static void hvac_drive_compressor(hvac_state_t const * p_state,
                                  hvac_outputs_t * p_out, bool b_want_o,
                                  uint64_t now_ms)
{
    bool b_off_ok =
        (!p_state->out.b_y1 &&
         (hvac_elapsed(now_ms, p_state->y1_edge_ms) >= HVAC_MIN_OFF_MS));

    if ((p_out->b_o != b_want_o) && b_off_ok)
    {
        p_out->b_o = b_want_o;
    }

    if (p_out->b_o != b_want_o)
    {
        p_out->b_y1 = false; // wait for the valve
    }
    else if (p_state->out.b_y1)
    {
        p_out->b_y1 = true; // keep running
    }
    else
    {
        p_out->b_y1 = b_off_ok; // start only after the minimum-off time
    }
}

// Aux staging while the compressor heats: add W when far below setpoint or
// still falling after a trend window; drop it near setpoint.
static void hvac_stage_aux(hvac_state_t * p_state, int16_t temp_f10,
                           int16_t sp_f10, uint64_t now_ms)
{
    if (p_state->b_aux)
    {
        if (temp_f10 >= (sp_f10 - HVAC_AUX_STOP_F10))
        {
            p_state->b_aux       = false;
            p_state->aux_ref_f10 = temp_f10;
            p_state->aux_ref_ms  = now_ms;
        }
    }
    else if (temp_f10 <= (sp_f10 - HVAC_AUX_START_F10))
    {
        p_state->b_aux = true;
    }
    else if (hvac_elapsed(now_ms, p_state->aux_ref_ms) >= HVAC_AUX_TREND_MS)
    {
        if ((temp_f10 < p_state->aux_ref_f10) &&
            (temp_f10 < (sp_f10 - HVAC_AUX_STOP_F10)))
        {
            p_state->b_aux = true;
        }
        p_state->aux_ref_f10 = temp_f10;
        p_state->aux_ref_ms  = now_ms;
    }
    else
    {
        // Mid-window, not far enough below: heat pump alone.
    }
}

static void hvac_drive_heat(hvac_state_t * p_state, hvac_outputs_t * p_out,
                            hvac_input_t const * p_in)
{
    hvac_drive_compressor(p_state, p_out, false, p_in->now_ms);

    if (!p_out->b_y1)
    {
        p_state->b_aux = false;
    }
    else
    {
        if (!p_state->out.b_y1)
        {
            // Compressor just started: the trend window starts with it.
            p_state->aux_ref_f10 = p_in->temp_f10;
            p_state->aux_ref_ms  = p_in->now_ms;
        }
        hvac_stage_aux(p_state, p_in->temp_f10, p_in->heat_sp_f10,
                       p_in->now_ms);
    }

    p_out->b_w = p_state->b_aux;
}

static void hvac_drive(hvac_state_t * p_state, hvac_input_t const * p_in)
{
    hvac_outputs_t out      = p_state->out;
    bool           b_off_ok = false;

    switch (p_state->call)
    {
        case HVAC_CALL_HEAT:
            hvac_drive_heat(p_state, &out, p_in);
            if (!hvac_mode_permits(p_in->mode, p_state->call))
            {
                // A heat run held over into Cool for its minimum run: the
                // heat pump only, never the strips.
                p_state->b_aux = false;
                out.b_w        = false;
            }
            break;
        case HVAC_CALL_COOL:
            hvac_drive_compressor(p_state, &out, true, p_in->now_ms);
            out.b_w = false;
            break;
        case HVAC_CALL_EHEAT:
            out.b_y1 = false;
            out.b_w  = true;
            b_off_ok = (!p_state->out.b_y1 &&
                        (hvac_elapsed(p_in->now_ms, p_state->y1_edge_ms) >=
                         HVAC_MIN_OFF_MS));
            if (b_off_ok)
            {
                out.b_o = false;
            }
            break;
        case HVAC_CALL_NONE:
        default:
            out.b_y1 = false; // O is held while idle
            out.b_w  = false;
            break;
    }

    out.b_g = (out.b_y1 || out.b_w || (HVAC_FAN_ON == p_in->fan) ||
               (p_in->now_ms < p_state->purge_until_ms));

    p_state->out = out;
}

// Adopts what was really driven as the truth. See hvac_logic.h, Feedback.
static void hvac_reconcile(hvac_state_t *         p_state,
                           hvac_outputs_t const * p_applied, uint64_t now_ms)
{
    if (p_state->out.b_y1 && !p_applied->b_y1)
    {
        if (p_state->b_start_pending)
        {
            // A start refused downstream: Y1 never ran, so the off time it
            // replaced still stands.
            p_state->y1_edge_ms = p_state->y1_edge_prev_ms;
        }
        else
        {
            // Stopped without being told: the off time starts now.
            p_state->y1_edge_prev_ms = p_state->y1_edge_ms;
            p_state->y1_edge_ms      = now_ms;
        }
    }
    else if (!p_state->out.b_y1 && p_applied->b_y1)
    {
        // Running without being asked (never expected): treat it as having
        // just started, so minimum run and minimum off both count from now.
        p_state->y1_edge_prev_ms = p_state->y1_edge_ms;
        p_state->y1_edge_ms      = now_ms;
    }
    else
    {
        // Y1 is as requested.
    }

    p_state->out             = *p_applied;
    p_state->b_start_pending = false;
}

// No valid temperature: every output off at once, no purge. O drops too,
// even inside the minimum-off time; that restarts the minimum-off time (as
// the relay guard does for an all-off request), so neither Y1 nor O changes
// again for HVAC_MIN_OFF_MS. (A running Y1 stopping here re-arms it anyway,
// in hvac_step.)
static void hvac_fault(hvac_state_t * p_state, uint64_t now_ms)
{
    // Every O drop re-arms, not only one inside the minimum-off time: the
    // guard decides "inside" on its own clock, read a little later, so this
    // keeps the logic never less strict than the guard after a fault.
    if (p_state->out.b_o && !p_state->out.b_y1)
    {
        p_state->y1_edge_prev_ms = p_state->y1_edge_ms;
        p_state->y1_edge_ms      = now_ms;
    }

    if (HVAC_CALL_NONE != p_state->call)
    {
        p_state->last_call     = p_state->call;
        p_state->idle_since_ms = now_ms;
    }

    memset(&p_state->out, 0, sizeof(p_state->out));
    p_state->call           = HVAC_CALL_NONE;
    p_state->b_aux          = false;
    p_state->purge_until_ms = now_ms;
}

static void hvac_report(hvac_state_t const * p_state, hvac_input_t const * p_in,
                        hvac_status_t * p_status)
{
    uint64_t off_ms = 0U;

    p_status->out       = p_state->out;
    p_status->call      = p_state->call;
    p_status->b_aux     = p_state->b_aux;
    p_status->b_fault   = !p_in->b_temp_valid;
    p_status->b_waiting = (((HVAC_CALL_HEAT == p_state->call) ||
                            (HVAC_CALL_COOL == p_state->call)) &&
                           !p_state->out.b_y1);
    p_status->wait_s    = 0U;

    if (!p_state->out.b_y1)
    {
        off_ms = hvac_elapsed(p_in->now_ms, p_state->y1_edge_ms);
        if (off_ms < HVAC_MIN_OFF_MS)
        {
            // At most HVAC_MIN_OFF_MS / 1000 = 300, rounded up; fits uint32_t.
            p_status->wait_s =
                (uint32_t)((HVAC_MIN_OFF_MS - off_ms + MS_PER_S - 1U) /
                           MS_PER_S);
        }
    }
}

void hvac_init(hvac_state_t * p_state, uint64_t now_ms)
{
    if (NULL == p_state)
    {
        goto done;
    }

    memset(p_state, 0, sizeof(*p_state));
    p_state->call            = HVAC_CALL_NONE;
    p_state->last_call       = HVAC_CALL_NONE;
    p_state->y1_edge_ms      = now_ms; // boot arms the minimum-off timer
    p_state->y1_edge_prev_ms = now_ms;
    p_state->idle_since_ms   = now_ms;
    p_state->purge_until_ms  = now_ms;
    p_state->aux_ref_ms      = now_ms;

done:
    return;
}

void hvac_step(hvac_state_t * p_state, hvac_input_t const * p_in,
               hvac_status_t * p_status)
{
    hvac_input_t in       = { 0 };
    hvac_call_t  want     = HVAC_CALL_NONE;
    bool         b_y1_was = false;

    if (NULL == p_status)
    {
        goto done;
    }
    if ((NULL == p_state) || (NULL == p_in))
    {
        memset(p_status, 0, sizeof(*p_status));
        p_status->b_fault = true;
        goto done;
    }

    in = *p_in;
    hvac_sanitise(&in);
    if (in.b_applied_valid)
    {
        hvac_reconcile(p_state, &in.applied, in.now_ms);
    }
    b_y1_was = p_state->out.b_y1;

    if (!in.b_temp_valid)
    {
        hvac_fault(p_state, in.now_ms);
    }
    else
    {
        want = hvac_demand(p_state, &in);
        if ((want != p_state->call) && hvac_min_run_holds(p_state, &in))
        {
            want = p_state->call;
        }
        if (want != p_state->call)
        {
            hvac_change_call(p_state, want, &in);
        }
        hvac_drive(p_state, &in);
    }

    p_state->b_start_pending = false;
    if (b_y1_was != p_state->out.b_y1)
    {
        p_state->y1_edge_prev_ms = p_state->y1_edge_ms;
        p_state->y1_edge_ms      = in.now_ms;
        p_state->b_start_pending = p_state->out.b_y1;
    }

    hvac_report(p_state, &in, p_status);

done:
    return;
}

void hvac_setpoints_apply(int16_t * p_heat_f10, int16_t * p_cool_f10,
                          bool b_heat_leads)
{
    int16_t heat = 0;
    int16_t cool = 0;

    if ((NULL == p_heat_f10) || (NULL == p_cool_f10))
    {
        goto done;
    }

    heat =
        hvac_clamp_i16(*p_heat_f10, HVAC_HEAT_SP_MIN_F10, HVAC_HEAT_SP_MAX_F10);
    cool =
        hvac_clamp_i16(*p_cool_f10, HVAC_COOL_SP_MIN_F10, HVAC_COOL_SP_MAX_F10);

    if ((heat + HVAC_AUTO_DEADBAND_F10) > cool)
    {
        if (b_heat_leads)
        {
            cool = hvac_clamp_i16(heat + HVAC_AUTO_DEADBAND_F10,
                                  HVAC_COOL_SP_MIN_F10, HVAC_COOL_SP_MAX_F10);
            heat = hvac_clamp_i16(cool - HVAC_AUTO_DEADBAND_F10,
                                  HVAC_HEAT_SP_MIN_F10, HVAC_HEAT_SP_MAX_F10);
        }
        else
        {
            heat = hvac_clamp_i16(cool - HVAC_AUTO_DEADBAND_F10,
                                  HVAC_HEAT_SP_MIN_F10, HVAC_HEAT_SP_MAX_F10);
            cool = hvac_clamp_i16(heat + HVAC_AUTO_DEADBAND_F10,
                                  HVAC_COOL_SP_MIN_F10, HVAC_COOL_SP_MAX_F10);
        }
    }

    *p_heat_f10 = heat;
    *p_cool_f10 = cool;

done:
    return;
}

bool hvac_setpoints_valid(int16_t heat_f10, int16_t cool_f10)
{
    return ((heat_f10 >= HVAC_HEAT_SP_MIN_F10) &&
            (heat_f10 <= HVAC_HEAT_SP_MAX_F10) &&
            (cool_f10 >= HVAC_COOL_SP_MIN_F10) &&
            (cool_f10 <= HVAC_COOL_SP_MAX_F10) &&
            ((heat_f10 + HVAC_AUTO_DEADBAND_F10) <= cool_f10));
}
