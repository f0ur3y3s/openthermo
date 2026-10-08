/**
 * @file  test_main.c
 * @brief Host tests for one D-pad key: debounce, press and auto-repeat.
 *
 * Runs with `pio test -e native`. The module under test is compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 *
 * Each test feeds levels at the UI task's 20 ms tick and checks the complete
 * list of events that came out, so a stray extra event fails the test too.
 */
#include "../../components/buttons/button_fsm.c"

#include <unity.h>

#define POLL_MS    20U
#define START_MS   1000ULL
#define MAX_EVENTS 32U

static button_fsm_t   g_fsm                = { 0 };
static uint64_t       g_time               = 0U;
static button_event_t g_events[MAX_EVENTS] = { BUTTON_EVENT_NONE };
static uint32_t       g_event_count        = 0U;

// Holds a level for `polls` ticks, recording every event that comes out.
static void feed(bool b_pressed, uint32_t polls)
{
    button_event_t event    = BUTTON_EVENT_NONE;
    uint32_t       poll_idx = 0U;

    for (poll_idx = 0U; poll_idx < polls; poll_idx++)
    {
        g_time += POLL_MS;
        event = button_fsm_step(&g_fsm, b_pressed, g_time);
        if ((BUTTON_EVENT_NONE != event) && (g_event_count < MAX_EVENTS))
        {
            g_events[g_event_count] = event;
            g_event_count++;
        }
    }
}

static uint32_t count_of(button_event_t wanted)
{
    uint32_t count     = 0U;
    uint32_t event_idx = 0U;

    for (event_idx = 0U; event_idx < g_event_count; event_idx++)
    {
        if (wanted == g_events[event_idx])
        {
            count++;
        }
    }

    return count;
}

void setUp(void)
{
    button_fsm_reset(&g_fsm);
    g_time        = START_MS;
    g_event_count = 0U;
}

void tearDown(void)
{
}

static void test_idle_reports_nothing(void)
{
    feed(false, 100U);
    TEST_ASSERT_EQUAL_UINT32(0U, g_event_count);
}

static void test_single_poll_glitch_is_ignored(void)
{
    feed(true, 1U);
    feed(false, 10U);
    TEST_ASSERT_EQUAL_UINT32(0U, g_event_count);
}

static void test_short_press_is_one_press(void)
{
    feed(true, 5U);
    feed(false, 50U);
    TEST_ASSERT_EQUAL_UINT32(1U, g_event_count);
    TEST_ASSERT_EQUAL(BUTTON_EVENT_PRESS, g_events[0]);
}

static void test_release_glitch_does_not_repress(void)
{
    feed(true, 10U);
    feed(false, 1U);
    feed(true, 10U);
    TEST_ASSERT_EQUAL_UINT32(1U, g_event_count);
}

static void test_hold_repeats(void)
{
    // Press lands on poll 2 (40 ms). Repeats fall due 500 ms later, then
    // 150 ms after each one, on the first poll at or past the due time: a
    // 1.2 s hold gives repeats at 540, 700, 860, 1020 and 1180 ms.
    feed(true, 60U);
    TEST_ASSERT_EQUAL(BUTTON_EVENT_PRESS, g_events[0]);
    TEST_ASSERT_EQUAL_UINT32(1U, count_of(BUTTON_EVENT_PRESS));
    TEST_ASSERT_EQUAL_UINT32(5U, count_of(BUTTON_EVENT_REPEAT));

    feed(false, 50U);
    TEST_ASSERT_EQUAL_UINT32(6U, g_event_count);
}

static void test_no_repeat_before_delay(void)
{
    feed(true, 25U); // 500 ms: press at 40 ms, first repeat due at 540 ms
    TEST_ASSERT_EQUAL_UINT32(0U, count_of(BUTTON_EVENT_REPEAT));
}

static void test_null_is_harmless(void)
{
    TEST_ASSERT_EQUAL(BUTTON_EVENT_NONE, button_fsm_step(NULL, true, 0U));
    button_fsm_reset(NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_idle_reports_nothing);
    RUN_TEST(test_single_poll_glitch_is_ignored);
    RUN_TEST(test_short_press_is_one_press);
    RUN_TEST(test_release_glitch_does_not_repress);
    RUN_TEST(test_hold_repeats);
    RUN_TEST(test_no_repeat_before_delay);
    RUN_TEST(test_null_is_harmless);
    return UNITY_END();
}
