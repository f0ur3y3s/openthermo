/**
 * @file  test_main.c
 * @brief Host tests for the whole output path as the firmware wires it:
 *        hvac_logic -> relays_guard, with the guard on its own clock, the
 *        logic fed back what was really driven, and random resets.
 *
 * Runs with `pio test -e native`. Both modules are compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 *
 * The hard rules are checked from the driven outputs alone, against real
 * elapsed time, across resets:
 *   - Y1 starts only after it has been off for the minimum-off time, and a
 *     reset counts as it going off (the pins drop low);
 *   - O changes only with Y1 off for the minimum-off time, or at a reset,
 *     or when a fault drops everything, which restarts the minimum-off time;
 *   - every output is off on every step without a valid temperature;
 *   - W never runs with Y1 + O; Y1 and W never run without G;
 *   - Y1 never runs in emergency heat;
 *   - a run lasts the minimum run unless a reset, a fault, or a change of
 *     mode to Off or emergency heat ended it. Any other change (Heat <->
 *     Cool, Auto, setpoints, fan) must not shorten it.
 * It also checks the system is live: the compressor does run, so a guard
 * that refused everything could not pass.
 */
#include "../../components/hvac_logic/hvac_logic.c"
#include "../../components/relays/relays_guard.c"

#include <unity.h>

#define SEEDS      12U
#define STEPS      400000U // about 4.6 days of 1 s steps per seed
#define PERIOD_MS  1000ULL
#define JITTER_MS  50U    // the guard reads its clock later in the cycle
#define RESET_ODDS 20000U // about one reset every 5.5 hours
#define FAULT_ODDS 15000U
#define FAULT_MAX  400U // a fault lasts 1..400 s (a 2 min loss is 120)
#define MODE_ODDS  3000U

typedef struct
{
    hvac_state_t     hvac;
    relays_guard_t   guard;
    relays_outputs_t driven;       // what the pins are doing now
    uint64_t         y1_off_ms;    // last time Y1 went off (resets count)
    uint64_t         y1_on_ms;     // when the current run started
    bool             b_run_waived; // the current run may end early
    uint32_t         fault_left;   // steps left in the current fault
    uint32_t         starts;
} chain_t;

static uint32_t g_rng = 1U;

static uint32_t rng_next(void)
{
    g_rng ^= g_rng << 13U;
    g_rng ^= g_rng >> 17U;
    g_rng ^= g_rng << 5U;
    return g_rng;
}

void setUp(void)
{
}

void tearDown(void)
{
}

// A reset: both modules start over and every pin drops low.
static void chain_reset(chain_t * p_chain, uint64_t now_ms)
{
    if (p_chain->driven.b_y1)
    {
        p_chain->y1_off_ms = now_ms;
    }
    hvac_init(&p_chain->hvac, now_ms);
    relays_guard_init(&p_chain->guard, now_ms);
    memset(&p_chain->driven, 0, sizeof(p_chain->driven));
}

// Checks every hard rule on one change of the driven outputs.
static void chain_check(chain_t * p_chain, relays_outputs_t const * p_new,
                        hvac_input_t const * p_in, uint64_t now_ms)
{
    relays_outputs_t const * p_old = &p_chain->driven;

    if (p_new->b_y1 && !p_old->b_y1)
    {
        TEST_ASSERT_TRUE((now_ms - p_chain->y1_off_ms) >= HVAC_MIN_OFF_MS);
        p_chain->y1_on_ms     = now_ms;
        p_chain->b_run_waived = false;
        p_chain->starts++;
    }
    if (!p_new->b_y1 && p_old->b_y1)
    {
        if (!p_chain->b_run_waived)
        {
            TEST_ASSERT_TRUE((now_ms - p_chain->y1_on_ms) >= HVAC_MIN_RUN_MS);
        }
        p_chain->y1_off_ms = now_ms;
    }
    if (p_new->b_o != p_old->b_o)
    {
        if (p_old->b_y1 || ((now_ms - p_chain->y1_off_ms) < HVAC_MIN_OFF_MS))
        {
            // Not allowed by the valve rule, so it must be a fault's all-off
            // drop, which restarts the minimum-off time (the Y1 start rule
            // above then holds the next start to that).
            TEST_ASSERT_FALSE(p_in->b_temp_valid);
            TEST_ASSERT_FALSE(p_new->b_o);
            p_chain->y1_off_ms = now_ms;
        }
    }
    if (!p_in->b_temp_valid)
    {
        TEST_ASSERT_FALSE(p_new->b_y1 || p_new->b_g || p_new->b_o ||
                          p_new->b_w);
    }
    TEST_ASSERT_FALSE((p_new->b_y1 || p_new->b_w) && !p_new->b_g);
    TEST_ASSERT_FALSE(p_new->b_w && p_new->b_y1 && p_new->b_o);
    if (HVAC_MODE_EHEAT == p_in->mode)
    {
        TEST_ASSERT_FALSE(p_new->b_y1);
    }
}

// One control-task cycle: logic, then guard on its own (later) clock.
static void chain_cycle(chain_t * p_chain, hvac_input_t * p_in)
{
    hvac_status_t    status   = { 0 };
    relays_outputs_t req      = { 0 };
    relays_outputs_t out      = { 0 };
    uint64_t         guard_ms = p_in->now_ms + (rng_next() % JITTER_MS);

    p_in->b_applied_valid = true;
    p_in->applied.b_y1    = p_chain->driven.b_y1;
    p_in->applied.b_g     = p_chain->driven.b_g;
    p_in->applied.b_o     = p_chain->driven.b_o;
    p_in->applied.b_w     = p_chain->driven.b_w;
    hvac_step(&p_chain->hvac, p_in, &status);

    req.b_y1 = status.out.b_y1;
    req.b_g  = status.out.b_g;
    req.b_o  = status.out.b_o;
    req.b_w  = status.out.b_w;
    (void)relays_guard_step(&p_chain->guard, &req, guard_ms, &out);

    chain_check(p_chain, &out, p_in, guard_ms);
    p_chain->driven = out;
}

// Random but plausible inputs: the room drifts, setpoints and modes move.
static void chain_inputs(hvac_input_t * p_in, chain_t * p_chain)
{
    int32_t     temp = p_in->temp_f10;
    hvac_mode_t mode = HVAC_MODE_OFF;

    temp += (int32_t)(rng_next() % 5U) - 2; // -0.2 .. +0.2 F per second
    if (p_chain->driven.b_y1 && !p_chain->driven.b_o)
    {
        temp += 1;
    }
    if (p_chain->driven.b_y1 && p_chain->driven.b_o)
    {
        temp -= 1;
    }
    if (temp < 550)
    {
        temp = 550;
    }
    if (temp > 900)
    {
        temp = 900;
    }
    p_in->temp_f10 = (int16_t)temp; // clamped to 550..900 just above

    if ((0U == p_chain->fault_left) && ((rng_next() % FAULT_ODDS) == 0U))
    {
        p_chain->fault_left = 1U + (rng_next() % FAULT_MAX);
    }
    p_in->b_temp_valid = (0U == p_chain->fault_left);
    if (!p_in->b_temp_valid)
    {
        p_chain->fault_left--;
        p_chain->b_run_waived = true; // a fault ends a run early
    }
    if ((rng_next() % MODE_ODDS) == 0U)
    {
        // 0..4 are the five modes.
        mode = (hvac_mode_t)(rng_next() % (uint32_t)HVAC_MODE_COUNT);
        if ((mode != p_in->mode) &&
            ((HVAC_MODE_OFF == mode) || (HVAC_MODE_EHEAT == mode)))
        {
            p_chain->b_run_waived = true; // Off and e-heat end a run early
        }
        p_in->mode        = mode;
        p_in->fan         = (hvac_fan_t)(rng_next() % (uint32_t)HVAC_FAN_COUNT);
        p_in->heat_sp_f10 = (int16_t)(640 + (int32_t)(rng_next() % 80U));
        p_in->cool_sp_f10 =
            (int16_t)(p_in->heat_sp_f10 + 30 + (int32_t)(rng_next() % 60U));
    }
}

static void chain_one_seed(uint32_t seed)
{
    chain_t      chain    = { 0 };
    hvac_input_t in       = { 0 };
    uint32_t     step_idx = 0U;
    uint64_t     now      = 1000U;

    g_rng           = seed;
    in.mode         = HVAC_MODE_AUTO;
    in.fan          = HVAC_FAN_AUTO;
    in.heat_sp_f10  = 680;
    in.cool_sp_f10  = 760;
    in.fan_purge_s  = 60U;
    in.temp_f10     = 700;
    chain.y1_off_ms = now;
    chain_reset(&chain, now);

    for (step_idx = 0U; step_idx < STEPS; step_idx++)
    {
        now += PERIOD_MS;
        in.now_ms = now;
        if ((rng_next() % RESET_ODDS) == 0U)
        {
            chain.b_run_waived = true;
            chain_reset(&chain, now);
        }
        chain_inputs(&in, &chain);
        chain_cycle(&chain, &in);
    }

    TEST_ASSERT_TRUE(chain.starts > 10U); // live, not just safe
}

static void test_chain_holds_the_rules_across_resets_and_skew(void)
{
    uint32_t seed = 0U;

    for (seed = 1U; seed <= SEEDS; seed++)
    {
        // Any non-zero seed works for xorshift32.
        chain_one_seed(seed * 2246822519U);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_chain_holds_the_rules_across_resets_and_skew);
    return UNITY_END();
}
