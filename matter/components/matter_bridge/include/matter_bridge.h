/**
 * @file  matter_bridge.h
 * @brief Exposes the thermostat to Matter (and through it to Apple Home).
 *
 * Endpoints:
 *   - Thermostat: SystemMode (Off, Heat, Cool, Auto), occupied heating and
 *     cooling setpoints, local temperature (null on a sensor fault) and the
 *     running state;
 *   - Fan: FanMode Auto or High, for G (bridge_map.h explains the mapping);
 *   - On/Off plug-in unit: emergency heat, since Apple Home has no e-heat
 *     mode.
 *
 * Writes from Matter become settings edits (settings_update), exactly as a
 * key press on the D-pad would, so the control loop, its timers and the
 * relay guard decide what the outputs do; Matter never drives an output.
 * A sync task reports the thermostat's own state back once a second.
 *
 * C interface to a C++ implementation (esp-matter is C++); see
 * docs/CODING_STANDARD.md for the deviation.
 */
#ifndef MATTER_BRIDGE_H
#define MATTER_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Builds the Matter node, starts Matter and the sync task. Call once, after
     * control_start() (app_net_t.p_start). Failures are logged and leave the
     * thermostat running locally.
     */
    void matter_bridge_start(void);

    /**
     * Forgets the pairing (fabrics, Thread credentials) and restarts the chip
     * (app_net_t.p_factory_reset).
     */
    void matter_bridge_factory_reset(void);

    /**
     * The commissioning QR payload ("MT:...") and manual pairing code, and
     * whether the thermostat is paired with any controller now
     * (app_net_t.p_pairing). False before Matter has started.
     */
    bool matter_bridge_pairing(char * p_qr, size_t qr_len, char * p_manual,
                               size_t manual_len, bool * p_b_paired);

#ifdef __cplusplus
}
#endif

#endif /* MATTER_BRIDGE_H */
