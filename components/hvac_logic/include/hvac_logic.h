/**
 * @file  hvac_logic.h
 * @brief Pure thermostat logic for a heat pump with electric aux strips.
 *
 * No ESP-IDF, Matter or HomeKit types: it takes the mode, setpoints, room
 * temperature and a monotonic time in ms, and returns the four outputs. The
 * same code runs on the C6 and in the host tests. The rules are
 * docs/CONTROL_SPEC.md; this header records how each one is read.
 *
 * Outputs per call (O is the reversing valve, energised in cool):
 *
 *   call    Y1 G O W
 *   none     0 * h 0     G per the fan setting or purge; O held
 *   heat     1 1 0 0/1   W added by the aux staging rule
 *   cool     1 1 1 0
 *   e-heat   0 1 0 1     Y1 never on
 *   fault    0 0 0 0     no valid temperature: everything off
 *
 * Interpretations of the spec, so they can be checked:
 *   - Emergency heat is a mode (HVAC_MODE_EHEAT). It heats to the heat
 *     setpoint with G + W only. The Matter bridge maps its On/Off endpoint
 *     onto this mode.
 *   - Minimum run holds Y1 on after the room is satisfied, and through a
 *     change of direction (Heat <-> Cool, by mode or by Auto): the running
 *     call continues until the run is complete (a held heat run drops W),
 *     then the new call waits out the minimum-off time and the O rule. It
 *     is waived only for mode Off, for a fault, and for emergency heat,
 *     whose own hard rule is "Y1 always off". The minimum-off time is never
 *     waived.
 *   - A fault drops every output, O included, even inside the minimum-off
 *     time; that restarts the minimum-off time (the relay guard does the
 *     same for an all-off request), so Y1 and O stay put for 5 min after.
 *   - Y1 waits for O: a call whose valve position differs from O waits until
 *     Y1 has been off for the minimum-off time, then O and Y1 change together.
 *   - W in heat is only ever added on top of a running Y1.
 *   - "Still falling after 15 min of heat": every 15 min of compressor heat,
 *     the room is compared with 15 min earlier; lower means add W.
 *   - Auto changes over between heat and cool only after 10 min with no
 *     call. Explicit Heat or Cool modes skip that wait; the O rule still
 *     applies.
 *   - Fan purge runs G on after any heat, cool or e-heat call ends, but not
 *     after a fault.
 *
 * Feedback: the relay guard downstream may refuse a request (it re-checks the
 * timers on its own clock). When the caller passes what was actually driven
 * (hvac_input_t.applied), the logic adopts it as the truth before each step:
 *   - a compressor start that was refused never happened, so the off time it
 *     replaced is restored and the start is simply asked for again;
 *   - a compressor that stopped without being told to counts as stopping
 *     now, which re-arms the minimum-off time in full;
 *   - minimum run is therefore timed from a start that really happened.
 */
#ifndef HVAC_LOGIC_H
#define HVAC_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

// Timing.
#define HVAC_MIN_OFF_MS 300000ULL // 5 min, armed at boot
#define HVAC_MIN_RUN_MS 180000ULL // 3 min

// Y1 is held for one extra control cycle past the minimum run. The pins
// switch a few ms after each decision, by an amount that varies from cycle
// to cycle, so timing the run exactly could leave the compressor running a
// few ms short of 3 min. One cycle of margin makes the physical run at
// least HVAC_MIN_RUN_MS (test_safety_chain checks it on the pins' clock).
#define HVAC_SWITCH_MARGIN_MS   1000ULL
#define HVAC_MIN_RUN_HOLD_MS    (HVAC_MIN_RUN_MS + HVAC_SWITCH_MARGIN_MS)
#define HVAC_AUX_TREND_MS       900000ULL // 15 min
#define HVAC_CHANGEOVER_IDLE_MS 600000ULL // 10 min
#define HVAC_FAN_PURGE_MAX_S    600U

// Temperatures, in tenths of a degree F.
#define HVAC_HYST_F10          5  // +/- 0.5 F around the setpoint
#define HVAC_AUX_START_F10     30 // add W this far below the setpoint
#define HVAC_AUX_STOP_F10      10 // drop W within this of the setpoint
#define HVAC_AUTO_DEADBAND_F10 30 // heat setpoint + 3 F <= cool setpoint

// Setpoint ranges: the user's chosen 60..80 F overall (Oct 2026). Matter
// needs the heat and cool ranges at least its deadband (1.6 C) apart at both
// ends; heat stops at 76 and cool starts at 64, keeping the Auto pair 3 F
// apart at every setting, and the Home app then offers 60..80.
// bridge_map_limits() derives the Matter limits from these, and test_bridge_map
// checks the gap.
#define HVAC_HEAT_SP_MIN_F10 600 // 60.0 F
#define HVAC_HEAT_SP_MAX_F10 760 // 76.0 F
#define HVAC_COOL_SP_MIN_F10 640 // 64.0 F
#define HVAC_COOL_SP_MAX_F10 800 // 80.0 F

typedef enum
{
    HVAC_MODE_OFF = 0,
    HVAC_MODE_HEAT,
    HVAC_MODE_COOL,
    HVAC_MODE_AUTO,
    HVAC_MODE_EHEAT,
    HVAC_MODE_COUNT
} hvac_mode_t;

typedef enum
{
    HVAC_FAN_AUTO = 0, // G only with a call, plus purge
    HVAC_FAN_ON,       // G always (except on a fault)
    HVAC_FAN_COUNT
} hvac_fan_t;

typedef enum
{
    HVAC_CALL_NONE = 0,
    HVAC_CALL_HEAT,
    HVAC_CALL_COOL,
    HVAC_CALL_EHEAT
} hvac_call_t;

typedef struct
{
    bool b_y1; // compressor
    bool b_g;  // blower
    bool b_o;  // reversing valve, energised in cool
    bool b_w;  // aux / emergency strips
} hvac_outputs_t;

typedef struct
{
    hvac_mode_t    mode;
    hvac_fan_t     fan;
    int16_t        heat_sp_f10;     // clamped to the HEAT_SP limits
    int16_t        cool_sp_f10;     // clamped to the COOL_SP limits
    uint16_t       fan_purge_s;     // clamped to HVAC_FAN_PURGE_MAX_S; 0 = none
    int16_t        temp_f10;        // room temperature, used only if valid
    bool           b_temp_valid;    // false = sensor fault: everything off
    uint64_t       now_ms;          // monotonic
    bool           b_applied_valid; // applied below is filled in
    hvac_outputs_t applied; // what was really driven after the last step
} hvac_input_t;

typedef struct
{
    hvac_outputs_t out;       // what to drive
    hvac_call_t    call;      // the call being served, or waiting to be
    bool           b_aux;     // W is staged on top of heat
    bool           b_waiting; // a heat or cool call is held by a timer
    uint32_t       wait_s;    // seconds until the compressor may start
    bool           b_fault;   // no valid temperature: everything off
} hvac_status_t;

/*
 * Internal state, carried between steps. Callers create one, pass it to
 * hvac_init() and then to every hvac_step(); they never touch the fields.
 */
typedef struct
{
    hvac_outputs_t out;             // what the last step returned
    hvac_call_t    call;            // the call being served
    hvac_call_t    last_call;       // most recent call, for auto changeover
    bool           b_aux;           // W staged on in heat
    uint64_t       y1_edge_ms;      // last Y1 change; boot is an off edge
    uint64_t       y1_edge_prev_ms; // y1_edge_ms before the last change
    bool           b_start_pending; // the last step asked Y1 to start
    uint64_t       idle_since_ms;   // when the last call ended
    uint64_t       purge_until_ms;  // G runs on until here
    uint64_t       aux_ref_ms;      // start of the current 15 min trend window
    int16_t        aux_ref_f10;     // room temperature at aux_ref_ms
} hvac_state_t;

/**
 * Puts p_state in the boot state: every output off, no call, and the
 * compressor minimum-off timer started at now_ms.
 */
void hvac_init(hvac_state_t * p_state, uint64_t now_ms);

/**
 * Advances the logic by one step. Call it about once a second; the timers
 * use p_in->now_ms, so the period only sets the reaction time.
 *
 * @param p_state  state from hvac_init(); NULL yields an all-off status
 * @param p_in     this step's inputs; NULL yields an all-off status
 * @param p_status receives the outputs to drive and what they mean
 */
void hvac_step(hvac_state_t * p_state, hvac_input_t const * p_in,
               hvac_status_t * p_status);

/**
 * Clamps both setpoints to their limits and restores the auto deadband by
 * moving the one that did not lead. If that one runs into its own limit, the
 * leader is pulled back instead. Call it on every setpoint write.
 *
 * @param p_heat_f10   heat setpoint, updated in place
 * @param p_cool_f10   cool setpoint, updated in place
 * @param b_heat_leads true if the heat setpoint is the one the user moved
 */
void hvac_setpoints_apply(int16_t * p_heat_f10, int16_t * p_cool_f10,
                          bool b_heat_leads);

/**
 * True if both setpoints are within their limits and the deadband holds.
 */
bool hvac_setpoints_valid(int16_t heat_f10, int16_t cool_f10);

#endif /* HVAC_LOGIC_H */
