/**
 * @file  app.h
 * @brief The thermostat application: start-up order, then the UI loop.
 */
#ifndef APP_H
#define APP_H

/**
 * Starts whatever connects the thermostat to the outside world (the Matter
 * bridge), once the control loop is already running. The thermostat works
 * the same without one, so it reports failures by logging, not by returning.
 */
typedef void (*app_net_start_t)(void);

/**
 * Drives every relay output low, then brings up NVS, settings, the I2C bus,
 * display, sensor, D-pad and the control task, calls p_net_start (if not
 * NULL), and runs the UI loop on the calling task. Does not return.
 */
void app_start(app_net_start_t p_net_start);

#endif /* APP_H */
