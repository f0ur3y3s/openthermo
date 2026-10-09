/**
 * @file  test_main.c
 * @brief Host tests for the UI logic (pages_nav) and the display formatting
 *        (page_fmt).
 *
 * Runs with `pio test -e native`. The modules under test are compiled into
 * this translation unit (see docs/CODING_STANDARD.md, deviation D4);
 * hvac_logic.c and settings_logic.c come along for the setpoint rules and
 * the defaults.
 */
#include "../../components/hvac_logic/hvac_logic.c"
#include "../../components/pages/page_fmt.c"
#include "../../components/pages/pages_nav.c"
#include "../../components/settings/settings_logic.c"

#include <unity.h>

#define BUF_LEN 16U

static pages_nav_t g_nav = { 0 };
static settings_t  g_cfg = { 0 };

static bool press(buttons_key_t key)
{
    return pages_nav_key(&g_nav, &g_cfg, key, BUTTON_EVENT_PRESS);
}

void setUp(void)
{
    pages_nav_reset(&g_nav);
    settings_logic_defaults(&g_cfg);
}

void tearDown(void)
{
}

/* ---- Navigation ---------------------------------------------------------- */

static void test_left_right_wrap(void)
{
    // Main, mode, fan, settings: info and pair are not in the cycle.
    (void)press(BUTTONS_KEY_LEFT);
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
    (void)press(BUTTONS_KEY_LEFT);
    TEST_ASSERT_EQUAL(PAGES_FAN, g_nav.page);
    (void)press(BUTTONS_KEY_RIGHT);
    (void)press(BUTTONS_KEY_RIGHT);
    TEST_ASSERT_EQUAL(PAGES_MAIN, g_nav.page);
    (void)press(BUTTONS_KEY_RIGHT);
    TEST_ASSERT_EQUAL(PAGES_MODE, g_nav.page);
}

static void test_page_keys_do_not_repeat(void)
{
    TEST_ASSERT_FALSE(
        pages_nav_key(&g_nav, &g_cfg, BUTTONS_KEY_RIGHT, BUTTON_EVENT_REPEAT));
    TEST_ASSERT_EQUAL(PAGES_MAIN, g_nav.page);
}

/* ---- Main page setpoints ------------------------------------------------- */

static void test_off_mode_ignores_up_down(void)
{
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_EQUAL_INT16(680, g_cfg.heat_sp_f10);
}

static void test_heat_mode_moves_heat_setpoint(void)
{
    g_cfg.mode = (uint8_t)HVAC_MODE_HEAT; // 1 fits
    TEST_ASSERT_TRUE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_EQUAL_INT16(690, g_cfg.heat_sp_f10);
    TEST_ASSERT_TRUE(
        pages_nav_key(&g_nav, &g_cfg, BUTTONS_KEY_DOWN, BUTTON_EVENT_REPEAT));
    TEST_ASSERT_EQUAL_INT16(680, g_cfg.heat_sp_f10);
}

static void test_cool_mode_moves_cool_setpoint_in_celsius_steps(void)
{
    g_cfg.mode  = (uint8_t)HVAC_MODE_COOL;   // 2 fits
    g_cfg.units = (uint8_t)SETTINGS_UNITS_C; // 1 fits
    TEST_ASSERT_TRUE(press(BUTTONS_KEY_DOWN));
    TEST_ASSERT_EQUAL_INT16(751, g_cfg.cool_sp_f10);
}

static void test_heat_pushes_cool_up(void)
{
    g_cfg.mode        = (uint8_t)HVAC_MODE_HEAT; // 1 fits
    g_cfg.heat_sp_f10 = 730;
    TEST_ASSERT_TRUE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_EQUAL_INT16(740, g_cfg.heat_sp_f10);
    TEST_ASSERT_EQUAL_INT16(770, g_cfg.cool_sp_f10);
}

static void test_auto_center_selects_cool(void)
{
    g_cfg.mode = (uint8_t)HVAC_MODE_AUTO; // 3 fits
    TEST_ASSERT_FALSE(pages_nav_cool_active(&g_nav, &g_cfg));
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_TRUE(pages_nav_cool_active(&g_nav, &g_cfg));
    TEST_ASSERT_TRUE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_EQUAL_INT16(770, g_cfg.cool_sp_f10);
    TEST_ASSERT_EQUAL_INT16(680, g_cfg.heat_sp_f10);
}

static void test_setpoint_stops_at_limit(void)
{
    g_cfg.mode        = (uint8_t)HVAC_MODE_HEAT; // 1 fits
    g_cfg.heat_sp_f10 = HVAC_HEAT_SP_MAX_F10;
    g_cfg.cool_sp_f10 = HVAC_COOL_SP_MAX_F10;
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_EQUAL_INT16(HVAC_HEAT_SP_MAX_F10, g_cfg.heat_sp_f10);
}

/* ---- Mode and fan pages -------------------------------------------------- */

static void test_mode_page_applies_on_center(void)
{
    (void)press(BUTTONS_KEY_RIGHT);
    TEST_ASSERT_EQUAL(PAGES_MODE, g_nav.page);
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_OFF, g_nav.cursor);

    TEST_ASSERT_FALSE(press(BUTTONS_KEY_DOWN));
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_DOWN));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_OFF, g_cfg.mode); // not yet applied

    TEST_ASSERT_TRUE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_COOL, g_cfg.mode);
    TEST_ASSERT_EQUAL(PAGES_MAIN, g_nav.page);
}

static void test_mode_cursor_stops_at_ends(void)
{
    uint32_t press_idx = 0U;

    (void)press(BUTTONS_KEY_RIGHT);
    (void)press(BUTTONS_KEY_UP);
    TEST_ASSERT_EQUAL_UINT8(0U, g_nav.cursor);
    for (press_idx = 0U; press_idx < 10U; press_idx++)
    {
        (void)press(BUTTONS_KEY_DOWN);
    }
    TEST_ASSERT_EQUAL_UINT8(HVAC_MODE_EHEAT, g_nav.cursor);
}

static void test_fan_page(void)
{
    (void)press(BUTTONS_KEY_RIGHT);
    (void)press(BUTTONS_KEY_RIGHT);
    TEST_ASSERT_EQUAL(PAGES_FAN, g_nav.page);
    (void)press(BUTTONS_KEY_DOWN);
    TEST_ASSERT_TRUE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_ON, g_cfg.fan);

    (void)press(BUTTONS_KEY_RIGHT);
    (void)press(BUTTONS_KEY_RIGHT);
    TEST_ASSERT_EQUAL_UINT8(HVAC_FAN_ON, g_nav.cursor); // starts on current
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));       // no change
}

/* ---- Settings page ------------------------------------------------------- */

static void go_to_settings(void)
{
    (void)press(BUTTONS_KEY_LEFT);
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
}

// Settings page, cursor on `row`.
static void go_to_setting(uint8_t row)
{
    uint32_t press_idx = 0U;

    go_to_settings();
    for (press_idx = 0U; press_idx < row; press_idx++)
    {
        (void)press(BUTTONS_KEY_DOWN);
    }
    TEST_ASSERT_EQUAL_UINT8(row, g_nav.cursor);
}

// Settings page, cursor on "Factory reset", prompt open.
static void open_reset_prompt(void)
{
    uint32_t press_idx = 0U;

    go_to_settings();
    for (press_idx = 0U; press_idx < (uint32_t)PAGES_SET_RESET; press_idx++)
    {
        (void)press(BUTTONS_KEY_DOWN);
    }
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_RESET, g_nav.cursor);
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_TRUE(g_nav.b_confirm);
    TEST_ASSERT_FALSE(g_nav.b_confirm_yes); // starts on No
    TEST_ASSERT_FALSE(g_nav.b_editing);
}

static void test_settings_edit_cal(void)
{
    go_to_settings();
    (void)press(BUTTONS_KEY_DOWN); // cal
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_TRUE(g_nav.b_editing);
    TEST_ASSERT_TRUE(press(BUTTONS_KEY_DOWN));
    TEST_ASSERT_EQUAL_INT16(-1, g_cfg.cal_offset_f10);
    (void)press(BUTTONS_KEY_CENTER);
    TEST_ASSERT_FALSE(g_nav.b_editing);
}

static void test_settings_units_toggle_ignores_repeat(void)
{
    go_to_settings();
    (void)press(BUTTONS_KEY_CENTER);
    TEST_ASSERT_TRUE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_EQUAL_UINT8(SETTINGS_UNITS_C, g_cfg.units);
    TEST_ASSERT_FALSE(
        pages_nav_key(&g_nav, &g_cfg, BUTTONS_KEY_UP, BUTTON_EVENT_REPEAT));
    TEST_ASSERT_EQUAL_UINT8(SETTINGS_UNITS_C, g_cfg.units);
}

static void test_settings_values_clamp(void)
{
    uint32_t press_idx = 0U;

    go_to_settings();
    g_nav.cursor = (uint8_t)PAGES_SET_BRIGHT; // 5 fits
    (void)press(BUTTONS_KEY_CENTER);
    for (press_idx = 0U; press_idx < 40U; press_idx++)
    {
        (void)press(BUTTONS_KEY_DOWN);
    }
    TEST_ASSERT_EQUAL_UINT8(SETTINGS_BRIGHTNESS_MIN, g_cfg.brightness);
    for (press_idx = 0U; press_idx < 40U; press_idx++)
    {
        (void)press(BUTTONS_KEY_UP);
    }
    TEST_ASSERT_EQUAL_UINT8(255U, g_cfg.brightness);
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_sanitise(&g_cfg));
}

static void test_screen_off_steps_by_minutes_down_to_never(void)
{
    uint32_t press_idx = 0U;

    go_to_settings();
    for (press_idx = 0U; press_idx < (uint32_t)PAGES_SET_OFF; press_idx++)
    {
        (void)press(BUTTONS_KEY_DOWN); // walk the cursor to the item
    }
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_OFF, g_nav.cursor);
    (void)press(BUTTONS_KEY_CENTER);

    TEST_ASSERT_TRUE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_EQUAL_UINT16(660U, g_cfg.screen_off_s); // 10 -> 11 min
    for (press_idx = 0U; press_idx < 20U; press_idx++)
    {
        (void)press(BUTTONS_KEY_DOWN);
    }
    TEST_ASSERT_EQUAL_UINT16(0U, g_cfg.screen_off_s); // "never"
    for (press_idx = 0U; press_idx < 200U; press_idx++)
    {
        (void)press(BUTTONS_KEY_UP);
    }
    TEST_ASSERT_EQUAL_UINT16(SETTINGS_SCREEN_OFF_MAX, g_cfg.screen_off_s);
    TEST_ASSERT_EQUAL_UINT32(0U, settings_logic_sanitise(&g_cfg));
}

static void test_reset_prompt_defaults_to_no(void)
{
    open_reset_prompt();
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER)); // answers No
    TEST_ASSERT_FALSE(g_nav.b_confirm);
    TEST_ASSERT_FALSE(pages_nav_take_reset(&g_nav));
}

static void test_reset_needs_yes_then_centre(void)
{
    open_reset_prompt();
    (void)press(BUTTONS_KEY_DOWN); // to Yes
    TEST_ASSERT_TRUE(g_nav.b_confirm_yes);
    TEST_ASSERT_FALSE(pages_nav_take_reset(&g_nav)); // not until centre
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));    // no setting changes
    TEST_ASSERT_TRUE(pages_nav_take_reset(&g_nav));
    TEST_ASSERT_FALSE(pages_nav_take_reset(&g_nav)); // once only
    TEST_ASSERT_FALSE(g_nav.b_confirm);
}

static void test_reset_prompt_up_is_no_down_is_yes(void)
{
    open_reset_prompt();
    (void)press(BUTTONS_KEY_DOWN);
    (void)press(BUTTONS_KEY_DOWN); // stays on Yes, does not wrap to No
    TEST_ASSERT_TRUE(g_nav.b_confirm_yes);
    (void)press(BUTTONS_KEY_UP);
    (void)press(BUTTONS_KEY_UP); // stays on No
    TEST_ASSERT_FALSE(g_nav.b_confirm_yes);
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_FALSE(pages_nav_take_reset(&g_nav));
}

static void test_reset_prompt_ignores_repeats(void)
{
    open_reset_prompt();
    // Holding a key cannot walk onto Yes.
    TEST_ASSERT_FALSE(
        pages_nav_key(&g_nav, &g_cfg, BUTTONS_KEY_DOWN, BUTTON_EVENT_REPEAT));
    TEST_ASSERT_FALSE(g_nav.b_confirm_yes);
}

static void test_reset_prompt_cancelled_by_left_right_and_dim(void)
{
    open_reset_prompt();
    (void)press(BUTTONS_KEY_DOWN); // Yes, but not confirmed
    (void)press(BUTTONS_KEY_RIGHT);
    TEST_ASSERT_FALSE(g_nav.b_confirm);
    TEST_ASSERT_FALSE(pages_nav_take_reset(&g_nav));
    // Closing the prompt stays on the settings list, on the same row.
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_RESET, g_nav.cursor);
    (void)press(BUTTONS_KEY_CENTER); // reopens on No
    TEST_ASSERT_FALSE(g_nav.b_confirm_yes);
    (void)press(BUTTONS_KEY_LEFT);
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);

    pages_nav_reset(&g_nav); // back to main, to walk to settings again
    open_reset_prompt();
    (void)press(BUTTONS_KEY_DOWN);
    pages_nav_reset(&g_nav); // the panel dims
    TEST_ASSERT_FALSE(g_nav.b_confirm);
    TEST_ASSERT_FALSE(pages_nav_take_reset(&g_nav));
    TEST_ASSERT_FALSE(pages_nav_take_reset(NULL));
}

static void test_settings_list_wraps_on_a_press(void)
{
    go_to_settings();
    TEST_ASSERT_EQUAL_UINT8(0U, g_nav.cursor);
    (void)press(BUTTONS_KEY_UP); // top: wraps to the last row
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_COUNT - 1U, g_nav.cursor);
    (void)press(BUTTONS_KEY_DOWN); // bottom: wraps to the first row
    TEST_ASSERT_EQUAL_UINT8(0U, g_nav.cursor);
}

static void test_settings_list_held_key_stops_at_the_ends(void)
{
    go_to_settings();
    // Holding up at the top does not wrap.
    TEST_ASSERT_FALSE(
        pages_nav_key(&g_nav, &g_cfg, BUTTONS_KEY_UP, BUTTON_EVENT_REPEAT));
    TEST_ASSERT_EQUAL_UINT8(0U, g_nav.cursor);

    // Holding down walks to the bottom and stays there.
    pages_nav_reset(&g_nav);
    go_to_setting((uint8_t)(PAGES_SET_COUNT - 1U));
    TEST_ASSERT_FALSE(
        pages_nav_key(&g_nav, &g_cfg, BUTTONS_KEY_DOWN, BUTTON_EVENT_REPEAT));
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_COUNT - 1U, g_nav.cursor);
}

static void test_settings_wrap_does_not_apply_while_editing(void)
{
    go_to_settings();
    (void)press(BUTTONS_KEY_CENTER); // edit Units
    (void)press(BUTTONS_KEY_UP);     // changes the value, not the row
    TEST_ASSERT_EQUAL_UINT8(0U, g_nav.cursor);
}

static void test_info_opens_from_settings_and_returns(void)
{
    go_to_setting((uint8_t)PAGES_SET_INFO);
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_EQUAL(PAGES_INFO, g_nav.page);

    // Centre goes back to the same row.
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_CENTER));
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_INFO, g_nav.cursor);

    // So do left and right, instead of moving through the cycle.
    (void)press(BUTTONS_KEY_CENTER);
    (void)press(BUTTONS_KEY_RIGHT);
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_INFO, g_nav.cursor);
    (void)press(BUTTONS_KEY_CENTER);
    (void)press(BUTTONS_KEY_LEFT);
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
}

static void test_pair_opens_from_settings_and_changes_nothing(void)
{
    settings_t before = { 0 };

    go_to_setting((uint8_t)PAGES_SET_PAIR);
    before = g_cfg;
    (void)press(BUTTONS_KEY_CENTER);
    TEST_ASSERT_EQUAL(PAGES_PAIR, g_nav.page);
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_UP));
    TEST_ASSERT_FALSE(press(BUTTONS_KEY_DOWN));
    TEST_ASSERT_EQUAL(PAGES_PAIR, g_nav.page);
    (void)press(BUTTONS_KEY_LEFT);
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_PAIR, g_nav.cursor);
    TEST_ASSERT_EQUAL_MEMORY(&before, &g_cfg, sizeof(g_cfg));
}

static void test_pair_shown_at_boot_returns_to_settings(void)
{
    g_nav.page = PAGES_PAIR; // as app_ui does for an unpaired thermostat
    (void)press(BUTTONS_KEY_CENTER);
    TEST_ASSERT_EQUAL(PAGES_SETTINGS, g_nav.page);
    TEST_ASSERT_EQUAL_UINT8(PAGES_SET_PAIR, g_nav.cursor);
}

static void test_null_and_none(void)
{
    TEST_ASSERT_FALSE(
        pages_nav_key(NULL, &g_cfg, BUTTONS_KEY_UP, BUTTON_EVENT_PRESS));
    TEST_ASSERT_FALSE(
        pages_nav_key(&g_nav, NULL, BUTTONS_KEY_UP, BUTTON_EVENT_PRESS));
    TEST_ASSERT_FALSE(
        pages_nav_key(&g_nav, &g_cfg, BUTTONS_KEY_RIGHT, BUTTON_EVENT_NONE));
    TEST_ASSERT_EQUAL(PAGES_MAIN, g_nav.page);
}

/* ---- Formatting ---------------------------------------------------------- */

static void test_fmt_celsius(void)
{
    TEST_ASSERT_EQUAL_INT16(0, page_fmt_temp(320, true));
    TEST_ASSERT_EQUAL_INT16(200, page_fmt_temp(680, true));
    TEST_ASSERT_EQUAL_INT16(-400, page_fmt_temp(-400, true));
    TEST_ASSERT_EQUAL_INT16(222, page_fmt_temp(720, true)); // 22.22 C
    TEST_ASSERT_EQUAL_INT16(713, page_fmt_temp(713, false));
    TEST_ASSERT_EQUAL_INT16(5, page_fmt_delta(9, true));
    TEST_ASSERT_EQUAL_INT16(-28, page_fmt_delta(-50, true));
}

static void test_fmt_tenths(void)
{
    char buf[BUF_LEN] = { 0 };

    page_fmt_tenths(buf, BUF_LEN, 713, false);
    TEST_ASSERT_EQUAL_STRING("71.3", buf);
    page_fmt_tenths(buf, BUF_LEN, -5, false);
    TEST_ASSERT_EQUAL_STRING("-0.5", buf);
    page_fmt_tenths(buf, BUF_LEN, 15, true);
    TEST_ASSERT_EQUAL_STRING("+1.5", buf);
    page_fmt_tenths(buf, BUF_LEN, 0, true);
    TEST_ASSERT_EQUAL_STRING("0.0", buf);
    page_fmt_tenths(buf, 3U, 713, false);
    TEST_ASSERT_EQUAL_STRING("71", buf);
}

static void test_fmt_mmss(void)
{
    char buf[BUF_LEN] = { 0 };

    page_fmt_mmss(buf, BUF_LEN, 299U);
    TEST_ASSERT_EQUAL_STRING("4:59", buf);
    page_fmt_mmss(buf, BUF_LEN, 0U);
    TEST_ASSERT_EQUAL_STRING("0:00", buf);
    page_fmt_mmss(NULL, BUF_LEN, 1U); // must not crash
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_left_right_wrap);
    RUN_TEST(test_page_keys_do_not_repeat);
    RUN_TEST(test_off_mode_ignores_up_down);
    RUN_TEST(test_heat_mode_moves_heat_setpoint);
    RUN_TEST(test_cool_mode_moves_cool_setpoint_in_celsius_steps);
    RUN_TEST(test_heat_pushes_cool_up);
    RUN_TEST(test_auto_center_selects_cool);
    RUN_TEST(test_setpoint_stops_at_limit);
    RUN_TEST(test_mode_page_applies_on_center);
    RUN_TEST(test_mode_cursor_stops_at_ends);
    RUN_TEST(test_fan_page);
    RUN_TEST(test_settings_edit_cal);
    RUN_TEST(test_settings_units_toggle_ignores_repeat);
    RUN_TEST(test_settings_values_clamp);
    RUN_TEST(test_screen_off_steps_by_minutes_down_to_never);
    RUN_TEST(test_reset_prompt_defaults_to_no);
    RUN_TEST(test_reset_needs_yes_then_centre);
    RUN_TEST(test_reset_prompt_up_is_no_down_is_yes);
    RUN_TEST(test_reset_prompt_ignores_repeats);
    RUN_TEST(test_reset_prompt_cancelled_by_left_right_and_dim);
    RUN_TEST(test_settings_list_wraps_on_a_press);
    RUN_TEST(test_settings_list_held_key_stops_at_the_ends);
    RUN_TEST(test_settings_wrap_does_not_apply_while_editing);
    RUN_TEST(test_info_opens_from_settings_and_returns);
    RUN_TEST(test_pair_opens_from_settings_and_changes_nothing);
    RUN_TEST(test_pair_shown_at_boot_returns_to_settings);
    RUN_TEST(test_null_and_none);
    RUN_TEST(test_fmt_celsius);
    RUN_TEST(test_fmt_tenths);
    RUN_TEST(test_fmt_mmss);
    return UNITY_END();
}
