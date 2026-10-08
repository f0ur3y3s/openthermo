/**
 * @file  test_main.c
 * @brief Host tests for the bench status LED: state -> pattern, and pattern
 *        -> lit at a given time.
 *
 * Runs with `pio test -e native`. The module is bench-only, so the macro that
 * the bench build sets is set here too, before compiling it into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 */
#define OPENTHERMO_STATUS_LED 1
#include "../../components/status_led/status_led_pattern.c"

#include <unity.h>

#define FRAME_MS (STATUS_LED_SLOT_MS * STATUS_LED_SLOTS)

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_each_state(void)
{
    // Arguments: fault, waiting, Y1, G, O, W.
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_IDLE,
        status_led_pattern(false, false, false, false, false, false));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_FAN,
        status_led_pattern(false, false, false, true, false, false));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_WAITING,
        status_led_pattern(false, true, false, false, false, false));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_HEATING,
        status_led_pattern(false, false, true, true, false, false));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_COOLING,
        status_led_pattern(false, false, true, true, true, false));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_AUX,
        status_led_pattern(false, false, true, true, false, true));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_AUX,
        status_led_pattern(false, false, false, true, false, true)); // e-heat
}

static void test_priority(void)
{
    // A fault outranks everything; W outranks the compressor; O held while
    // idle (Y1 off) is not cooling.
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_FAULT,
        status_led_pattern(true, true, true, true, true, true));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_AUX,
        status_led_pattern(false, true, true, true, true, true));
    TEST_ASSERT_EQUAL_HEX16(
        STATUS_LED_IDLE,
        status_led_pattern(false, false, false, false, true, false));
}

static void test_patterns_are_distinct(void)
{
    uint16_t const all[] = { STATUS_LED_FAULT,   STATUS_LED_AUX,
                             STATUS_LED_COOLING, STATUS_LED_HEATING,
                             STATUS_LED_WAITING, STATUS_LED_FAN,
                             STATUS_LED_IDLE };
    uint32_t       outer = 0U;
    uint32_t       inner = 0U;

    for (outer = 0U; outer < (sizeof(all) / sizeof(all[0])); outer++)
    {
        for (inner = outer + 1U; inner < (sizeof(all) / sizeof(all[0]));
             inner++)
        {
            TEST_ASSERT_NOT_EQUAL(all[outer], all[inner]);
        }
    }
}

static void test_lit_follows_the_clock(void)
{
    // Idle: lit only in the first 125 ms of each 2 s frame.
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_IDLE, 0U));
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_IDLE, 124U));
    TEST_ASSERT_FALSE(status_led_lit(STATUS_LED_IDLE, 125U));
    TEST_ASSERT_FALSE(status_led_lit(STATUS_LED_IDLE, FRAME_MS - 1U));
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_IDLE, FRAME_MS));

    // Heating is always on; the fault blink is on for the first second.
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_HEATING, 1999U));
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_FAULT, 999U));
    TEST_ASSERT_FALSE(status_led_lit(STATUS_LED_FAULT, 1000U));

    // Fast blink alternates every slot.
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_AUX, 0U));
    TEST_ASSERT_FALSE(status_led_lit(STATUS_LED_AUX, 125U));
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_AUX, 250U));

    // Large uptimes (weeks) still land in the right slot.
    TEST_ASSERT_TRUE(status_led_lit(STATUS_LED_IDLE, 1000ULL * FRAME_MS));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_each_state);
    RUN_TEST(test_priority);
    RUN_TEST(test_patterns_are_distinct);
    RUN_TEST(test_lit_follows_the_clock);
    return UNITY_END();
}
