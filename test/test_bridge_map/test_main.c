/**
 * @file  test_main.c
 * @brief Host tests for the thermostat <-> Matter value mapping.
 *
 * Runs with `pio test -e native`. The module under test is compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 */
#include "../../components/hvac_logic/hvac_logic.c"
#include "../../matter/components/matter_bridge/bridge_map.c"

#include <unity.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_known_temperatures(void)
{
    TEST_ASSERT_EQUAL_INT16(0, bridge_map_f10_to_c100(320));
    TEST_ASSERT_EQUAL_INT16(2000, bridge_map_f10_to_c100(680));
    TEST_ASSERT_EQUAL_INT16(-4000, bridge_map_f10_to_c100(-400));
    TEST_ASSERT_EQUAL_INT16(680, bridge_map_c100_to_f10(2000));
    TEST_ASSERT_EQUAL_INT16(320, bridge_map_c100_to_f10(0));
    TEST_ASSERT_EQUAL_INT16(-400, bridge_map_c100_to_f10(-4000));
}

static void test_apple_fahrenheit_steps_round_trip(void)
{
    int16_t f10 = 0;

    // Whole degrees F reported as C100 must come back to the same whole
    // degree F.
    for (f10 = 400; f10 <= 990; f10 = (int16_t)(f10 + 10)) // fits int16_t
    {
        TEST_ASSERT_EQUAL_INT16(
            f10, bridge_map_c100_to_f10(bridge_map_f10_to_c100(f10)));
    }
}

static void test_setpoint_writes_snap_to_whole_fahrenheit(void)
{
    int16_t f10  = 0;
    int16_t c100 = 0;
    int16_t off  = 0;

    // 65 F arrived from the Home app as 65.1 and 65.2 F (Oct 2026).
    TEST_ASSERT_EQUAL_INT16(650, bridge_map_setpoint_from_c100(1840, false));
    TEST_ASSERT_EQUAL_INT16(650, bridge_map_setpoint_from_c100(1844, false));
    TEST_ASSERT_EQUAL_INT16(650, bridge_map_setpoint_from_c100(1833, false));

    // Any rounding of a whole degree F to within 0.25 C still lands on it.
    for (f10 = 600; f10 <= 800; f10 = (int16_t)(f10 + 10)) // fits int16_t
    {
        c100 = bridge_map_f10_to_c100(f10);
        for (off = -25; off <= 25; off++)
        {
            // c100 is a few thousand, so the sum fits int16_t.
            TEST_ASSERT_EQUAL_INT16(f10, bridge_map_setpoint_from_c100(
                                             (int16_t)(c100 + off), false));
        }
    }
}

static void test_setpoint_writes_snap_to_half_celsius(void)
{
    // 18.4 C -> 18.5 C = 65.3 F; 21.0 C stays 69.8 F; 20.74 C -> 20.5 C.
    TEST_ASSERT_EQUAL_INT16(653, bridge_map_setpoint_from_c100(1840, true));
    TEST_ASSERT_EQUAL_INT16(698, bridge_map_setpoint_from_c100(2100, true));
    TEST_ASSERT_EQUAL_INT16(689, bridge_map_setpoint_from_c100(2074, true));
}

static void test_fixed_attributes(void)
{
    uint32_t const fixed[] = {
        BRIDGE_ATTR_MIN_HEAT_LIMIT, BRIDGE_ATTR_MAX_HEAT_LIMIT,
        BRIDGE_ATTR_MIN_COOL_LIMIT, BRIDGE_ATTR_MAX_COOL_LIMIT,
        BRIDGE_ATTR_MIN_DEADBAND,   BRIDGE_ATTR_CONTROL_SEQUENCE,
    };
    uint32_t idx = 0U;

    for (idx = 0U; idx < (sizeof(fixed) / sizeof(fixed[0])); idx++)
    {
        TEST_ASSERT_TRUE(
            bridge_map_attr_fixed(BRIDGE_CLUSTER_THERMOSTAT, fixed[idx]));
    }
    TEST_ASSERT_TRUE(bridge_map_attr_fixed(BRIDGE_CLUSTER_ON_OFF,
                                           BRIDGE_ATTR_START_UP_ON_OFF));

    // What Home must be able to write stays writable: SystemMode (0x1C),
    // the occupied setpoints (0x11, 0x12), OnOff itself (0x0) and the On/Off
    // server's own timers (OnTime 0x4001, OffWaitTime 0x4002).
    TEST_ASSERT_FALSE(bridge_map_attr_fixed(BRIDGE_CLUSTER_THERMOSTAT, 0x1CU));
    TEST_ASSERT_FALSE(bridge_map_attr_fixed(BRIDGE_CLUSTER_THERMOSTAT, 0x11U));
    TEST_ASSERT_FALSE(bridge_map_attr_fixed(BRIDGE_CLUSTER_THERMOSTAT, 0x12U));
    TEST_ASSERT_FALSE(bridge_map_attr_fixed(BRIDGE_CLUSTER_ON_OFF, 0x0U));
    TEST_ASSERT_FALSE(bridge_map_attr_fixed(BRIDGE_CLUSTER_ON_OFF, 0x4001U));
    TEST_ASSERT_FALSE(bridge_map_attr_fixed(BRIDGE_CLUSTER_ON_OFF, 0x4002U));
    // The same IDs on another cluster are not fixed.
    TEST_ASSERT_FALSE(bridge_map_attr_fixed(0x0202U, BRIDGE_ATTR_MIN_DEADBAND));
}

static void test_conversion_saturates(void)
{
    TEST_ASSERT_EQUAL_INT16(INT16_MAX, bridge_map_f10_to_c100(INT16_MAX));
    TEST_ASSERT_EQUAL_INT16(INT16_MIN, bridge_map_f10_to_c100(INT16_MIN));
    // C100 -> F10 shrinks the value, so the top of the range cannot saturate.
    TEST_ASSERT_EQUAL_INT16(6218, bridge_map_c100_to_f10(INT16_MAX));
}

static void test_system_mode_out(void)
{
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_SYSTEM_MODE_OFF,
                            bridge_map_system_mode(HVAC_MODE_OFF));
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_SYSTEM_MODE_HEAT,
                            bridge_map_system_mode(HVAC_MODE_HEAT));
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_SYSTEM_MODE_COOL,
                            bridge_map_system_mode(HVAC_MODE_COOL));
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_SYSTEM_MODE_AUTO,
                            bridge_map_system_mode(HVAC_MODE_AUTO));
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_SYSTEM_MODE_HEAT,
                            bridge_map_system_mode(HVAC_MODE_EHEAT));
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_SYSTEM_MODE_OFF,
                            bridge_map_system_mode(99U));
}

static void test_system_mode_in(void)
{
    uint8_t mode = 77U;

    TEST_ASSERT_TRUE(bridge_map_mode_from_system(BRIDGE_SYSTEM_MODE_COOL,
                                                 HVAC_MODE_OFF, &mode));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_COOL, mode);
    TEST_ASSERT_TRUE(bridge_map_mode_from_system(BRIDGE_SYSTEM_MODE_HEAT,
                                                 HVAC_MODE_COOL, &mode));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_HEAT, mode);

    // Fan-only (7) is not something this thermostat does.
    mode = 77U;
    TEST_ASSERT_FALSE(bridge_map_mode_from_system(7U, HVAC_MODE_OFF, &mode));
    TEST_ASSERT_EQUAL_UINT8(77U, mode);
    TEST_ASSERT_FALSE(bridge_map_mode_from_system(BRIDGE_SYSTEM_MODE_HEAT,
                                                  HVAC_MODE_OFF, NULL));
}

static void test_eheat_ignores_heat_but_not_others(void)
{
    uint8_t mode = 0U;

    TEST_ASSERT_TRUE(bridge_map_mode_from_system(BRIDGE_SYSTEM_MODE_HEAT,
                                                 HVAC_MODE_EHEAT, &mode));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_EHEAT, mode);
    TEST_ASSERT_TRUE(bridge_map_mode_from_system(BRIDGE_SYSTEM_MODE_OFF,
                                                 HVAC_MODE_EHEAT, &mode));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_OFF, mode);
}

static void test_eheat_switch(void)
{
    TEST_ASSERT_EQUAL_UINT8(
        HVAC_MODE_EHEAT,
        bridge_map_mode_from_eheat(true, HVAC_MODE_COOL, HVAC_MODE_COOL));
    // Off goes back to the mode e-heat was entered from.
    TEST_ASSERT_EQUAL_UINT8(
        HVAC_MODE_COOL,
        bridge_map_mode_from_eheat(false, HVAC_MODE_EHEAT, HVAC_MODE_COOL));
    TEST_ASSERT_EQUAL_UINT8(
        HVAC_MODE_OFF,
        bridge_map_mode_from_eheat(false, HVAC_MODE_EHEAT, HVAC_MODE_OFF));
    TEST_ASSERT_EQUAL_UINT8(
        HVAC_MODE_AUTO,
        bridge_map_mode_from_eheat(false, HVAC_MODE_EHEAT, HVAC_MODE_AUTO));
    TEST_ASSERT_EQUAL_UINT8(
        HVAC_MODE_HEAT,
        bridge_map_mode_from_eheat(false, HVAC_MODE_EHEAT, HVAC_MODE_HEAT));
    // Unknown, or e-heat itself: Heat.
    TEST_ASSERT_EQUAL_UINT8(
        HVAC_MODE_HEAT,
        bridge_map_mode_from_eheat(false, HVAC_MODE_EHEAT, HVAC_MODE_EHEAT));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_HEAT, bridge_map_mode_from_eheat(
                                                false, HVAC_MODE_EHEAT, 99U));
    // Off when not in e-heat changes nothing.
    TEST_ASSERT_EQUAL_UINT8(
        HVAC_MODE_COOL,
        bridge_map_mode_from_eheat(false, HVAC_MODE_COOL, HVAC_MODE_OFF));
}

static void test_fan_modes(void)
{
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_FAN_MODE_HIGH,
                            bridge_map_fan_mode(HVAC_FAN_ON));
    TEST_ASSERT_EQUAL_UINT8(BRIDGE_FAN_MODE_AUTO,
                            bridge_map_fan_mode(HVAC_FAN_AUTO));
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_ON,
                            bridge_map_fan_from_mode(BRIDGE_FAN_MODE_HIGH));
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_ON,
                            bridge_map_fan_from_mode(BRIDGE_FAN_MODE_ON));
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_AUTO,
                            bridge_map_fan_from_mode(BRIDGE_FAN_MODE_OFF));
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_AUTO,
                            bridge_map_fan_from_mode(BRIDGE_FAN_MODE_AUTO));
}

static void test_running_state(void)
{
    TEST_ASSERT_EQUAL_HEX16(
        0U, bridge_map_running_state(false, false, false, false));
    TEST_ASSERT_EQUAL_HEX16(BRIDGE_RUNNING_HEAT | BRIDGE_RUNNING_FAN,
                            bridge_map_running_state(true, true, false, false));
    TEST_ASSERT_EQUAL_HEX16(BRIDGE_RUNNING_COOL | BRIDGE_RUNNING_FAN,
                            bridge_map_running_state(true, true, true, false));
    TEST_ASSERT_EQUAL_HEX16(BRIDGE_RUNNING_HEAT | BRIDGE_RUNNING_FAN |
                                BRIDGE_RUNNING_HEAT_STAGE2,
                            bridge_map_running_state(true, true, false, true));
    // E-heat: strips alone are heat, not stage 2.
    TEST_ASSERT_EQUAL_HEX16(BRIDGE_RUNNING_HEAT | BRIDGE_RUNNING_FAN,
                            bridge_map_running_state(false, true, false, true));
    // O held while idle reports nothing.
    TEST_ASSERT_EQUAL_HEX16(
        0U, bridge_map_running_state(false, false, true, false));
}

static void test_limits_are_the_converted_ends(void)
{
    bridge_limits_t lim = { 0 };

    bridge_map_limits(&lim);
    TEST_ASSERT_EQUAL_INT16(bridge_map_f10_to_c100(HVAC_HEAT_SP_MIN_F10),
                            lim.min_heat_c100);
    TEST_ASSERT_EQUAL_INT16(bridge_map_f10_to_c100(HVAC_HEAT_SP_MAX_F10),
                            lim.max_heat_c100);
    TEST_ASSERT_EQUAL_INT16(bridge_map_f10_to_c100(HVAC_COOL_SP_MIN_F10),
                            lim.min_cool_c100);
    TEST_ASSERT_EQUAL_INT16(bridge_map_f10_to_c100(HVAC_COOL_SP_MAX_F10),
                            lim.max_cool_c100);
    bridge_map_limits(NULL); // must not crash
}

static void test_every_reported_setpoint_is_inside_the_limits(void)
{
    bridge_limits_t lim = { 0 };
    int16_t         f10 = 0;
    int16_t         c   = 0;

    bridge_map_limits(&lim);
    // f10 stays within a few hundred, so each step fits int16_t.
    for (f10 = HVAC_HEAT_SP_MIN_F10; f10 <= HVAC_HEAT_SP_MAX_F10;
         f10 = (int16_t)(f10 + 1))
    {
        c = bridge_map_f10_to_c100(f10);
        TEST_ASSERT_TRUE((c >= lim.min_heat_c100) && (c <= lim.max_heat_c100));
    }
    for (f10 = HVAC_COOL_SP_MIN_F10; f10 <= HVAC_COOL_SP_MAX_F10;
         f10 = (int16_t)(f10 + 1))
    {
        c = bridge_map_f10_to_c100(f10);
        TEST_ASSERT_TRUE((c >= lim.min_cool_c100) && (c <= lim.max_cool_c100));
    }
}

static void test_every_write_inside_the_limits_gives_a_valid_pair(void)
{
    bridge_limits_t lim  = { 0 };
    int16_t         c    = 0;
    int16_t         heat = 0;
    int16_t         cool = 0;
    uint32_t        unit = 0U;

    bridge_map_limits(&lim);
    for (unit = 0U; unit < 2U; unit++)
    {
        // c stays within a few thousand, so each step fits int16_t.
        for (c = lim.min_heat_c100; c <= lim.max_heat_c100;
             c = (int16_t)(c + 1))
        {
            heat = 680;
            cool = 760;
            bridge_map_setpoint_write(c, (1U == unit), true, &heat, &cool);
            TEST_ASSERT_TRUE(hvac_setpoints_valid(heat, cool));
        }
        for (c = lim.min_cool_c100; c <= lim.max_cool_c100;
             c = (int16_t)(c + 1))
        {
            heat = 680;
            cool = 760;
            bridge_map_setpoint_write(c, (1U == unit), false, &heat, &cool);
            TEST_ASSERT_TRUE(hvac_setpoints_valid(heat, cool));
        }
    }
}

static void test_setpoint_write_leads_with_the_written_one(void)
{
    int16_t heat = 680;
    int16_t cool = 720;

    // 74 F heat pushes cool up to keep the 3 F gap.
    bridge_map_setpoint_write(bridge_map_f10_to_c100(740), false, true, &heat,
                              &cool);
    TEST_ASSERT_EQUAL_INT16(740, heat);
    TEST_ASSERT_EQUAL_INT16(770, cool);

    // 65 F cool, as the Home app sent it (18.44 C), pulls heat down.
    bridge_map_setpoint_write(1844, false, false, &heat, &cool);
    TEST_ASSERT_EQUAL_INT16(650, cool);
    TEST_ASSERT_EQUAL_INT16(620, heat);

    bridge_map_setpoint_write(1844, false, false, NULL, &cool); // no crash
}

static void test_limits_meet_matter_rules(void)
{
    bridge_limits_t lim = { 0 };

    bridge_map_limits(&lim);

    // Matter: MinHeat <= MinCool - deadband, MaxHeat <= MaxCool - deadband.
    TEST_ASSERT_TRUE(lim.min_heat_c100 + (BRIDGE_DEADBAND_C10 * 10) <=
                     lim.min_cool_c100);
    TEST_ASSERT_TRUE(lim.max_heat_c100 + (BRIDGE_DEADBAND_C10 * 10) <=
                     lim.max_cool_c100);
}

static void test_every_thermostat_pair_meets_the_cluster_deadband(void)
{
    int16_t heat = 0;

    // The cluster requires cool - heat >= MinSetpointDeadBand as reported;
    // the thermostat's tightest pair is heat + HVAC_AUTO_DEADBAND_F10.
    for (heat = HVAC_HEAT_SP_MIN_F10; heat <= HVAC_HEAT_SP_MAX_F10;
         heat = (int16_t)(heat + 1)) // fits int16_t
    {
        TEST_ASSERT_TRUE(
            (bridge_map_f10_to_c100(
                 (int16_t)(heat + HVAC_AUTO_DEADBAND_F10)) - // fits int16_t
             bridge_map_f10_to_c100(heat)) >= (BRIDGE_DEADBAND_C10 * 10));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_known_temperatures);
    RUN_TEST(test_apple_fahrenheit_steps_round_trip);
    RUN_TEST(test_setpoint_writes_snap_to_whole_fahrenheit);
    RUN_TEST(test_setpoint_writes_snap_to_half_celsius);
    RUN_TEST(test_fixed_attributes);
    RUN_TEST(test_conversion_saturates);
    RUN_TEST(test_system_mode_out);
    RUN_TEST(test_system_mode_in);
    RUN_TEST(test_eheat_ignores_heat_but_not_others);
    RUN_TEST(test_eheat_switch);
    RUN_TEST(test_fan_modes);
    RUN_TEST(test_running_state);
    RUN_TEST(test_limits_are_the_converted_ends);
    RUN_TEST(test_every_reported_setpoint_is_inside_the_limits);
    RUN_TEST(test_every_write_inside_the_limits_gives_a_valid_pair);
    RUN_TEST(test_setpoint_write_leads_with_the_written_one);
    RUN_TEST(test_limits_meet_matter_rules);
    RUN_TEST(test_every_thermostat_pair_meets_the_cluster_deadband);
    return UNITY_END();
}
