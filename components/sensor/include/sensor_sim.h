/**
 * @file  sensor_sim.h
 * @brief A simulated room for bench testing without an SHT40. Pure logic, no
 *        ESP-IDF, host-tested.
 *
 * Compiled in only by the C6 bench build (OPENTHERMO_SIM_SENSOR), which feeds
 * the control logic this temperature instead of the sensor's. The room
 * leaks heat toward an outdoor temperature and is pushed by whatever the
 * relays are driving, so LEDs on the IN pins show the real control logic,
 * with its real timers, cycling on its own:
 *
 *   - outdoor swings 45 -> 100 -> 45 F over SENSOR_SIM_PERIOD_MS, so Auto
 *     heats, then waits out the changeover, then cools;
 *   - the room relaxes toward outdoor with time constant
 *     SENSOR_SIM_LOSS_TAU_MS;
 *   - Y1 with O off heats, Y1 with O on cools, and W adds aux heat.
 *
 * NEVER run a sim build on the installed thermostat: it would control the
 * house from a made-up temperature.
 */
#ifndef SENSOR_SIM_H
#define SENSOR_SIM_H

#include <stdbool.h>
#include <stdint.h>

#define SENSOR_SIM_START_F10        700 // the room at boot, 70.0 F
#define SENSOR_SIM_OUTDOOR_MIN_F10  450
#define SENSOR_SIM_OUTDOOR_MAX_F10  1000
#define SENSOR_SIM_PERIOD_MS        7200000ULL // 2 h, one full swing
#define SENSOR_SIM_LOSS_TAU_MS      1800000ULL // 30 min
#define SENSOR_SIM_HEAT_F10_PER_MIN 6          // heat pump: 0.6 F/min
#define SENSOR_SIM_COOL_F10_PER_MIN 6
#define SENSOR_SIM_AUX_F10_PER_MIN  10       // strips: 1.0 F/min
#define SENSOR_SIM_MAX_STEP_MS      60000ULL // longer gaps count as this
#define SENSOR_SIM_RH_X10           450      // a fixed 45.0 %

typedef struct
{
    int64_t  temp_milli_f10; // room, in thousandths of 0.1 F
    uint64_t last_ms;        // when the model last advanced
} sensor_sim_t;

/**
 * Starts the room at SENSOR_SIM_START_F10.
 */
void sensor_sim_init(sensor_sim_t * p_sim, uint64_t now_ms);

/**
 * The outdoor temperature at now_ms: a triangle wave from the minimum (at
 * boot) to the maximum and back, once per SENSOR_SIM_PERIOD_MS.
 */
int16_t sensor_sim_outdoor_f10(uint64_t now_ms);

/**
 * Advances the room to now_ms with these outputs driven since the last call,
 * and returns its temperature in tenths F. NULL returns the start value.
 */
int16_t sensor_sim_step(sensor_sim_t * p_sim, bool b_y1, bool b_o, bool b_w,
                        uint64_t now_ms);

#endif /* SENSOR_SIM_H */
