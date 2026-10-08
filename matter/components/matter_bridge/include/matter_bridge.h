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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Builds the Matter node, starts Matter and the sync task. Call once, after
 * control_start(); matches app_net_start_t. Failures are logged and leave
 * the thermostat running locally.
 */
void matter_bridge_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MATTER_BRIDGE_H */
