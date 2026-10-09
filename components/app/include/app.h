/**
 * @file  app.h
 * @brief The thermostat application: start-up order, then the UI loop.
 */
#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Whatever connects the thermostat to the outside world (the Matter
 * bridge). The thermostat works the same without one; any member may be
 * NULL.
 */
typedef struct
{
    /**
     * Starts the network, once the control loop is already running.
     * Failures are logged, not returned.
     */
    void (*p_start)(void);

    /**
     * Forgets the pairing and restarts the chip (the D-pad's factory reset;
     * the thermostat's own settings are reset by the caller first).
     */
    void (*p_factory_reset)(void);

    /**
     * The pairing codes for the pair page: the QR payload, the manual code,
     * and whether the thermostat is paired now. False while there are none
     * to show (the network not started).
     */
    bool (*p_pairing)(char * p_qr, size_t qr_len, char * p_manual,
                      size_t manual_len, bool * p_b_paired);
} app_net_t;

/**
 * Drives every relay output low, then brings up NVS, settings, the I2C bus,
 * display, sensor, D-pad and the control task, starts the network (p_net
 * may be NULL), and runs the UI loop on the calling task. Does not return.
 */
void app_start(app_net_t const * p_net);

#endif /* APP_H */
