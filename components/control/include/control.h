/**
 * @file  control.h
 * @brief The control task: once a second it reads the sensor, runs
 *        hvac_logic and drives the relays. It is the only task that touches
 *        the sensor or the relays after start-up, and it feeds the task
 *        watchdog (docs/CONTROL_SPEC.md, faults).
 *
 * Other tasks see what it is doing through control_get(), a snapshot taken
 * under its lock (rule 9).
 */
#ifndef CONTROL_H
#define CONTROL_H

#include "esp_err.h"
#include "hvac_logic.h"
#include "sensor.h"
#include <stdint.h>

#define CONTROL_PERIOD_MS 1000U

typedef struct
{
    hvac_status_t    hvac;    // what the logic asked for, and why
    hvac_outputs_t   applied; // what the relays are actually driving
    sensor_reading_t sensor;  // the reading the logic used
    uint64_t         now_ms;  // when this snapshot was taken
} control_status_t;

/**
 * Starts the control loop and subscribes it to the task watchdog (a failure
 * to subscribe is returned, so start-up aborts rather than run unwatched).
 * relays_init(), settings_init() and sensor_init() must have run.
 *
 * @param hold_off_ms every output is held off for this long from now, while
 *                    the logic runs as usual (the crash-loop breaker, see
 *                    boot_guard.h); 0 for none
 */
esp_err_t control_start(uint32_t hold_off_ms);

/**
 * Copies out the latest snapshot. Safe from any task; all zero (outputs off,
 * no valid reading) until the first cycle completes.
 */
void control_get(control_status_t * p_out);

#endif /* CONTROL_H */
