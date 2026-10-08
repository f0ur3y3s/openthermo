/**
 * @file  test_main.c
 * @brief Host tests for the bench simulated room, alone and in a closed
 *        loop with the real thermostat logic.
 *
 * Runs with `pio test -e native`. The modules under test are compiled into
 * this translation unit (see docs/CODING_STANDARD.md, deviation D4).
 */
#include "../../components/hvac_logic/hvac_logic.c"
#include "../../components/sensor/sensor_sim.c"

#include <unity.h>

#define START_MS 1000ULL
#define MID_F10  ((SENSOR_SIM_OUTDOOR_MIN_F10 + SENSOR_SIM_OUTDOOR_MAX_F10) / 2)
#define MINUTE   60000ULL

static sensor_sim_t g_sim = { 0 };

// Runs the model for `minutes` with fixed outputs, from outdoor time 0.
static int16_t run_fixed(bool b_y1, bool b_o, bool b_w, uint32_t minutes)
{
    int16_t  temp    = 0;
    uint32_t min_idx = 0U;

    for (min_idx = 1U; min_idx <= minutes; min_idx++)
    {
        temp = sensor_sim_step(&g_sim, b_y1, b_o, b_w,
                               START_MS + ((uint64_t)min_idx * MINUTE));
    }

    return temp;
}

void setUp(void)
{
    sensor_sim_init(&g_sim, START_MS);
}

void tearDown(void)
{
}

static void test_outdoor_is_a_triangle(void)
{
    TEST_ASSERT_EQUAL_INT16(SENSOR_SIM_OUTDOOR_MIN_F10,
                            sensor_sim_outdoor_f10(0U));
    TEST_ASSERT_EQUAL_INT16(SENSOR_SIM_OUTDOOR_MAX_F10,
                            sensor_sim_outdoor_f10(SENSOR_SIM_PERIOD_MS / 2U));
    TEST_ASSERT_EQUAL_INT16(MID_F10,
                            sensor_sim_outdoor_f10(SENSOR_SIM_PERIOD_MS / 4U));
    TEST_ASSERT_EQUAL_INT16(
        MID_F10, sensor_sim_outdoor_f10((3U * SENSOR_SIM_PERIOD_MS) / 4U));
    TEST_ASSERT_EQUAL_INT16(SENSOR_SIM_OUTDOOR_MIN_F10,
                            sensor_sim_outdoor_f10(SENSOR_SIM_PERIOD_MS));
}

static void test_idle_room_drifts_toward_outdoor(void)
{
    int16_t temp = run_fixed(false, false, false, 5U);

    // Outdoor starts at 50 F and rises slowly; a 70 F room cools.
    TEST_ASSERT_TRUE(temp < SENSOR_SIM_START_F10);
    TEST_ASSERT_TRUE(temp > 650);
}

static void test_heat_pump_warms(void)
{
    int16_t idle = run_fixed(false, false, false, 10U);
    int16_t heat = 0;

    // 0.6 F/min for 10 min is 6 F over the same room left alone, less the
    // extra leak of a warmer room.
    sensor_sim_init(&g_sim, START_MS);
    heat = run_fixed(true, false, false, 10U);
    TEST_ASSERT_TRUE(heat > idle + 40);
    TEST_ASSERT_TRUE(heat <= idle + 60);
}

static void test_cool_cools_faster_than_drift(void)
{
    int16_t idle = run_fixed(false, false, false, 10U);

    sensor_sim_init(&g_sim, START_MS);
    TEST_ASSERT_TRUE(run_fixed(true, true, false, 10U) < idle - 40);
}

static void test_aux_adds_to_heat(void)
{
    int16_t heat = run_fixed(true, false, false, 10U);

    sensor_sim_init(&g_sim, START_MS);
    TEST_ASSERT_TRUE(run_fixed(true, false, true, 10U) > heat + 80);
}

static void test_long_gap_is_capped(void)
{
    int16_t temp =
        sensor_sim_step(&g_sim, true, false, false, START_MS + (100U * MINUTE));

    // One capped minute of heat, not 100.
    TEST_ASSERT_INT16_WITHIN(5, SENSOR_SIM_START_F10 + 3, temp);
}

static void test_null_is_harmless(void)
{
    TEST_ASSERT_EQUAL_INT16(SENSOR_SIM_START_F10,
                            sensor_sim_step(NULL, true, false, false, 0U));
    sensor_sim_init(NULL, 0U);
}

// The bench demo: Auto, default setpoints, no keys pressed. Over one outdoor
// swing the LEDs must show heat (Y1), then cool (Y1 + O), and the room must
// stay near the setpoints the whole time.
static void test_closed_loop_auto_heats_then_cools(void)
{
    hvac_state_t  state      = { 0 };
    hvac_input_t  in         = { 0 };
    hvac_status_t status     = { 0 };
    int16_t       temp       = SENSOR_SIM_START_F10;
    uint32_t      heat_steps = 0U;
    uint32_t      cool_steps = 0U;
    uint64_t      now_ms     = START_MS;

    hvac_init(&state, START_MS);
    in.mode         = HVAC_MODE_AUTO;
    in.fan          = HVAC_FAN_AUTO;
    in.heat_sp_f10  = 680;
    in.cool_sp_f10  = 760;
    in.fan_purge_s  = 60U;
    in.b_temp_valid = true;

    while (now_ms < (START_MS + SENSOR_SIM_PERIOD_MS))
    {
        now_ms += 1000U;
        temp        = sensor_sim_step(&g_sim, status.out.b_y1, status.out.b_o,
                                      status.out.b_w, now_ms);
        in.temp_f10 = temp;
        in.now_ms   = now_ms;
        hvac_step(&state, &in, &status);

        if (status.out.b_y1 && !status.out.b_o)
        {
            heat_steps++;
        }
        if (status.out.b_y1 && status.out.b_o)
        {
            cool_steps++;
        }
        // The worst excursion is the 5 min minimum-off at full drift.
        TEST_ASSERT_TRUE(temp > 640);
        TEST_ASSERT_TRUE(temp < 800);
    }

    TEST_ASSERT_TRUE(heat_steps > 0U);
    TEST_ASSERT_TRUE(cool_steps > 0U);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_outdoor_is_a_triangle);
    RUN_TEST(test_idle_room_drifts_toward_outdoor);
    RUN_TEST(test_heat_pump_warms);
    RUN_TEST(test_cool_cools_faster_than_drift);
    RUN_TEST(test_aux_adds_to_heat);
    RUN_TEST(test_long_gap_is_capped);
    RUN_TEST(test_null_is_harmless);
    RUN_TEST(test_closed_loop_auto_heats_then_cools);
    return UNITY_END();
}
