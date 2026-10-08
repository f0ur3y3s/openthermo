/**
 * @file  test_main.c
 * @brief Host tests for the thermostat logic: every rule in
 *        docs/CONTROL_SPEC.md and the hard safety rules in CLAUDE.md.
 *
 * Runs with `pio test -e native`. The module under test is compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 *
 * Time advances in 1 s steps, the control task's period, so the timers are
 * exercised the way the firmware drives them.
 */
#include "../../components/hvac_logic/hvac_logic.c"

#include <unity.h>

#define START_MS 1000ULL
#define STEP_MS  1000ULL
#define SEC      1000ULL
#define MIN      60000ULL

#define HEAT_SP 680 // 68.0 F
#define COOL_SP 760 // 76.0 F

static hvac_state_t  g_state  = { 0 };
static hvac_input_t  g_in     = { 0 };
static hvac_status_t g_status = { 0 };

static void step(void)
{
    hvac_step(&g_state, &g_in, &g_status);
}

// Steps once per second until `duration_ms` has passed.
static void run_for(uint64_t duration_ms)
{
    uint64_t end_ms = g_in.now_ms + duration_ms;

    while (g_in.now_ms < end_ms)
    {
        g_in.now_ms += STEP_MS;
        step();
    }
}

static void expect_out(bool b_y1, bool b_g, bool b_o, bool b_w)
{
    TEST_ASSERT_EQUAL_MESSAGE(b_y1, g_status.out.b_y1, "Y1");
    TEST_ASSERT_EQUAL_MESSAGE(b_g, g_status.out.b_g, "G");
    TEST_ASSERT_EQUAL_MESSAGE(b_o, g_status.out.b_o, "O");
    TEST_ASSERT_EQUAL_MESSAGE(b_w, g_status.out.b_w, "W");
}

// Runs through the boot minimum-off time with the room satisfied in every
// mode, so the compressor is free to start on the next call.
static void settle_after_boot(void)
{
    g_in.mode = HVAC_MODE_OFF;
    run_for(HVAC_MIN_OFF_MS);
}

// Starts a heat call that is running by the time this returns.
static void start_heating(void)
{
    settle_after_boot();
    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    expect_out(true, true, false, false);
}

// Starts a cool call that is running by the time this returns.
static void start_cooling(void)
{
    settle_after_boot();
    g_in.mode     = HVAC_MODE_COOL;
    g_in.temp_f10 = COOL_SP + 10;
    step();
    expect_out(true, true, true, false);
}

void setUp(void)
{
    hvac_init(&g_state, START_MS);
    memset(&g_status, 0, sizeof(g_status));
    g_in.mode            = HVAC_MODE_OFF;
    g_in.fan             = HVAC_FAN_AUTO;
    g_in.heat_sp_f10     = HEAT_SP;
    g_in.cool_sp_f10     = COOL_SP;
    g_in.fan_purge_s     = 60U;
    g_in.temp_f10        = 720;
    g_in.b_temp_valid    = true;
    g_in.now_ms          = START_MS;
    g_in.b_applied_valid = false;
}

void tearDown(void)
{
}

/* ---- Boot and the compressor timers ------------------------------------ */

static void test_boot_is_all_off_with_min_off_armed(void)
{
    step();
    expect_out(false, false, false, false);
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);
    TEST_ASSERT_EQUAL_UINT32(300U, g_status.wait_s);
}

static void test_heat_at_boot_waits_five_minutes(void)
{
    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 10;

    run_for(HVAC_MIN_OFF_MS - STEP_MS);
    expect_out(false, false, false, false);
    TEST_ASSERT_EQUAL(HVAC_CALL_HEAT, g_status.call);
    TEST_ASSERT_TRUE(g_status.b_waiting);
    TEST_ASSERT_EQUAL_UINT32(1U, g_status.wait_s);

    run_for(STEP_MS);
    expect_out(true, true, false, false);
    TEST_ASSERT_FALSE(g_status.b_waiting);
}

static void test_min_run_holds_a_satisfied_call(void)
{
    start_heating();

    g_in.temp_f10 = HEAT_SP + 20; // satisfied at once
    run_for(HVAC_MIN_RUN_HOLD_MS - STEP_MS);
    expect_out(true, true, false, false);

    run_for(STEP_MS);
    TEST_ASSERT_FALSE(g_status.out.b_y1);
}

static void test_min_off_after_a_run(void)
{
    start_heating();
    g_in.temp_f10 = HEAT_SP + 20;
    run_for(HVAC_MIN_RUN_HOLD_MS);
    TEST_ASSERT_FALSE(g_status.out.b_y1);

    g_in.temp_f10 = HEAT_SP - 10;
    run_for(HVAC_MIN_OFF_MS - STEP_MS);
    TEST_ASSERT_FALSE(g_status.out.b_y1);
    TEST_ASSERT_TRUE(g_status.b_waiting);

    run_for(STEP_MS);
    TEST_ASSERT_TRUE(g_status.out.b_y1);
}

static void test_mode_off_waives_min_run(void)
{
    start_heating();
    run_for(10U * SEC);

    g_in.mode = HVAC_MODE_OFF;
    step();
    TEST_ASSERT_FALSE(g_status.out.b_y1);
    TEST_ASSERT_FALSE(g_status.out.b_w);
}

static void test_clock_going_backwards_does_not_start_y1(void)
{
    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    g_in.now_ms   = START_MS - 500U;
    step();
    TEST_ASSERT_FALSE(g_status.out.b_y1);
}

/* ---- Hysteresis ---------------------------------------------------------- */

static void test_heat_hysteresis(void)
{
    settle_after_boot();
    g_in.mode = HVAC_MODE_HEAT;

    g_in.temp_f10 = HEAT_SP - 4; // 67.6: inside the band, no call
    step();
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);

    g_in.temp_f10 = HEAT_SP - 5; // 67.5: call
    step();
    TEST_ASSERT_TRUE(g_status.out.b_y1);

    run_for(HVAC_MIN_RUN_HOLD_MS);
    g_in.temp_f10 = HEAT_SP + 4; // 68.4: still heating
    step();
    TEST_ASSERT_TRUE(g_status.out.b_y1);

    g_in.temp_f10 = HEAT_SP + 5; // 68.5: satisfied
    step();
    TEST_ASSERT_FALSE(g_status.out.b_y1);
}

static void test_cool_hysteresis(void)
{
    settle_after_boot();
    g_in.mode = HVAC_MODE_COOL;

    g_in.temp_f10 = COOL_SP + 4;
    step();
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);

    g_in.temp_f10 = COOL_SP + 5;
    step();
    expect_out(true, true, true, false);

    run_for(HVAC_MIN_RUN_HOLD_MS);
    g_in.temp_f10 = COOL_SP - 4;
    step();
    TEST_ASSERT_TRUE(g_status.out.b_y1);

    g_in.temp_f10 = COOL_SP - 5;
    step();
    TEST_ASSERT_FALSE(g_status.out.b_y1);
}

/* ---- Reversing valve ----------------------------------------------------- */

static void test_cool_sets_o_with_y1(void)
{
    start_cooling(); // O and Y1 together once the boot timer has run
}

static void test_o_is_held_while_idle(void)
{
    start_cooling();
    g_in.temp_f10 = COOL_SP - 10;
    run_for(HVAC_MIN_RUN_HOLD_MS);
    TEST_ASSERT_FALSE(g_status.out.b_y1);
    TEST_ASSERT_TRUE(g_status.out.b_o);

    g_in.mode = HVAC_MODE_OFF;
    run_for(30U * MIN);
    TEST_ASSERT_TRUE(g_status.out.b_o);
}

static void test_o_changes_only_after_min_off(void)
{
    start_cooling();
    g_in.temp_f10 = COOL_SP - 10;
    run_for(HVAC_MIN_RUN_HOLD_MS); // Y1 just went off

    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    run_for(HVAC_MIN_OFF_MS - STEP_MS);
    expect_out(false, false, true, false);

    run_for(STEP_MS);
    expect_out(true, true, false, false);
}

static void test_heat_to_cool_holds_the_run_then_waits(void)
{
    start_heating();
    run_for(10U * SEC);

    g_in.mode     = HVAC_MODE_COOL;
    g_in.temp_f10 = COOL_SP + 10;
    step();
    // The heat run continues to its minimum run, on the heat valve position.
    expect_out(true, true, false, false);

    run_for(HVAC_MIN_RUN_HOLD_MS - (11U * SEC)); // 180 s into the run
    expect_out(true, true, false, false);

    run_for(STEP_MS); // run complete: Y1 stops, the valve waits
    expect_out(false, true, false, false);
    TEST_ASSERT_TRUE(g_status.b_waiting);
    TEST_ASSERT_EQUAL(HVAC_CALL_COOL, g_status.call);

    run_for(HVAC_MIN_OFF_MS - STEP_MS);
    TEST_ASSERT_FALSE(g_status.out.b_o);
    TEST_ASSERT_FALSE(g_status.out.b_y1);

    run_for(STEP_MS);
    expect_out(true, true, true, false);
}

static void test_cool_to_heat_holds_the_run_then_waits(void)
{
    start_cooling();
    run_for(10U * SEC);

    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    expect_out(true, true, true, false);

    run_for(HVAC_MIN_RUN_HOLD_MS - (11U * SEC));
    expect_out(true, true, true, false);

    run_for(STEP_MS); // Y1 stops; O is held until the minimum-off time
    expect_out(false, true, true, false);

    run_for(HVAC_MIN_OFF_MS - STEP_MS); // the purge has ended by now
    TEST_ASSERT_FALSE(g_status.out.b_y1);
    TEST_ASSERT_TRUE(g_status.out.b_o);

    run_for(STEP_MS);
    expect_out(true, true, false, false);
}

static void test_heat_run_held_into_cool_drops_aux(void)
{
    start_heating();
    g_in.temp_f10 = HEAT_SP - 30;
    step();
    expect_out(true, true, false, true);

    g_in.mode = HVAC_MODE_COOL;
    step();
    // Still the heat run, but never the strips once the mode is Cool.
    expect_out(true, true, false, false);
}

/* ---- Aux staging --------------------------------------------------------- */

static void test_aux_added_three_degrees_below(void)
{
    start_heating();

    g_in.temp_f10 = HEAT_SP - 29;
    step();
    TEST_ASSERT_FALSE(g_status.out.b_w);

    g_in.temp_f10 = HEAT_SP - 30;
    step();
    expect_out(true, true, false, true);
    TEST_ASSERT_TRUE(g_status.b_aux);
}

static void test_aux_dropped_within_one_degree(void)
{
    start_heating();
    g_in.temp_f10 = HEAT_SP - 30;
    step();
    TEST_ASSERT_TRUE(g_status.out.b_w);

    g_in.temp_f10 = HEAT_SP - 11;
    step();
    TEST_ASSERT_TRUE(g_status.out.b_w);

    g_in.temp_f10 = HEAT_SP - 10;
    step();
    expect_out(true, true, false, false);
}

static void test_aux_waits_for_the_compressor(void)
{
    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 80; // very cold, but Y1 is on its boot timer

    run_for(HVAC_MIN_OFF_MS - STEP_MS);
    expect_out(false, false, false, false);

    run_for(STEP_MS);
    expect_out(true, true, false, true);
}

static void test_aux_added_when_still_falling(void)
{
    start_heating(); // at 67.0
    g_in.temp_f10 = HEAT_SP - 15;
    run_for(HVAC_AUX_TREND_MS - STEP_MS);
    TEST_ASSERT_FALSE(g_status.out.b_w);

    run_for(STEP_MS);
    TEST_ASSERT_TRUE(g_status.out.b_w);
}

static void test_aux_not_added_when_holding(void)
{
    start_heating(); // at 67.0
    run_for(2U * HVAC_AUX_TREND_MS);
    TEST_ASSERT_FALSE(g_status.out.b_w);
}

static void test_aux_not_added_by_trend_within_one_degree(void)
{
    settle_after_boot();
    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 8;
    step();
    TEST_ASSERT_TRUE(g_status.out.b_y1);

    g_in.temp_f10 = HEAT_SP - 9;
    run_for(2U * HVAC_AUX_TREND_MS);
    TEST_ASSERT_FALSE(g_status.out.b_w);
}

static void test_aux_never_in_cool(void)
{
    start_cooling();
    g_in.temp_f10 = COOL_SP + 100;
    run_for(2U * HVAC_AUX_TREND_MS);
    expect_out(true, true, true, false);
}

/* ---- Emergency heat ------------------------------------------------------ */

static void test_eheat_is_g_and_w_without_waiting(void)
{
    g_in.mode     = HVAC_MODE_EHEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    expect_out(false, true, false, true);

    run_for(HVAC_MIN_OFF_MS + HVAC_AUX_TREND_MS);
    expect_out(false, true, false, true);
}

static void test_eheat_ends_with_hysteresis(void)
{
    g_in.mode     = HVAC_MODE_EHEAT;
    g_in.temp_f10 = HEAT_SP - 5;
    step();
    TEST_ASSERT_TRUE(g_status.out.b_w);

    g_in.temp_f10 = HEAT_SP + 4;
    step();
    TEST_ASSERT_TRUE(g_status.out.b_w);

    g_in.temp_f10 = HEAT_SP + 5;
    step();
    TEST_ASSERT_FALSE(g_status.out.b_w);
    TEST_ASSERT_FALSE(g_status.out.b_y1);
}

static void test_eheat_from_heat_stops_y1_at_once(void)
{
    start_heating();
    g_in.mode = HVAC_MODE_EHEAT;
    step();
    expect_out(false, true, false, true);
}

static void test_eheat_after_cool_releases_o_after_min_off(void)
{
    start_cooling();
    g_in.temp_f10 = COOL_SP - 10;
    run_for(HVAC_MIN_RUN_HOLD_MS); // Y1 just went off, O still on

    g_in.mode     = HVAC_MODE_EHEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    expect_out(false, true, true, true);

    run_for(HVAC_MIN_OFF_MS);
    expect_out(false, true, false, true);
}

/* ---- Faults -------------------------------------------------------------- */

static void test_fault_turns_everything_off(void)
{
    start_cooling();
    g_in.fan          = HVAC_FAN_ON;
    g_in.b_temp_valid = false;
    step();
    expect_out(false, false, false, false);
    TEST_ASSERT_TRUE(g_status.b_fault);
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);
}

static void test_fault_drops_o_inside_min_off_and_rearms(void)
{
    uint64_t fault_ms = 0U;

    start_cooling();
    g_in.temp_f10 = COOL_SP - 10;
    run_for(HVAC_MIN_RUN_HOLD_MS); // Y1 just went off, O held on
    expect_out(false, true, true, false);

    run_for(60U * SEC);
    g_in.b_temp_valid = false;
    g_in.now_ms += STEP_MS;
    step();
    fault_ms = g_in.now_ms;
    expect_out(false, false, false, false); // O drops too

    // The minimum-off time now counts from the fault, not the Y1 stop.
    g_in.b_temp_valid = true;
    g_in.temp_f10     = COOL_SP + 10;
    run_for((fault_ms + HVAC_MIN_OFF_MS - STEP_MS) - g_in.now_ms);
    TEST_ASSERT_FALSE(g_status.out.b_y1);
    TEST_ASSERT_FALSE(g_status.out.b_o);

    run_for(STEP_MS);
    expect_out(true, true, true, false);
}

static void test_fault_rearms_min_off(void)
{
    start_heating();
    g_in.b_temp_valid = false;
    step();

    g_in.b_temp_valid = true;
    run_for(HVAC_MIN_OFF_MS - STEP_MS);
    TEST_ASSERT_FALSE(g_status.out.b_y1);
    TEST_ASSERT_FALSE(g_status.b_fault);

    run_for(STEP_MS);
    TEST_ASSERT_TRUE(g_status.out.b_y1);
}

static void test_fault_during_eheat(void)
{
    g_in.mode     = HVAC_MODE_EHEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    g_in.b_temp_valid = false;
    step();
    expect_out(false, false, false, false);
}

static void test_null_arguments_are_all_off(void)
{
    hvac_step(NULL, &g_in, &g_status);
    expect_out(false, false, false, false);
    TEST_ASSERT_TRUE(g_status.b_fault);

    g_status.out.b_y1 = true;
    hvac_step(&g_state, NULL, &g_status);
    expect_out(false, false, false, false);

    hvac_step(&g_state, &g_in, NULL); // must not crash
    hvac_init(NULL, START_MS);
}

static void test_bad_mode_is_off(void)
{
    settle_after_boot();
    // Deliberately out of range, as a corrupted value would be.
    g_in.mode     = (hvac_mode_t)42;
    g_in.temp_f10 = HEAT_SP - 100;
    step();
    expect_out(false, false, false, false);
}

/* ---- Fan ----------------------------------------------------------------- */

static void test_fan_on_runs_g_when_idle(void)
{
    g_in.fan = HVAC_FAN_ON;
    step();
    expect_out(false, true, false, false);
}

static void test_purge_after_heat(void)
{
    start_heating();
    g_in.temp_f10 = HEAT_SP + 10;
    run_for(HVAC_MIN_RUN_HOLD_MS);
    expect_out(false, true, false, false);

    run_for(59U * SEC);
    TEST_ASSERT_TRUE(g_status.out.b_g);
    run_for(STEP_MS);
    TEST_ASSERT_FALSE(g_status.out.b_g);
}

static void test_no_purge_when_zero(void)
{
    g_in.fan_purge_s = 0U;
    start_heating();
    g_in.temp_f10 = HEAT_SP + 10;
    run_for(HVAC_MIN_RUN_HOLD_MS);
    expect_out(false, false, false, false);
}

static void test_purge_is_capped(void)
{
    g_in.fan_purge_s = 60000U;
    start_heating();
    g_in.temp_f10 = HEAT_SP + 10;
    run_for(HVAC_MIN_RUN_HOLD_MS + ((uint64_t)HVAC_FAN_PURGE_MAX_S * SEC));
    TEST_ASSERT_FALSE(g_status.out.b_g);
}

static void test_no_blower_while_waiting(void)
{
    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    TEST_ASSERT_TRUE(g_status.b_waiting);
    TEST_ASSERT_FALSE(g_status.out.b_g);
}

/* ---- Auto ---------------------------------------------------------------- */

static void test_auto_heats_and_cools(void)
{
    settle_after_boot();
    g_in.mode     = HVAC_MODE_AUTO;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    TEST_ASSERT_EQUAL(HVAC_CALL_HEAT, g_status.call);

    g_in.temp_f10 = 720;
    run_for(HVAC_MIN_RUN_HOLD_MS + HVAC_CHANGEOVER_IDLE_MS);
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);

    g_in.temp_f10 = COOL_SP + 10;
    step();
    expect_out(true, true, true, false);
}

static void test_auto_changeover_waits_ten_minutes_idle(void)
{
    settle_after_boot();
    g_in.mode     = HVAC_MODE_AUTO;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    g_in.temp_f10 = 720;
    run_for(HVAC_MIN_RUN_HOLD_MS); // heat call ends here
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);

    g_in.temp_f10 = COOL_SP + 10;
    run_for(HVAC_CHANGEOVER_IDLE_MS - STEP_MS);
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);
    TEST_ASSERT_FALSE(g_status.out.b_y1);

    run_for(STEP_MS);
    expect_out(true, true, true, false);
}

static void test_auto_same_direction_needs_no_changeover(void)
{
    settle_after_boot();
    g_in.mode     = HVAC_MODE_AUTO;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    g_in.temp_f10 = 720;
    run_for(HVAC_MIN_RUN_HOLD_MS);

    g_in.temp_f10 = HEAT_SP - 10;
    run_for(HVAC_MIN_OFF_MS); // only the minimum-off applies
    TEST_ASSERT_TRUE(g_status.out.b_y1);
}

static void test_auto_after_eheat_waits_for_changeover(void)
{
    g_in.mode     = HVAC_MODE_EHEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();

    g_in.mode     = HVAC_MODE_AUTO;
    g_in.temp_f10 = COOL_SP + 10;
    run_for(HVAC_CHANGEOVER_IDLE_MS);
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);

    run_for(STEP_MS);
    TEST_ASSERT_EQUAL(HVAC_CALL_COOL, g_status.call);
}

static void test_auto_inverted_setpoints_call_nothing(void)
{
    settle_after_boot();
    g_in.mode        = HVAC_MODE_AUTO;
    g_in.heat_sp_f10 = COOL_SP;
    g_in.cool_sp_f10 = HEAT_SP;
    g_in.temp_f10    = 720; // both "too cold" and "too hot"
    step();
    TEST_ASSERT_EQUAL(HVAC_CALL_NONE, g_status.call);
}

static void test_auto_min_run_holds(void)
{
    settle_after_boot();
    g_in.mode     = HVAC_MODE_AUTO;
    g_in.temp_f10 = COOL_SP + 10;
    step();
    g_in.temp_f10 = 700;
    run_for(HVAC_MIN_RUN_HOLD_MS - STEP_MS);
    TEST_ASSERT_TRUE(g_status.out.b_y1);
}

/* ---- Setpoints ----------------------------------------------------------- */

static void test_setpoints_push_cool_up(void)
{
    int16_t heat = 750;
    int16_t cool = 760;

    hvac_setpoints_apply(&heat, &cool, true);
    TEST_ASSERT_EQUAL_INT16(750, heat);
    TEST_ASSERT_EQUAL_INT16(780, cool);
}

static void test_setpoints_push_heat_down(void)
{
    int16_t heat = 700;
    int16_t cool = 710;

    hvac_setpoints_apply(&heat, &cool, false);
    TEST_ASSERT_EQUAL_INT16(680, heat);
    TEST_ASSERT_EQUAL_INT16(710, cool);
}

static void test_setpoints_clamp_to_limits(void)
{
    int16_t heat = 2000;
    int16_t cool = 2000;

    hvac_setpoints_apply(&heat, &cool, true);
    TEST_ASSERT_EQUAL_INT16(HVAC_HEAT_SP_MAX_F10, heat);
    TEST_ASSERT_EQUAL_INT16(HVAC_COOL_SP_MAX_F10, cool);

    heat = -500;
    cool = -500;
    hvac_setpoints_apply(&heat, &cool, false);
    TEST_ASSERT_EQUAL_INT16(HVAC_HEAT_SP_MIN_F10, heat);
    TEST_ASSERT_EQUAL_INT16(HVAC_COOL_SP_MIN_F10, cool);
    TEST_ASSERT_TRUE(hvac_setpoints_valid(heat, cool));
}

static void test_setpoints_at_the_edges_keep_the_deadband(void)
{
    int16_t heat = HVAC_HEAT_SP_MAX_F10;
    int16_t cool = HVAC_COOL_SP_MIN_F10;

    hvac_setpoints_apply(&heat, &cool, true);
    TEST_ASSERT_TRUE(hvac_setpoints_valid(heat, cool));
    TEST_ASSERT_EQUAL_INT16(HVAC_HEAT_SP_MAX_F10, heat);

    heat = HVAC_HEAT_SP_MAX_F10;
    cool = HVAC_COOL_SP_MIN_F10;
    hvac_setpoints_apply(&heat, &cool, false);
    TEST_ASSERT_TRUE(hvac_setpoints_valid(heat, cool));
    TEST_ASSERT_EQUAL_INT16(HVAC_COOL_SP_MIN_F10, cool);

    hvac_setpoints_apply(NULL, &cool, true); // must not crash
}

static void test_setpoints_valid(void)
{
    TEST_ASSERT_TRUE(hvac_setpoints_valid(680, 710));
    TEST_ASSERT_FALSE(hvac_setpoints_valid(681, 710));
    TEST_ASSERT_FALSE(hvac_setpoints_valid(399, 760));
    TEST_ASSERT_FALSE(hvac_setpoints_valid(680, 991));
}

/* ---- Feedback from what was really driven -------------------------------- */

// Steps once with `applied` as what the guard really drove last time.
static void step_applied(bool b_y1, bool b_g, bool b_o, bool b_w)
{
    g_in.b_applied_valid = true;
    g_in.applied.b_y1    = b_y1;
    g_in.applied.b_g     = b_g;
    g_in.applied.b_o     = b_o;
    g_in.applied.b_w     = b_w;
    g_in.now_ms += STEP_MS;
    step();
}

static void test_refused_start_is_retried_without_a_new_wait(void)
{
    start_heating();                          // asks for Y1 + G
    step_applied(false, false, false, false); // the guard refused it
    expect_out(true, true, false, false);     // asked again at once
    TEST_ASSERT_FALSE(g_status.b_waiting);
}

static void test_min_run_counts_from_the_real_start(void)
{
    start_heating();                          // asked at t0, refused
    step_applied(false, false, false, false); // asked again at t0 + 1 s
    g_in.temp_f10 = HEAT_SP + 20;             // satisfied at once

    // From here every request is applied as asked.
    while (g_in.now_ms < START_MS + HVAC_MIN_OFF_MS + HVAC_MIN_RUN_HOLD_MS)
    {
        step_applied(g_status.out.b_y1, g_status.out.b_g, g_status.out.b_o,
                     g_status.out.b_w);
    }
    // Timed from t0 the run would be over; from the real start it is not.
    TEST_ASSERT_TRUE(g_status.out.b_y1);
    step_applied(true, true, false, false);
    TEST_ASSERT_FALSE(g_status.out.b_y1);
}

static void test_unexpected_stop_rearms_min_off(void)
{
    start_heating();
    step_applied(true, true, false, false);
    step_applied(false, false, false, false); // stopped without being told
    TEST_ASSERT_FALSE(g_status.out.b_y1);
    TEST_ASSERT_TRUE(g_status.b_waiting);

    while (g_in.now_ms < START_MS + (2U * HVAC_MIN_OFF_MS))
    {
        step_applied(g_status.out.b_y1, g_status.out.b_g, g_status.out.b_o,
                     g_status.out.b_w);
        TEST_ASSERT_FALSE(g_status.out.b_y1);
    }
    run_for(3U * SEC);
    TEST_ASSERT_TRUE(g_status.out.b_y1);
}

static void test_refused_valve_change_is_asked_again(void)
{
    start_cooling();
    g_in.temp_f10 = COOL_SP - 10;
    run_for(HVAC_MIN_RUN_HOLD_MS + HVAC_MIN_OFF_MS); // idle, O held on
    g_in.mode     = HVAC_MODE_HEAT;
    g_in.temp_f10 = HEAT_SP - 10;
    step();
    expect_out(true, true, false, false); // O off and Y1 on together

    step_applied(false, false, true, false); // the guard kept O and Y1
    expect_out(true, true, false, false);    // asked again, no new wait
}

/* ---- Safety invariants over a long mixed run ----------------------------- */

// Drives every mode through a long, varied temperature trace and checks the
// hard rules on every step: no Y1 without G, no W in cool, never Y1 in
// e-heat, and Y1 and O timing from the outputs alone.
static void test_invariants_hold_over_a_long_run(void)
{
    uint64_t       last_y1_off = START_MS;
    uint64_t       last_y1_on  = 0U;
    hvac_outputs_t prev        = { 0 };
    uint32_t       step_idx    = 0U;

    for (step_idx = 0U; step_idx < 40000U; step_idx++)
    {
        g_in.now_ms += STEP_MS;
        // Mode changes every ~23 min; temperature swings every ~7 min.
        g_in.mode         = (hvac_mode_t)((step_idx / 1381U) % 5U); // 0..4 fits
        g_in.fan          = (hvac_fan_t)((step_idx / 4007U) % 2U);  // 0..1 fits
        g_in.temp_f10     = (int16_t)(600 + (int32_t)((step_idx * 7U) % 260U));
        g_in.b_temp_valid = ((step_idx % 9973U) > 30U);
        step();

        if (g_status.out.b_y1 || g_status.out.b_w)
        {
            TEST_ASSERT_TRUE(g_status.out.b_g);
        }
        if (g_status.out.b_w)
        {
            TEST_ASSERT_FALSE(g_status.out.b_o && g_status.out.b_y1);
        }
        if (HVAC_MODE_EHEAT == g_in.mode)
        {
            TEST_ASSERT_FALSE(g_status.out.b_y1);
        }
        if (g_status.out.b_y1 && !prev.b_y1)
        {
            TEST_ASSERT_TRUE(g_in.now_ms - last_y1_off >= HVAC_MIN_OFF_MS);
            last_y1_on = g_in.now_ms;
        }
        if (!g_status.out.b_y1 && prev.b_y1)
        {
            last_y1_off = g_in.now_ms;
        }
        if (g_status.out.b_o != prev.b_o)
        {
            // O may change only with Y1 off (or a fault forcing all off).
            TEST_ASSERT_FALSE(prev.b_y1 && g_status.out.b_y1);
            if (!g_status.b_fault)
            {
                TEST_ASSERT_FALSE(prev.b_y1);
                TEST_ASSERT_TRUE(g_in.now_ms - last_y1_off >= HVAC_MIN_OFF_MS);
            }
        }
        prev = g_status.out;
    }

    TEST_ASSERT_TRUE(last_y1_on > 0U); // the trace did run the compressor
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_boot_is_all_off_with_min_off_armed);
    RUN_TEST(test_heat_at_boot_waits_five_minutes);
    RUN_TEST(test_min_run_holds_a_satisfied_call);
    RUN_TEST(test_min_off_after_a_run);
    RUN_TEST(test_mode_off_waives_min_run);
    RUN_TEST(test_clock_going_backwards_does_not_start_y1);
    RUN_TEST(test_heat_hysteresis);
    RUN_TEST(test_cool_hysteresis);
    RUN_TEST(test_cool_sets_o_with_y1);
    RUN_TEST(test_o_is_held_while_idle);
    RUN_TEST(test_o_changes_only_after_min_off);
    RUN_TEST(test_heat_to_cool_holds_the_run_then_waits);
    RUN_TEST(test_cool_to_heat_holds_the_run_then_waits);
    RUN_TEST(test_heat_run_held_into_cool_drops_aux);
    RUN_TEST(test_aux_added_three_degrees_below);
    RUN_TEST(test_aux_dropped_within_one_degree);
    RUN_TEST(test_aux_waits_for_the_compressor);
    RUN_TEST(test_aux_added_when_still_falling);
    RUN_TEST(test_aux_not_added_when_holding);
    RUN_TEST(test_aux_not_added_by_trend_within_one_degree);
    RUN_TEST(test_aux_never_in_cool);
    RUN_TEST(test_eheat_is_g_and_w_without_waiting);
    RUN_TEST(test_eheat_ends_with_hysteresis);
    RUN_TEST(test_eheat_from_heat_stops_y1_at_once);
    RUN_TEST(test_eheat_after_cool_releases_o_after_min_off);
    RUN_TEST(test_fault_turns_everything_off);
    RUN_TEST(test_fault_drops_o_inside_min_off_and_rearms);
    RUN_TEST(test_fault_rearms_min_off);
    RUN_TEST(test_fault_during_eheat);
    RUN_TEST(test_null_arguments_are_all_off);
    RUN_TEST(test_bad_mode_is_off);
    RUN_TEST(test_fan_on_runs_g_when_idle);
    RUN_TEST(test_purge_after_heat);
    RUN_TEST(test_no_purge_when_zero);
    RUN_TEST(test_purge_is_capped);
    RUN_TEST(test_no_blower_while_waiting);
    RUN_TEST(test_auto_heats_and_cools);
    RUN_TEST(test_auto_changeover_waits_ten_minutes_idle);
    RUN_TEST(test_auto_same_direction_needs_no_changeover);
    RUN_TEST(test_auto_after_eheat_waits_for_changeover);
    RUN_TEST(test_auto_inverted_setpoints_call_nothing);
    RUN_TEST(test_auto_min_run_holds);
    RUN_TEST(test_setpoints_push_cool_up);
    RUN_TEST(test_setpoints_push_heat_down);
    RUN_TEST(test_setpoints_clamp_to_limits);
    RUN_TEST(test_setpoints_at_the_edges_keep_the_deadband);
    RUN_TEST(test_setpoints_valid);
    RUN_TEST(test_refused_start_is_retried_without_a_new_wait);
    RUN_TEST(test_min_run_counts_from_the_real_start);
    RUN_TEST(test_unexpected_stop_rearms_min_off);
    RUN_TEST(test_refused_valve_change_is_asked_again);
    RUN_TEST(test_invariants_hold_over_a_long_run);
    return UNITY_END();
}
