/**
 * @file  test_main.c
 * @brief Host tests for the SHT40 decoding, the 2 min fault rule and the
 *        self-heating correction.
 *
 * Runs with `pio test -e native`. The module under test is compiled into this
 * translation unit (see docs/CODING_STANDARD.md, deviation D4).
 */
#include "../../components/sensor/sensor_logic.c"

#include <unity.h>

#define START_MS 2000ULL

static sensor_logic_t g_logic = { 0 };

// Builds a frame with correct CRCs from two raw words.
static void make_frame(uint8_t * p_frame, uint16_t raw_t, uint16_t raw_rh)
{
    // Each byte is one half of a 16-bit word, so the casts drop nothing.
    p_frame[0] = (uint8_t)(raw_t >> 8U);
    p_frame[1] = (uint8_t)(raw_t & 0xFFU);
    p_frame[2] = sensor_crc8(&p_frame[0], 2U);
    p_frame[3] = (uint8_t)(raw_rh >> 8U);
    p_frame[4] = (uint8_t)(raw_rh & 0xFFU);
    p_frame[5] = sensor_crc8(&p_frame[3], 2U);
}

void setUp(void)
{
    sensor_logic_init(&g_logic, START_MS);
}

void tearDown(void)
{
}

static void test_crc_matches_datasheet_example(void)
{
    uint8_t const data[2] = { 0xBEU, 0xEFU };

    TEST_ASSERT_EQUAL_HEX8(0x92U, sensor_crc8(data, 2U));
}

static void test_decode_known_values(void)
{
    uint8_t frame[SENSOR_FRAME_LEN] = { 0 };
    int16_t temp                    = 0;
    int16_t rh                      = 0;

    make_frame(frame, 0x6666U, 0x8000U); // 25.0 C, 56.5 %
    TEST_ASSERT_TRUE(sensor_decode(frame, &temp, &rh));
    TEST_ASSERT_EQUAL_INT16(770, temp);
    TEST_ASSERT_EQUAL_INT16(565, rh);
}

static void test_decode_extremes(void)
{
    uint8_t frame[SENSOR_FRAME_LEN] = { 0 };
    int16_t temp                    = 0;
    int16_t rh                      = 0;

    // The ends of the sensor's range decode to -49 F and 266 F with good
    // CRCs: implausible for a room, so they fail like a bad CRC.
    temp = 123;
    make_frame(frame, 0x0000U, 0x0000U);
    TEST_ASSERT_FALSE(sensor_decode(frame, &temp, &rh));
    make_frame(frame, 0xFFFFU, 0xFFFFU);
    TEST_ASSERT_FALSE(sensor_decode(frame, &temp, &rh));
    TEST_ASSERT_EQUAL_INT16(123, temp);

    // Humidity still clamps: 25 C with raw RH 0 (-6 %) and 0xFFFF (119 %).
    make_frame(frame, 0x6666U, 0x0000U);
    TEST_ASSERT_TRUE(sensor_decode(frame, &temp, &rh));
    TEST_ASSERT_EQUAL_INT16(0, rh);
    make_frame(frame, 0x6666U, 0xFFFFU);
    TEST_ASSERT_TRUE(sensor_decode(frame, &temp, &rh));
    TEST_ASSERT_EQUAL_INT16(1000, rh);
}

static void test_decode_rejects_bad_crc(void)
{
    uint8_t frame[SENSOR_FRAME_LEN] = { 0 };
    int16_t temp                    = 123;
    int16_t rh                      = 456;

    make_frame(frame, 0x6666U, 0x8000U);
    frame[2] ^= 0x01U;
    TEST_ASSERT_FALSE(sensor_decode(frame, &temp, &rh));
    TEST_ASSERT_EQUAL_INT16(123, temp);
    TEST_ASSERT_EQUAL_INT16(456, rh);

    make_frame(frame, 0x6666U, 0x8000U);
    frame[4] ^= 0x80U;
    TEST_ASSERT_FALSE(sensor_decode(frame, &temp, &rh));
}

static void test_decode_rejects_idle_bus_patterns(void)
{
    uint8_t const zeros[SENSOR_FRAME_LEN] = { 0 };
    uint8_t const ones[SENSOR_FRAME_LEN]  = { 0xFFU, 0xFFU, 0xFFU,
                                              0xFFU, 0xFFU, 0xFFU };
    int16_t       temp                    = 0;
    int16_t       rh                      = 0;

    TEST_ASSERT_FALSE(sensor_decode(zeros, &temp, &rh));
    TEST_ASSERT_FALSE(sensor_decode(ones, &temp, &rh));
    TEST_ASSERT_FALSE(sensor_decode(NULL, &temp, &rh));
    TEST_ASSERT_FALSE(sensor_decode(zeros, NULL, &rh));
}

static void test_plausible_range_edges(void)
{
    TEST_ASSERT_TRUE(sensor_temp_plausible(0));
    TEST_ASSERT_TRUE(sensor_temp_plausible(1200));
    TEST_ASSERT_TRUE(sensor_temp_plausible(700));
    TEST_ASSERT_FALSE(sensor_temp_plausible(-1));
    TEST_ASSERT_FALSE(sensor_temp_plausible(1201));
    TEST_ASSERT_FALSE(sensor_temp_plausible(-490));
}

static void test_two_minutes_of_implausible_reads_is_a_fault(void)
{
    uint8_t  frame[SENSOR_FRAME_LEN] = { 0 };
    int16_t  temp                    = 0;
    int16_t  rh                      = 0;
    uint64_t t                       = START_MS;

    sensor_logic_reading(&g_logic, 700, 400, t);
    make_frame(frame, 0x0000U, 0x8000U); // -49 F, good CRC
    for (t = START_MS + 1000U; t <= START_MS + SENSOR_FAULT_MS; t += 1000U)
    {
        // As sensor.c does: only a decoded frame counts as a reading.
        if (sensor_decode(frame, &temp, &rh))
        {
            sensor_logic_reading(&g_logic, temp, rh, t);
        }
    }
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, START_MS + SENSOR_FAULT_MS));
}

static void test_recovery_needs_a_minute_of_good_reads(void)
{
    uint64_t t = START_MS;

    sensor_logic_reading(&g_logic, 700, 400, t);
    t += SENSOR_FAULT_MS; // two minutes of nothing: the fault
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, t));

    // One good read inside the fault leaves it faulted.
    sensor_logic_reading(&g_logic, 700, 400, t);
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, t));

    // Good reads every second: still faulted just short of a minute...
    for (t += 1000U; t < (START_MS + SENSOR_FAULT_MS + SENSOR_RECOVER_MS);
         t += 1000U)
    {
        sensor_logic_reading(&g_logic, 700, 400, t);
        TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, t));
    }
    // ...and valid at a minute.
    sensor_logic_reading(&g_logic, 700, 400, t);
    TEST_ASSERT_TRUE(sensor_logic_valid(&g_logic, t));
}

static void test_a_missed_read_restarts_recovery(void)
{
    uint64_t t     = START_MS;
    uint64_t fault = START_MS + SENSOR_FAULT_MS;

    sensor_logic_reading(&g_logic, 700, 400, t);
    for (t = fault; t < (fault + 59000U); t += 1000U)
    {
        sensor_logic_reading(&g_logic, 700, 400, t);
    }
    // A 5 s gap at 59 s: the minute starts again from the next read.
    t += 5000U;
    sensor_logic_reading(&g_logic, 700, 400, t);
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, t));
    fault = t;
    for (t += 1000U; t < (fault + SENSOR_RECOVER_MS); t += 1000U)
    {
        sensor_logic_reading(&g_logic, 700, 400, t);
        TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, t));
    }
    sensor_logic_reading(&g_logic, 700, 400, t);
    TEST_ASSERT_TRUE(sensor_logic_valid(&g_logic, t));
}

static void test_short_gaps_without_a_fault_stay_valid(void)
{
    sensor_logic_reading(&g_logic, 700, 400, START_MS);
    sensor_logic_reading(&g_logic, 700, 400, START_MS + 10000U); // 10 s gap
    TEST_ASSERT_TRUE(sensor_logic_valid(&g_logic, START_MS + 10000U));
}

static void test_clock_backwards_fails_closed(void)
{
    sensor_logic_reading(&g_logic, 700, 400, START_MS + 5000U);
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, START_MS));
}

static void test_invalid_until_first_reading(void)
{
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, START_MS));
    sensor_logic_reading(&g_logic, 700, 400, START_MS + 100U);
    TEST_ASSERT_TRUE(sensor_logic_valid(&g_logic, START_MS + 100U));
}

static void test_fault_after_two_minutes(void)
{
    sensor_logic_reading(&g_logic, 700, 400, START_MS);
    TEST_ASSERT_TRUE(
        sensor_logic_valid(&g_logic, START_MS + SENSOR_FAULT_MS - 1U));
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, START_MS + SENSOR_FAULT_MS));

    // A read arriving after the fault does not clear it at once (recovery).
    sensor_logic_reading(&g_logic, 701, 400, START_MS + SENSOR_FAULT_MS);
    TEST_ASSERT_FALSE(sensor_logic_valid(&g_logic, START_MS + SENSOR_FAULT_MS));
    TEST_ASSERT_FALSE(sensor_logic_valid(NULL, START_MS));
}

static void test_calibration_offset(void)
{
    sensor_logic_reading(&g_logic, 700, 400, START_MS);
    TEST_ASSERT_EQUAL_INT16(700, sensor_logic_room_f10(&g_logic, 0));
    TEST_ASSERT_EQUAL_INT16(685, sensor_logic_room_f10(&g_logic, -15));
}

static void test_self_heat_lags_toward_target(void)
{
    uint64_t now = START_MS;
    uint32_t sec = 0U;

    sensor_logic_reading(&g_logic, 700, 400, now);

    // Four relays at 0.5 F each: 2.0 F after many time constants. After one
    // time constant a first-order lag has covered about 63 %.
    for (sec = 0U; sec < 600U; sec++)
    {
        now += 1000U;
        sensor_logic_self_heat(&g_logic, 4U, 5, now);
    }
    TEST_ASSERT_INT16_WITHIN(1, 13, sensor_logic_heat_f10(&g_logic));
    TEST_ASSERT_INT16_WITHIN(1, 687, sensor_logic_room_f10(&g_logic, 0));

    for (sec = 0U; sec < 6000U; sec++)
    {
        now += 1000U;
        sensor_logic_self_heat(&g_logic, 4U, 5, now);
    }
    TEST_ASSERT_EQUAL_INT16(20, sensor_logic_heat_f10(&g_logic));
    TEST_ASSERT_EQUAL_INT16(680, sensor_logic_room_f10(&g_logic, 0));

    // And it decays once the relays drop out.
    for (sec = 0U; sec < 6000U; sec++)
    {
        now += 1000U;
        sensor_logic_self_heat(&g_logic, 0U, 5, now);
    }
    TEST_ASSERT_EQUAL_INT16(0, sensor_logic_heat_f10(&g_logic));
}

static void test_self_heat_long_gap_settles(void)
{
    sensor_logic_self_heat(&g_logic, 2U, 5, START_MS + SENSOR_SELF_HEAT_TAU_MS);
    TEST_ASSERT_EQUAL_INT16(10, sensor_logic_heat_f10(&g_logic));
}

static void test_room_saturates(void)
{
    sensor_logic_reading(&g_logic, INT16_MAX, 0, START_MS);
    TEST_ASSERT_EQUAL_INT16(INT16_MAX, sensor_logic_room_f10(&g_logic, 100));
    sensor_logic_reading(&g_logic, INT16_MIN, 0, START_MS);
    TEST_ASSERT_EQUAL_INT16(INT16_MIN, sensor_logic_room_f10(&g_logic, -100));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc_matches_datasheet_example);
    RUN_TEST(test_decode_known_values);
    RUN_TEST(test_decode_extremes);
    RUN_TEST(test_decode_rejects_bad_crc);
    RUN_TEST(test_decode_rejects_idle_bus_patterns);
    RUN_TEST(test_plausible_range_edges);
    RUN_TEST(test_two_minutes_of_implausible_reads_is_a_fault);
    RUN_TEST(test_recovery_needs_a_minute_of_good_reads);
    RUN_TEST(test_a_missed_read_restarts_recovery);
    RUN_TEST(test_short_gaps_without_a_fault_stay_valid);
    RUN_TEST(test_clock_backwards_fails_closed);
    RUN_TEST(test_invalid_until_first_reading);
    RUN_TEST(test_fault_after_two_minutes);
    RUN_TEST(test_calibration_offset);
    RUN_TEST(test_self_heat_lags_toward_target);
    RUN_TEST(test_self_heat_long_gap_settles);
    RUN_TEST(test_room_saturates);
    return UNITY_END();
}
