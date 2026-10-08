/**
 * @file  test_main.c
 * @brief Host tests for the relay interlock: each hard safety rule it
 *        re-checks, fed requests that hvac_logic would never make.
 *
 * Runs with `pio test -e native`. The module under test is compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 */
#include "../../components/relays/relays_guard.c"

#include <unity.h>

#define BOOT_MS  5000ULL
#define AFTER_MS (BOOT_MS + RELAYS_GUARD_MIN_OFF_MS)

static relays_guard_t   g_guard = { 0 };
static relays_outputs_t g_out   = { 0 };

static bool request(bool b_y1, bool b_g, bool b_o, bool b_w, uint64_t now_ms)
{
    relays_outputs_t req = { 0 };

    req.b_y1 = b_y1;
    req.b_g  = b_g;
    req.b_o  = b_o;
    req.b_w  = b_w;

    return relays_guard_step(&g_guard, &req, now_ms, &g_out);
}

static void expect_out(bool b_y1, bool b_g, bool b_o, bool b_w)
{
    TEST_ASSERT_EQUAL_MESSAGE(b_y1, g_out.b_y1, "Y1");
    TEST_ASSERT_EQUAL_MESSAGE(b_g, g_out.b_g, "G");
    TEST_ASSERT_EQUAL_MESSAGE(b_o, g_out.b_o, "O");
    TEST_ASSERT_EQUAL_MESSAGE(b_w, g_out.b_w, "W");
}

void setUp(void)
{
    relays_guard_init(&g_guard, BOOT_MS);
    memset(&g_out, 0, sizeof(g_out));
}

void tearDown(void)
{
}

static void test_y1_refused_until_min_off_after_boot(void)
{
    TEST_ASSERT_FALSE(request(true, true, false, false, AFTER_MS - 1U));
    expect_out(false, true, false, false);

    TEST_ASSERT_TRUE(request(true, true, false, false, AFTER_MS));
    expect_out(true, true, false, false);
}

static void test_y1_refused_until_min_off_after_a_run(void)
{
    (void)request(true, true, false, false, AFTER_MS);
    (void)request(false, false, false, false, AFTER_MS + 1000U);

    TEST_ASSERT_FALSE(request(true, true, false, false,
                              AFTER_MS + 1000U + RELAYS_GUARD_MIN_OFF_MS - 1U));
    TEST_ASSERT_FALSE(g_out.b_y1);

    TEST_ASSERT_TRUE(request(true, true, false, false,
                             AFTER_MS + 1000U + RELAYS_GUARD_MIN_OFF_MS));
    TEST_ASSERT_TRUE(g_out.b_y1);
}

static void test_turning_off_is_always_allowed(void)
{
    (void)request(true, true, true, false, AFTER_MS);
    TEST_ASSERT_TRUE(request(false, false, true, false, AFTER_MS + 1U));
    expect_out(false, false, true, false);
}

static void test_all_off_drops_o_and_rearms(void)
{
    uint64_t const drop_ms = AFTER_MS + (10U * 60000U); // cooled 10 min

    (void)request(true, true, true, false, AFTER_MS);
    // Sensor fault: everything off, O included, at once.
    TEST_ASSERT_TRUE(request(false, false, false, false, drop_ms));
    expect_out(false, false, false, false);

    // Neither the valve nor the compressor may move for a full minimum-off
    // time from the drop.
    TEST_ASSERT_FALSE(request(false, false, true, false,
                              drop_ms + RELAYS_GUARD_MIN_OFF_MS - 1U));
    TEST_ASSERT_FALSE(g_out.b_o);
    TEST_ASSERT_FALSE(request(true, true, false, false,
                              drop_ms + RELAYS_GUARD_MIN_OFF_MS - 1U));
    TEST_ASSERT_FALSE(g_out.b_y1);

    TEST_ASSERT_TRUE(
        request(true, true, true, false, drop_ms + RELAYS_GUARD_MIN_OFF_MS));
    expect_out(true, true, true, false);
}

static void test_all_off_after_y1_stopped_still_rearms(void)
{
    uint64_t const stop_ms = AFTER_MS + 1000U;
    uint64_t const drop_ms = stop_ms + (2U * 60000U); // 2 min into min-off

    (void)request(true, true, true, false, AFTER_MS);
    (void)request(false, true, true, false, stop_ms); // idle, O held
    TEST_ASSERT_TRUE(request(false, false, false, false, drop_ms));
    expect_out(false, false, false, false);

    // The 5 min count starts again at the drop, not at the Y1 stop.
    TEST_ASSERT_FALSE(
        request(true, true, false, false, stop_ms + RELAYS_GUARD_MIN_OFF_MS));
    TEST_ASSERT_FALSE(request(true, true, false, false,
                              drop_ms + RELAYS_GUARD_MIN_OFF_MS - 1U));
    TEST_ASSERT_TRUE(
        request(true, true, false, false, drop_ms + RELAYS_GUARD_MIN_OFF_MS));
}

static void test_partial_off_still_holds_o(void)
{
    (void)request(true, true, true, false, AFTER_MS);
    // Only an all-off request may drop O early: with G still asked for, the
    // valve is held as before.
    TEST_ASSERT_FALSE(request(false, true, false, false, AFTER_MS + 1000U));
    expect_out(false, true, true, false);
}

static void test_o_change_refused_while_y1_runs(void)
{
    (void)request(true, true, false, false, AFTER_MS);

    // A buggy caller flips O under a running compressor: O is held and Y1
    // is stopped rather than left running on the wrong valve position.
    TEST_ASSERT_FALSE(request(true, true, true, false, AFTER_MS + 1000U));
    expect_out(false, true, false, false);
}

static void test_o_change_refused_until_min_off(void)
{
    (void)request(true, true, false, false, AFTER_MS);
    (void)request(false, false, false, false, AFTER_MS + 1000U);

    TEST_ASSERT_FALSE(request(false, false, true, false, AFTER_MS + 2000U));
    TEST_ASSERT_FALSE(g_out.b_o);

    TEST_ASSERT_TRUE(request(true, true, true, false,
                             AFTER_MS + 1000U + RELAYS_GUARD_MIN_OFF_MS));
    expect_out(true, true, true, false);
}

static void test_o_change_at_boot_waits(void)
{
    TEST_ASSERT_FALSE(request(false, false, true, false, BOOT_MS + 1U));
    TEST_ASSERT_FALSE(g_out.b_o);
}

static void test_aux_refused_while_cooling(void)
{
    TEST_ASSERT_FALSE(request(true, true, true, true, AFTER_MS));
    expect_out(true, true, true, false);
}

static void test_eheat_with_o_held_is_allowed(void)
{
    // W with O but no Y1: the valve position does not matter without the
    // compressor.
    (void)request(true, true, true, false, AFTER_MS);
    (void)request(false, true, true, false, AFTER_MS + 1000U);
    TEST_ASSERT_TRUE(request(false, true, true, true, AFTER_MS + 2000U));
}

static void test_g_forced_with_y1_or_w(void)
{
    TEST_ASSERT_FALSE(request(false, false, false, true, BOOT_MS + 1U));
    expect_out(false, true, false, true);

    TEST_ASSERT_FALSE(request(true, false, false, false, AFTER_MS));
    expect_out(true, true, false, false);
}

static void test_clock_going_backwards_refuses_y1(void)
{
    TEST_ASSERT_FALSE(request(true, true, false, false, BOOT_MS - 1U));
    TEST_ASSERT_FALSE(g_out.b_y1);
}

static void test_null_arguments(void)
{
    g_out.b_y1 = true;
    TEST_ASSERT_FALSE(relays_guard_step(NULL, &g_out, AFTER_MS, &g_out));
    expect_out(false, false, false, false);

    g_out.b_y1 = true;
    TEST_ASSERT_FALSE(relays_guard_step(&g_guard, NULL, AFTER_MS, &g_out));
    expect_out(false, false, false, false);

    TEST_ASSERT_FALSE(relays_guard_step(&g_guard, &g_out, AFTER_MS, NULL));
    relays_guard_init(NULL, 0U); // must not crash
}

/* ---- State corruption --------------------------------------------------- */

static void test_corrupted_timer_forces_all_off_and_rearms(void)
{
    (void)request(true, true, false, false, AFTER_MS);
    g_guard.y1_off_since_ms ^= 1ULL << 40U; // one flipped bit

    TEST_ASSERT_FALSE(request(true, true, false, false, AFTER_MS + 1000U));
    expect_out(false, false, false, false);
    TEST_ASSERT_EQUAL_UINT32(1U, g_guard.resyncs);

    // The minimum-off time restarts from the moment it was caught.
    TEST_ASSERT_FALSE(request(true, true, false, false,
                              AFTER_MS + 1000U + RELAYS_GUARD_MIN_OFF_MS - 1U));
    TEST_ASSERT_TRUE(request(true, true, false, false,
                             AFTER_MS + 1000U + RELAYS_GUARD_MIN_OFF_MS));
}

static void test_corrupted_outputs_are_caught(void)
{
    (void)request(true, true, false, false, AFTER_MS);
    g_guard.applied.b_y1 = false; // the guard "forgets" Y1 is running

    TEST_ASSERT_FALSE(request(true, true, false, false, AFTER_MS + 1000U));
    expect_out(false, false, false, false);
    TEST_ASSERT_EQUAL_UINT32(1U, g_guard.resyncs);
}

static void test_corrupted_check_copy_is_caught(void)
{
    g_guard.applied_inv ^= 0x04U;
    TEST_ASSERT_FALSE(request(false, true, false, false, AFTER_MS));
    expect_out(false, false, false, false);
    TEST_ASSERT_EQUAL_UINT32(1U, g_guard.resyncs);
}

/* ---- Randomised: hostile requests, random timing, random corruption ------ */

#define FUZZ_SEEDS 16U
#define FUZZ_STEPS 250000U

static uint32_t g_rng = 1U;

// xorshift32: deterministic, so a failure reproduces from its seed.
static uint32_t rng_next(void)
{
    g_rng ^= g_rng << 13U;
    g_rng ^= g_rng >> 17U;
    g_rng ^= g_rng << 5U;
    return g_rng;
}

// Usually the control task's 1 s period; sometimes much longer or shorter,
// so every timer edge gets hit from both sides.
static uint64_t fuzz_dt(void)
{
    uint32_t pick = rng_next() % 100U;
    uint64_t dt   = 1000U;

    if (pick < 10U)
    {
        dt = rng_next() % 2000U;
    }
    else if (pick < 15U)
    {
        dt = rng_next() % (2U * RELAYS_GUARD_MIN_OFF_MS);
    }
    else
    {
        // The 1 s default stands.
    }

    return dt;
}

// Flips one thing in the guard's state, as a stray write might.
static void fuzz_corrupt(void)
{
    switch (rng_next() % 5U)
    {
        case 0U:
            g_guard.y1_off_since_ms ^= 1ULL << (rng_next() % 64U);
            break;
        case 1U:
            g_guard.off_since_inv ^= 1ULL << (rng_next() % 64U);
            break;
        case 2U:
            // Bit index 0..7 of a uint8_t, so the cast drops nothing.
            g_guard.applied_inv ^= (uint8_t)(1U << (rng_next() % 8U));
            break;
        case 3U:
            g_guard.applied.b_y1 = !g_guard.applied.b_y1;
            break;
        default:
            g_guard.applied.b_o = !g_guard.applied.b_o;
            break;
    }
}

// Runs one seed, checking the hard rules from the outputs alone.
static void fuzz_one_seed(uint32_t seed)
{
    relays_outputs_t req      = { 0 };
    relays_outputs_t prev     = { 0 };
    uint64_t         now      = BOOT_MS;
    uint64_t         y1_off   = BOOT_MS; // boot counts as Y1 going off
    uint32_t         step_idx = 0U;
    uint32_t         starts   = 0U;

    g_rng = seed;
    relays_guard_init(&g_guard, BOOT_MS);

    for (step_idx = 0U; step_idx < FUZZ_STEPS; step_idx++)
    {
        now += fuzz_dt();
        req.b_y1 = (0U != (rng_next() & 1U));
        req.b_g  = (0U != (rng_next() & 1U));
        req.b_o  = ((rng_next() % 8U) == 0U) ? !prev.b_o : prev.b_o;
        req.b_w  = ((rng_next() % 4U) == 0U);
        if ((rng_next() % 20000U) == 0U)
        {
            fuzz_corrupt();
        }

        (void)relays_guard_step(&g_guard, &req, now, &g_out);

        if (g_out.b_y1 && !prev.b_y1)
        {
            TEST_ASSERT_TRUE((now - y1_off) >= RELAYS_GUARD_MIN_OFF_MS);
            starts++;
        }
        if (!g_out.b_y1 && prev.b_y1)
        {
            y1_off = now;
        }
        if ((g_out.b_o != prev.b_o) &&
            !(!prev.b_y1 && ((now - y1_off) >= RELAYS_GUARD_MIN_OFF_MS)))
        {
            // Not the normal valve rule, so it must be an all-off drop (an
            // all-off request, or a corruption resync), which re-arms the
            // minimum-off time: the Y1 check above then holds it to that.
            TEST_ASSERT_FALSE(g_out.b_o);
            TEST_ASSERT_FALSE(g_out.b_y1 || g_out.b_g || g_out.b_w);
            y1_off = now;
        }
        TEST_ASSERT_FALSE((g_out.b_y1 || g_out.b_w) && !g_out.b_g);
        TEST_ASSERT_FALSE(g_out.b_w && g_out.b_y1 && g_out.b_o);
        // Off is never refused.
        TEST_ASSERT_FALSE(!req.b_y1 && g_out.b_y1);
        TEST_ASSERT_FALSE(!req.b_w && g_out.b_w);

        prev = g_out;
    }

    TEST_ASSERT_TRUE(starts > 0U); // the run did exercise the compressor
}

static void test_randomised_requests_never_break_the_rules(void)
{
    uint32_t seed = 0U;

    for (seed = 1U; seed <= FUZZ_SEEDS; seed++)
    {
        // Any non-zero seed works for xorshift32.
        fuzz_one_seed(seed * 2654435761U);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_y1_refused_until_min_off_after_boot);
    RUN_TEST(test_y1_refused_until_min_off_after_a_run);
    RUN_TEST(test_turning_off_is_always_allowed);
    RUN_TEST(test_all_off_drops_o_and_rearms);
    RUN_TEST(test_all_off_after_y1_stopped_still_rearms);
    RUN_TEST(test_partial_off_still_holds_o);
    RUN_TEST(test_o_change_refused_while_y1_runs);
    RUN_TEST(test_o_change_refused_until_min_off);
    RUN_TEST(test_o_change_at_boot_waits);
    RUN_TEST(test_aux_refused_while_cooling);
    RUN_TEST(test_eheat_with_o_held_is_allowed);
    RUN_TEST(test_g_forced_with_y1_or_w);
    RUN_TEST(test_clock_going_backwards_refuses_y1);
    RUN_TEST(test_null_arguments);
    RUN_TEST(test_corrupted_timer_forces_all_off_and_rearms);
    RUN_TEST(test_corrupted_outputs_are_caught);
    RUN_TEST(test_corrupted_check_copy_is_caught);
    RUN_TEST(test_randomised_requests_never_break_the_rules);
    return UNITY_END();
}
