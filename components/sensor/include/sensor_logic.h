/**
 * @file  sensor_logic.h
 * @brief Pure sensor logic: SHT40 frame decoding, the 2 min fault rule and
 *        the self-heating correction.
 *
 * No ESP-IDF dependency, so it runs unchanged in the host tests; sensor.c
 * feeds it from the I2C bus. Temperatures are int16_t tenths of a degree F
 * (docs/CONTROL_SPEC.md); humidity is tenths of a percent.
 *
 * Self-heating: each energised relay coil dissipates about 0.35 W inside the
 * case, which warms the sensor chamber over minutes, not instantly. The
 * correction therefore follows the relay count through a first-order lag
 * with time constant SENSOR_SELF_HEAT_TAU_MS, and is subtracted from the
 * reading.
 */
#ifndef SENSOR_LOGIC_H
#define SENSOR_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#define SENSOR_FRAME_LEN        6U        // T msb, lsb, crc, RH msb, lsb, crc
#define SENSOR_FAULT_MS         120000ULL // no good read for 2 min = fault
#define SENSOR_SELF_HEAT_TAU_MS 600000ULL // 10 min

// A reading outside this range is a failed read, whatever its CRC: no room
// the thermostat controls is below 0 F or above 120 F, and a broken sensor
// that reported -49 F with a good CRC would otherwise run heat and strips
// without end. Wide enough that a cold house still calls for heat.
#define SENSOR_PLAUSIBLE_MIN_F10 0    // 0.0 F
#define SENSOR_PLAUSIBLE_MAX_F10 1200 // 120.0 F

// After a fault, the reading counts as valid again only after this long of
// unbroken good reads, so a flaky link cannot start the compressor on one
// good read and fault it two minutes later. A gap between good reads longer
// than SENSOR_GAP_MS restarts the count. The first reading after boot needs
// no recovery: the 5 min boot lockout covers it.
#define SENSOR_RECOVER_MS 60000ULL // 1 min
#define SENSOR_GAP_MS     2500ULL  // reads come every 1 s

typedef struct
{
    bool     b_have_reading; // at least one good frame since boot
    bool     b_recovering;   // a fault was declared; not yet recovered
    uint64_t last_good_ms;   // when the last good frame arrived
    uint64_t good_since_ms;  // start of the current unbroken run
    uint64_t last_heat_ms;   // when the self-heat lag last advanced
    int16_t  raw_f10;        // last good temperature, uncorrected
    int16_t  rh_x10;         // last good humidity
    int32_t  heat_milli_f10; // self-heat correction, thousandths of 0.1 F
} sensor_logic_t;

/**
 * Sensirion CRC-8: polynomial 0x31, initial value 0xFF, no reflection.
 */
uint8_t sensor_crc8(uint8_t const * p_data, uint32_t len);

/**
 * True if a temperature lies in SENSOR_PLAUSIBLE_MIN_F10..MAX_F10.
 */
bool sensor_temp_plausible(int16_t temp_f10);

/**
 * Checks both CRCs of an SHT40 measurement frame and converts it. A
 * temperature outside the plausible range (sensor_temp_plausible) fails
 * like a bad CRC.
 *
 * @param p_frame    SENSOR_FRAME_LEN bytes as read from the sensor
 * @param p_temp_f10 receives the temperature in tenths of a degree F
 * @param p_rh_x10   receives relative humidity in tenths of a percent,
 *                   clamped to 0..1000
 * @return false (and outputs untouched) on a CRC mismatch, an implausible
 *         temperature or a NULL argument
 */
bool sensor_decode(uint8_t const * p_frame, int16_t * p_temp_f10,
                   int16_t * p_rh_x10);

/**
 * No reading yet, no self-heat correction, the lag clock started at now_ms.
 */
void sensor_logic_init(sensor_logic_t * p_logic, uint64_t now_ms);

/**
 * Records a good reading.
 */
void sensor_logic_reading(sensor_logic_t * p_logic, int16_t temp_f10,
                          int16_t rh_x10, uint64_t now_ms);

/**
 * Advances the self-heat lag toward relays_on * per_relay_f10.
 */
void sensor_logic_self_heat(sensor_logic_t * p_logic, uint8_t relays_on,
                            int16_t per_relay_f10, uint64_t now_ms);

/**
 * True if a good reading arrived less than SENSOR_FAULT_MS ago and, after a
 * fault, good reads have run unbroken for SENSOR_RECOVER_MS. False if the
 * clock reads earlier than the last reading (fails closed).
 */
bool sensor_logic_valid(sensor_logic_t const * p_logic, uint64_t now_ms);

/**
 * The current self-heat correction, rounded to tenths of a degree F.
 */
int16_t sensor_logic_heat_f10(sensor_logic_t const * p_logic);

/**
 * The room temperature: the last good reading, less the self-heat
 * correction, plus the user's calibration offset, saturated to int16_t.
 */
int16_t sensor_logic_room_f10(sensor_logic_t const * p_logic, int16_t cal_f10);

#endif /* SENSOR_LOGIC_H */
