/**
 * @file  relays_guard.h
 * @brief Pure interlock that re-checks the hard safety rules on every
 *        request, independently of hvac_logic (defence in depth).
 *
 * It shares no code or state with hvac_logic: it keeps its own record of
 * what was driven and when Y1 last went off, so a bug in the control logic
 * cannot also defeat the check. Turning Y1, G or W off is always allowed,
 * and so is a request for everything off (below). What it refuses:
 *
 *   - Y1 starting before it has been off for RELAYS_GUARD_MIN_OFF_MS, which
 *     is armed at boot;
 *   - O changing unless Y1 is off and has been for RELAYS_GUARD_MIN_OFF_MS;
 *     Y1 is then held off too, so it never runs against the wrong valve.
 *     The one exception is a request for all four outputs off (a sensor
 *     fault, or NULL to relays_apply()): it is honoured in full, like a
 *     reset, and the minimum-off time restarts at that moment, so neither
 *     Y1 nor O can change again for RELAYS_GUARD_MIN_OFF_MS;
 *   - W together with Y1 and O, which is aux strips while cooling;
 *   - Y1 or W without G: G is forced on with them.
 *
 * Its own state is kept twice, the second copy bit-inverted. If the copies
 * ever disagree (a stray write, a corrupted RAM word), nothing about the
 * timers can be trusted: the guard drives everything off and restarts the
 * minimum-off time from that moment, then carries on.
 *
 * relays.c feeds it time from its own read of the clock, not the control
 * task's, so the two timer checks share no inputs.
 */
#ifndef RELAYS_GUARD_H
#define RELAYS_GUARD_H

#include <stdbool.h>
#include <stdint.h>

#define RELAYS_GUARD_MIN_OFF_MS 300000ULL // 5 min

typedef struct
{
    bool b_y1; // IN1, compressor
    bool b_g;  // IN2, blower
    bool b_o;  // IN3, reversing valve, energised in cool
    bool b_w;  // IN4, aux / emergency strips
} relays_outputs_t;

typedef struct
{
    relays_outputs_t applied;         // what is being driven now
    uint64_t         y1_off_since_ms; // last Y1 off edge; boot counts
    uint8_t          applied_inv;     // ~(applied as a bit mask)
    uint64_t         off_since_inv;   // ~y1_off_since_ms
    uint32_t         resyncs;         // times the copies disagreed
} relays_guard_t;

/**
 * Starts with everything off and the minimum-off timer armed at now_ms.
 */
void relays_guard_init(relays_guard_t * p_guard, uint64_t now_ms);

/**
 * Filters a request into outputs that are safe to drive, and records them as
 * applied.
 *
 * @param p_guard state from relays_guard_init()
 * @param p_req   requested outputs
 * @param now_ms  monotonic time, the same clock hvac_logic uses
 * @param p_out   receives the outputs to drive; all off if any argument is
 *                NULL
 * @return true if the request passed unchanged
 */
bool relays_guard_step(relays_guard_t * p_guard, relays_outputs_t const * p_req,
                       uint64_t now_ms, relays_outputs_t * p_out);

#endif /* RELAYS_GUARD_H */
