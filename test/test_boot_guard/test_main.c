/**
 * @file  test_main.c
 * @brief Host tests for the crash-loop breaker.
 *
 * Runs with `pio test -e native`. The module under test is compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 */
#include "../../components/app/boot_guard.c"

#include <string.h>
#include <unity.h>

static boot_guard_rec_t g_rec = { 0 };

void setUp(void)
{
    // Power-on: RTC memory holds whatever it holds; here, garbage.
    memset(&g_rec, 0xA5, sizeof(g_rec));
}

void tearDown(void)
{
}

static void test_power_on_garbage_reads_as_no_crashes(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, boot_guard_on_boot(&g_rec, false));
    TEST_ASSERT_EQUAL_UINT32(BOOT_GUARD_MAGIC, g_rec.magic);
}

static void test_three_crashes_in_a_row_trip(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, boot_guard_on_boot(&g_rec, false)); // power-on
    TEST_ASSERT_FALSE(boot_guard_tripped(boot_guard_on_boot(&g_rec, true)));
    TEST_ASSERT_FALSE(boot_guard_tripped(boot_guard_on_boot(&g_rec, true)));
    TEST_ASSERT_TRUE(boot_guard_tripped(boot_guard_on_boot(&g_rec, true)));
    TEST_ASSERT_TRUE(boot_guard_tripped(boot_guard_on_boot(&g_rec, true)));
}

static void test_a_clean_boot_resets_the_count(void)
{
    (void)boot_guard_on_boot(&g_rec, true);
    (void)boot_guard_on_boot(&g_rec, true);
    TEST_ASSERT_EQUAL_UINT32(0U, boot_guard_on_boot(&g_rec, false));
    TEST_ASSERT_EQUAL_UINT32(1U, boot_guard_on_boot(&g_rec, true));
}

static void test_running_long_enough_clears(void)
{
    (void)boot_guard_on_boot(&g_rec, true);
    (void)boot_guard_on_boot(&g_rec, true);
    boot_guard_clear(&g_rec);
    TEST_ASSERT_EQUAL_UINT32(1U, boot_guard_on_boot(&g_rec, true));
}

static void test_a_corrupt_record_counts_as_zero(void)
{
    (void)boot_guard_on_boot(&g_rec, true);
    (void)boot_guard_on_boot(&g_rec, true);
    g_rec.crashes = 40U; // check copy no longer matches
    TEST_ASSERT_EQUAL_UINT32(1U, boot_guard_on_boot(&g_rec, true));
}

static void test_count_saturates(void)
{
    uint32_t idx = 0U;

    for (idx = 0U; idx < 300U; idx++)
    {
        (void)boot_guard_on_boot(&g_rec, true);
    }
    TEST_ASSERT_EQUAL_UINT32(BOOT_GUARD_MAX, g_rec.crashes);
    TEST_ASSERT_TRUE(boot_guard_tripped(boot_guard_on_boot(&g_rec, true)));
}

static void test_null_is_harmless(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, boot_guard_on_boot(NULL, true));
    boot_guard_clear(NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_on_garbage_reads_as_no_crashes);
    RUN_TEST(test_three_crashes_in_a_row_trip);
    RUN_TEST(test_a_clean_boot_resets_the_count);
    RUN_TEST(test_running_long_enough_clears);
    RUN_TEST(test_a_corrupt_record_counts_as_zero);
    RUN_TEST(test_count_saturates);
    RUN_TEST(test_null_is_harmless);
    return UNITY_END();
}
