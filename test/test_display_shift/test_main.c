/**
 * @file  test_main.c
 * @brief Host tests for the OLED burn-in creep: the offsets over time, and
 *        that a frame moves exactly as far as asked, losing only what
 *        crosses the right and bottom edges.
 *
 * Runs with `pio test -e native`. The module under test is compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 */
#include "../../components/display/display_shift.c"

#include <stdbool.h>
#include <unity.h>

#define W     128U
#define PAGES 8U
#define H     (PAGES * 8U)

static uint8_t g_buf[PAGES * W] = { 0 };

static void set_px(uint32_t x, uint32_t y)
{
    // y % 8 is a bit index 0..7, so the shifted value fits a uint8_t.
    g_buf[((y / 8U) * W) + x] |= (uint8_t)(1U << (y % 8U));
}

static bool get_px(uint32_t x, uint32_t y)
{
    return (0U != (g_buf[((y / 8U) * W) + x] & (1U << (y % 8U))));
}

static uint32_t count_px(void)
{
    uint32_t count = 0U;
    uint32_t x     = 0U;
    uint32_t y     = 0U;

    for (y = 0U; y < H; y++)
    {
        for (x = 0U; x < W; x++)
        {
            count += get_px(x, y) ? 1U : 0U;
        }
    }

    return count;
}

void setUp(void)
{
    memset(g_buf, 0, sizeof(g_buf));
}

void tearDown(void)
{
}

static void test_offsets_start_at_zero_and_stay_in_range(void)
{
    uint8_t  dx = 99U;
    uint8_t  dy = 99U;
    uint64_t t  = 0U;

    display_shift_offsets(0U, &dx, &dy);
    TEST_ASSERT_EQUAL_UINT8(0U, dx);
    TEST_ASSERT_EQUAL_UINT8(0U, dy);

    // A day, minute by minute: never outside the margins pages leave.
    for (t = 0U; t < (24ULL * 3600ULL * 1000ULL); t += 60000ULL)
    {
        display_shift_offsets(t, &dx, &dy);
        TEST_ASSERT_TRUE(dx <= DISPLAY_SHIFT_X_MAX);
        TEST_ASSERT_TRUE(dy <= DISPLAY_SHIFT_Y_MAX);
    }
}

static void test_x_walks_out_and_back(void)
{
    uint8_t const expect[] = { 0U, 1U, 2U, 3U, 2U, 1U, 0U, 1U };
    uint8_t       dx       = 0U;
    uint32_t      step     = 0U;

    for (step = 0U; step < sizeof(expect); step++)
    {
        display_shift_offsets(
            (uint64_t)step * DISPLAY_SHIFT_X_PERIOD_S * 1000ULL, &dx, NULL);
        TEST_ASSERT_EQUAL_UINT8(expect[step], dx);
    }
}

static void test_y_is_slower(void)
{
    uint8_t dy = 0U;

    display_shift_offsets((DISPLAY_SHIFT_Y_PERIOD_S * 1000ULL) - 1U, NULL, &dy);
    TEST_ASSERT_EQUAL_UINT8(0U, dy);
    display_shift_offsets(DISPLAY_SHIFT_Y_PERIOD_S * 1000ULL, NULL, &dy);
    TEST_ASSERT_EQUAL_UINT8(1U, dy);
}

static void test_pixel_moves_right_and_down(void)
{
    set_px(10U, 6U); // near a page boundary, so dy crosses into page 1
    display_shift_buffer(g_buf, W, PAGES, 3U, 2U);
    TEST_ASSERT_TRUE(get_px(13U, 8U));
    TEST_ASSERT_EQUAL_UINT32(1U, count_px());
}

static void test_edges_drop_off(void)
{
    set_px(W - 1U, 0U); // off the right
    set_px(0U, H - 1U); // off the bottom
    set_px(50U, 30U);   // survives
    display_shift_buffer(g_buf, W, PAGES, 1U, 1U);
    TEST_ASSERT_EQUAL_UINT32(1U, count_px());
    TEST_ASSERT_TRUE(get_px(51U, 31U));
}

static void test_uncovered_strips_are_cleared(void)
{
    memset(g_buf, 0xFF, sizeof(g_buf)); // every pixel lit
    display_shift_buffer(g_buf, W, PAGES, 2U, 1U);
    TEST_ASSERT_FALSE(get_px(0U, 20U));
    TEST_ASSERT_FALSE(get_px(1U, 20U));
    TEST_ASSERT_FALSE(get_px(40U, 0U));
    TEST_ASSERT_TRUE(get_px(2U, 1U));
    TEST_ASSERT_EQUAL_UINT32((W - 2U) * (H - 1U), count_px());
}

static void test_zero_shift_and_bad_args_change_nothing(void)
{
    set_px(5U, 5U);
    display_shift_buffer(g_buf, W, PAGES, 0U, 0U);
    display_shift_buffer(g_buf, W, PAGES, (uint8_t)W, 1U); // dx too big
    display_shift_buffer(g_buf, W, 9U, 1U, 1U);            // too many pages
    display_shift_buffer(NULL, W, PAGES, 1U, 1U);
    TEST_ASSERT_TRUE(get_px(5U, 5U));
    TEST_ASSERT_EQUAL_UINT32(1U, count_px());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_offsets_start_at_zero_and_stay_in_range);
    RUN_TEST(test_x_walks_out_and_back);
    RUN_TEST(test_y_is_slower);
    RUN_TEST(test_pixel_moves_right_and_down);
    RUN_TEST(test_edges_drop_off);
    RUN_TEST(test_uncovered_strips_are_cleared);
    RUN_TEST(test_zero_shift_and_bad_args_change_nothing);
    return UNITY_END();
}
