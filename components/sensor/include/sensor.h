/**
 * @file  sensor.h
 * @brief SHT40 on the shared I2C bus, at the address named by board.h.
 *
 * Runs on the control task only: it polls, then reads the result back with
 * sensor_get(). Other tasks see the reading through the control snapshot.
 *
 * Built with OPENTHERMO_SIM_SENSOR=1 (the C6 bench build, `tools/matter.sh build esp32c6 sim`), the SHT40 is
 * replaced by the simulated room in sensor_sim.h, driven by the relay
 * outputs. Everything downstream, the fault rule included, is unchanged.
 */
#ifndef SENSOR_H
#define SENSOR_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * Self-heating correction per energised relay, in tenths of a degree F.
 * NOT YET MEASURED: 0 until a bench run in the closed case, with relays held
 * on for 30 min beside a reference thermometer, gives a real figure.
 */
#define SENSOR_SELF_HEAT_PER_RELAY_F10 0

typedef struct
{
    bool     b_valid;     // false = fault (no good read for 2 min)
    int16_t  temp_f10;    // room temperature, corrected and calibrated
    int16_t  raw_f10;     // as the sensor read it
    int16_t  heat_f10;    // self-heating correction subtracted
    int16_t  rh_x10;      // relative humidity, tenths of a percent
    uint32_t read_fails;  // I2C or CRC failures since boot
    bool     b_simulated; // bench build: a made-up room, not the SHT40
} sensor_reading_t;

/**
 * Adds the SHT40 to the bus (i2c_bus_init() must have run) and soft-resets
 * it. A missing sensor is not an error here; it shows up as a fault.
 */
esp_err_t sensor_init(void);

/**
 * Takes one high-precision measurement (blocks about 20 ms) and updates the
 * reading.
 *
 * @param now_ms    monotonic time
 * @param relays_on relays energised now, for the self-heating correction
 * @param cal_f10   user calibration offset, tenths of a degree F
 */
void sensor_poll(uint64_t now_ms, uint8_t relays_on, int16_t cal_f10);

/**
 * Copies out the latest reading.
 */
void sensor_get(sensor_reading_t * p_out);

#endif /* SENSOR_H */
