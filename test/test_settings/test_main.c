/**
 * @file  test_main.c
 * @brief Host tests for the settings defaults, range checks, and reading a
 *        stored record (the current version, and the v1 migration).
 *
 * Runs with `pio test -e native`. The modules under test are compiled into
 * this translation unit (see docs/CODING_STANDARD.md, deviation D4);
 * hvac_logic.c comes along for the setpoint rules.
 */
#include "../../components/hvac_logic/hvac_logic.c"
#include "../../components/settings/settings_logic.c"

#include <unity.h>

static settings_t g_cfg = { 0 };

void setUp(void)
{
    settings_logic_defaults(&g_cfg);
}

void tearDown(void)
{
}

static void test_defaults_are_valid_and_off(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_sanitise(&g_cfg));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_OFF, g_cfg.mode);
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_AUTO, g_cfg.fan);
    TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, g_cfg.version);
    TEST_ASSERT_TRUE(
        hvac_setpoints_valid(g_cfg.heat_sp_f10, g_cfg.cool_sp_f10));
}

static void test_bad_setpoints_reset_as_a_pair(void)
{
    g_cfg.heat_sp_f10 = 750;
    g_cfg.cool_sp_f10 = 760; // breaks the 3 F deadband
    TEST_ASSERT_EQUAL_UINT32(1U, settings_logic_sanitise(&g_cfg));
    TEST_ASSERT_EQUAL_INT16(680, g_cfg.heat_sp_f10);
    TEST_ASSERT_EQUAL_INT16(760, g_cfg.cool_sp_f10);
}

static void test_each_bad_field_resets_alone(void)
{
    g_cfg.cal_offset_f10    = 51;
    g_cfg.fan_purge_s       = HVAC_FAN_PURGE_MAX_S + 1U;
    g_cfg.display_timeout_s = SETTINGS_DISPLAY_TIMEOUT_MAX + 1U;
    g_cfg.screen_off_s      = SETTINGS_SCREEN_OFF_MAX + 1U;
    g_cfg.mode              = 9U;
    g_cfg.fan               = 2U;
    g_cfg.units             = 7U;
    g_cfg.brightness        = 0U;
    g_cfg.heat_sp_f10       = 650; // valid, must survive

    TEST_ASSERT_EQUAL_UINT32(8U, settings_logic_sanitise(&g_cfg));
    TEST_ASSERT_EQUAL_INT16(0, g_cfg.cal_offset_f10);
    TEST_ASSERT_EQUAL_UINT16(60U, g_cfg.fan_purge_s);
    TEST_ASSERT_EQUAL_UINT16(30U, g_cfg.display_timeout_s);
    TEST_ASSERT_EQUAL_UINT16(600U, g_cfg.screen_off_s);
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_OFF, g_cfg.mode);
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_AUTO, g_cfg.fan);
    TEST_ASSERT_EQUAL_UINT8(SETTINGS_UNITS_F, g_cfg.units);
    TEST_ASSERT_EQUAL_UINT8(200U, g_cfg.brightness);
    TEST_ASSERT_EQUAL_INT16(650, g_cfg.heat_sp_f10);
}

static void test_limits_are_inclusive(void)
{
    g_cfg.cal_offset_f10    = -SETTINGS_CAL_LIMIT_F10;
    g_cfg.fan_purge_s       = HVAC_FAN_PURGE_MAX_S;
    g_cfg.display_timeout_s = 0U;
    g_cfg.screen_off_s      = SETTINGS_SCREEN_OFF_MAX;
    g_cfg.mode              = (uint8_t)HVAC_MODE_EHEAT; // 4 fits
    g_cfg.brightness        = SETTINGS_BRIGHTNESS_MIN;
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_sanitise(&g_cfg));
}

static void test_version_is_stamped(void)
{
    g_cfg.version = 0U;
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_sanitise(&g_cfg));
    TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, g_cfg.version);
}

static void test_defaults_turn_the_screen_off_after_ten_minutes(void)
{
    TEST_ASSERT_EQUAL_UINT16(600U, g_cfg.screen_off_s);
}

static void test_current_record_reads_as_is(void)
{
    settings_t stored = { 0 };
    settings_t out    = { 0 };

    settings_logic_defaults(&stored);
    stored.heat_sp_f10  = 650;
    stored.screen_off_s = 120U;
    TEST_ASSERT_EQUAL_UINT32(
        SETTINGS_VERSION,
        settings_logic_from_blob(&stored, sizeof(stored), &out));
    TEST_ASSERT_EQUAL_MEMORY(&stored, &out, sizeof(out));
}

static void test_v1_record_migrates(void)
{
    settings_v1_t v1  = { 0 };
    settings_t    out = { 0 };

    v1.version           = SETTINGS_V1_VERSION;
    v1.heat_sp_f10       = 620;
    v1.cool_sp_f10       = 650;
    v1.cal_offset_f10    = -12;
    v1.fan_purge_s       = 90U;
    v1.display_timeout_s = 45U;
    v1.mode              = (uint8_t)HVAC_MODE_COOL; // 2 fits
    v1.fan               = (uint8_t)HVAC_FAN_ON;    // 1 fits
    v1.units             = (uint8_t)SETTINGS_UNITS_C;
    v1.brightness        = 128U;

    TEST_ASSERT_EQUAL_UINT32(SETTINGS_V1_VERSION,
                             settings_logic_from_blob(&v1, sizeof(v1), &out));
    // Every v1 field kept; the new one gets its default; version is current.
    TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
    TEST_ASSERT_EQUAL_INT16(620, out.heat_sp_f10);
    TEST_ASSERT_EQUAL_INT16(650, out.cool_sp_f10);
    TEST_ASSERT_EQUAL_INT16(-12, out.cal_offset_f10);
    TEST_ASSERT_EQUAL_UINT16(90U, out.fan_purge_s);
    TEST_ASSERT_EQUAL_UINT16(45U, out.display_timeout_s);
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_COOL, out.mode);
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_ON, out.fan);
    TEST_ASSERT_EQUAL_UINT8(SETTINGS_UNITS_C, out.units);
    TEST_ASSERT_EQUAL_UINT8(128U, out.brightness);
    TEST_ASSERT_EQUAL_UINT16(600U, out.screen_off_s);
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_sanitise(&out));
}

static void test_unknown_blobs_are_refused(void)
{
    settings_t    cfg     = { 0 };
    settings_t    out     = { 0 };
    settings_v1_t v1      = { 0 };
    uint8_t       big[32] = { 0 };

    settings_logic_defaults(&cfg);
    out.heat_sp_f10 = 123; // must survive every refusal

    cfg.version = 3U; // a newer firmware's
    TEST_ASSERT_EQUAL_UINT32(0U,
                             settings_logic_from_blob(&cfg, sizeof(cfg), &out));
    cfg.version = SETTINGS_VERSION; // right version, wrong size
    TEST_ASSERT_EQUAL_UINT32(
        0U, settings_logic_from_blob(&cfg, sizeof(cfg) - 1U, &out));
    v1.version = SETTINGS_V1_VERSION; // v1 tag on a v2-sized blob
    (void)memcpy(big, &v1, sizeof(v1));
    TEST_ASSERT_EQUAL_UINT32(0U,
                             settings_logic_from_blob(big, sizeof(cfg), &out));
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_from_blob(big, 1U, &out));
    TEST_ASSERT_EQUAL_UINT32(0U,
                             settings_logic_from_blob(NULL, sizeof(cfg), &out));
    TEST_ASSERT_EQUAL_UINT32(0U,
                             settings_logic_from_blob(&cfg, sizeof(cfg), NULL));
    TEST_ASSERT_EQUAL_INT16(123, out.heat_sp_f10);
}

static void test_null_is_harmless(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_sanitise(NULL));
    settings_logic_defaults(NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_valid_and_off);
    RUN_TEST(test_bad_setpoints_reset_as_a_pair);
    RUN_TEST(test_each_bad_field_resets_alone);
    RUN_TEST(test_limits_are_inclusive);
    RUN_TEST(test_version_is_stamped);
    RUN_TEST(test_defaults_turn_the_screen_off_after_ten_minutes);
    RUN_TEST(test_current_record_reads_as_is);
    RUN_TEST(test_v1_record_migrates);
    RUN_TEST(test_unknown_blobs_are_refused);
    RUN_TEST(test_null_is_harmless);
    return UNITY_END();
}
