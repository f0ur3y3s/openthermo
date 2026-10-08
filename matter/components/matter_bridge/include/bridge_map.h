/**
 * @file  bridge_map.h
 * @brief Pure mapping between the thermostat's own values and Matter's
 *        attribute values. No ESP-IDF or Matter types, host-tested; the
 *        C++ bridge (matter_bridge.cpp) uses it at the boundary only.
 *
 * Units: the thermostat stores tenths of a degree F (docs/CONTROL_SPEC.md);
 * Matter's Thermostat cluster uses hundredths of a degree C. Converting at
 * this one boundary is the only place Celsius enters the control path.
 *
 * Modes (Thermostat SystemMode): Off, Auto, Cool and Heat map one to one.
 * Emergency heat is its own On/Off endpoint, because Apple Home has no
 * e-heat mode (CLAUDE.md): while it is on, SystemMode reads Heat, and a
 * write of Heat leaves e-heat in force; any other mode turns e-heat off.
 *
 * Fan (Fan Control FanMode, sequence Off/High/Auto): the blower has no
 * speeds and its "off" is the thermostat's Auto, so High and On mean Fan
 * On, and Auto, Smart and Off mean Fan Auto.
 */
#ifndef BRIDGE_MAP_H
#define BRIDGE_MAP_H

#include <stdbool.h>
#include <stdint.h>

// Thermostat cluster SystemMode values (Matter 1.x).
#define BRIDGE_SYSTEM_MODE_OFF   0U
#define BRIDGE_SYSTEM_MODE_AUTO  1U
#define BRIDGE_SYSTEM_MODE_COOL  3U
#define BRIDGE_SYSTEM_MODE_HEAT  4U
#define BRIDGE_SYSTEM_MODE_EHEAT 5U

// Cluster and attribute IDs bridge_map_attr_fixed() knows (Matter 1.x).
#define BRIDGE_CLUSTER_ON_OFF             0x0006UL
#define BRIDGE_CLUSTER_THERMOSTAT         0x0201UL
#define BRIDGE_ATTR_MIN_HEAT_LIMIT        0x0015UL
#define BRIDGE_ATTR_MAX_HEAT_LIMIT        0x0016UL
#define BRIDGE_ATTR_MIN_COOL_LIMIT        0x0017UL
#define BRIDGE_ATTR_MAX_COOL_LIMIT        0x0018UL
#define BRIDGE_ATTR_MIN_DEADBAND          0x0019UL
#define BRIDGE_ATTR_CONTROL_SEQUENCE      0x001BUL
#define BRIDGE_ATTR_START_UP_ON_OFF       0x4003UL
#define BRIDGE_CONTROL_SEQUENCE_HEAT_COOL 4U // cooling and heating

// Thermostat cluster ThermostatRunningState bits.
#define BRIDGE_RUNNING_HEAT        0x0001U
#define BRIDGE_RUNNING_COOL        0x0002U
#define BRIDGE_RUNNING_FAN         0x0004U
#define BRIDGE_RUNNING_HEAT_STAGE2 0x0008U

// Fan Control cluster FanMode values and the sequence the bridge offers.
#define BRIDGE_FAN_MODE_OFF          0U
#define BRIDGE_FAN_MODE_LOW          1U
#define BRIDGE_FAN_MODE_MEDIUM       2U
#define BRIDGE_FAN_MODE_HIGH         3U
#define BRIDGE_FAN_MODE_ON           4U
#define BRIDGE_FAN_MODE_AUTO         5U
#define BRIDGE_FAN_MODE_SMART        6U
#define BRIDGE_FAN_SEQ_OFF_HIGH_AUTO 4U

// Auto deadband for the Thermostat cluster (MinSetpointDeadBand), in 0.1 C.
// The cluster's invariant is cool - heat >= this, and every pair the
// thermostat holds is 3 F apart, which is 1.66 or 1.67 C after conversion:
// 1.6 C is the largest whole tenth that all of them meet. A Home pair closer
// than 3 F is still widened to 3 F by hvac_setpoints_apply().
#define BRIDGE_DEADBAND_C10 16

/*
 * Setpoint limits for the Thermostat cluster, in hundredths C. The cluster
 * checks writes against BOTH the absolute and the operating (Min/Max)
 * limits, and uses Matter's defaults (cool 16..32 C, heat 7..30 C) for any
 * it is not given, so the bridge sets all eight from these, which are the
 * thermostat's own ranges (hvac_logic.h).
 */
typedef struct
{
    int16_t min_heat_c100;
    int16_t max_heat_c100;
    int16_t min_cool_c100;
    int16_t max_cool_c100;
} bridge_limits_t;

/**
 * The thermostat's setpoint ranges in Matter units: each end converted
 * exactly as a reported setpoint is (bridge_map_f10_to_c100), so every
 * setpoint the thermostat reports lies inside them, the ends exactly on
 * them. NULL is ignored.
 */
void bridge_map_limits(bridge_limits_t * p_limits);

/**
 * Tenths F to hundredths C, rounded to nearest, saturated to int16_t.
 */
int16_t bridge_map_f10_to_c100(int16_t f10);

/**
 * Hundredths C to tenths F, rounded to nearest, saturated to int16_t.
 */
int16_t bridge_map_c100_to_f10(int16_t c100);

/**
 * A setpoint written over Matter, in tenths F, snapped to the step the
 * thermostat itself uses: a whole degree F, or half a degree C when the
 * display is in Celsius.
 *
 * The Home app converts the user's whole degree F to Celsius with its own
 * rounding (65 F has arrived as 65.1 or 65.2 F after the exact conversion),
 * and the thermostat should store the 65 the user picked.
 *
 * @param c100      the written value, hundredths C
 * @param b_celsius true when the thermostat displays Celsius
 */
int16_t bridge_map_setpoint_from_c100(int16_t c100, bool b_celsius);

/**
 * A setpoint write over Matter, applied to the thermostat's setpoint pair:
 * snapped (bridge_map_setpoint_from_c100), then kept inside the ranges and
 * the Auto deadband by hvac_setpoints_apply(), the written one leading. The
 * pair that results always satisfies hvac_setpoints_valid(). NULL pointers
 * are ignored.
 *
 * @param c100       the written value, hundredths C
 * @param b_celsius  true when the thermostat displays Celsius
 * @param b_heat     true for the heating setpoint, false for cooling
 * @param p_heat_f10 the heat setpoint, updated in place
 * @param p_cool_f10 the cool setpoint, updated in place
 */
void bridge_map_setpoint_write(int16_t c100, bool b_celsius, bool b_heat,
                               int16_t * p_heat_f10, int16_t * p_cool_f10);

/**
 * True for an attribute the thermostat sets itself at every boot and a
 * controller may not change: the operating setpoint limits, the Auto
 * deadband and the control sequence (they define what the thermostat
 * accepts), and the e-heat endpoint's StartUpOnOff (null, so a reboot never
 * switches e-heat off). The bridge refuses writes to these.
 */
bool bridge_map_attr_fixed(uint32_t cluster_id, uint32_t attribute_id);

/**
 * The SystemMode to report for a thermostat mode (hvac_mode_t value).
 * E-heat reports Heat; anything unknown reports Off.
 */
uint8_t bridge_map_system_mode(uint8_t hvac_mode);

/**
 * The thermostat mode a SystemMode write asks for.
 *
 * @param system_mode  the value written
 * @param current_mode the thermostat's mode now (hvac_mode_t value)
 * @param p_mode       receives the new mode
 * @return false (p_mode untouched) for a SystemMode the thermostat does not
 *         support, or a NULL p_mode
 */
bool bridge_map_mode_from_system(uint8_t system_mode, uint8_t current_mode,
                                 uint8_t * p_mode);

/**
 * The thermostat mode after the e-heat endpoint is switched: on forces
 * e-heat; off returns e-heat to the mode it was entered from (restore_mode)
 * when that is Off, Heat, Cool or Auto, else to Heat, and leaves any mode
 * other than e-heat alone.
 *
 * @param b_on         the endpoint's new state
 * @param current_mode the thermostat's mode now (hvac_mode_t value)
 * @param restore_mode the last mode before e-heat (hvac_mode_t value)
 */
uint8_t bridge_map_mode_from_eheat(bool b_on, uint8_t current_mode,
                                   uint8_t restore_mode);

/**
 * The FanMode to report for a thermostat fan setting (hvac_fan_t value).
 */
uint8_t bridge_map_fan_mode(uint8_t hvac_fan);

/**
 * The thermostat fan setting (hvac_fan_t value) a FanMode write asks for.
 */
uint8_t bridge_map_fan_from_mode(uint8_t fan_mode);

/**
 * ThermostatRunningState for the outputs being driven. Heat is the
 * compressor in heat or any strips; Cool is the compressor with O; aux strips
 * on top of the compressor also set Heat Stage 2.
 */
uint16_t bridge_map_running_state(bool b_y1, bool b_g, bool b_o, bool b_w);

#endif /* BRIDGE_MAP_H */
