/**
 * @file  settings_types.h
 * @brief The persisted settings record. Pure types, no ESP-IDF, so the host
 *        tests can use it.
 *
 * settings_t is stored in NVS as a raw blob, so its layout IS the on-flash
 * format (docs/CODING_STANDARD.md, rule 10). settings_logic.c pins the size
 * and every offset with _Static_assert. Changing the layout means: bump
 * SETTINGS_VERSION, freeze the old layout in settings_v<N>.h, and add a
 * migration from it.
 *
 * Fields are ordered widest first so the struct has no padding.
 */
#ifndef SETTINGS_TYPES_H
#define SETTINGS_TYPES_H

#include <stdint.h>

#define SETTINGS_VERSION 2U

typedef enum
{
    SETTINGS_UNITS_F = 0,
    SETTINGS_UNITS_C,
    SETTINGS_UNITS_COUNT
} settings_units_t;

// Ranges; anything outside falls back to the default for that field.
#define SETTINGS_CAL_LIMIT_F10       50   // +/- 5.0 F
#define SETTINGS_DISPLAY_TIMEOUT_MAX 3600 // s; 0 = never dim
#define SETTINGS_SCREEN_OFF_MAX      7200 // s; 0 = never off
#define SETTINGS_BRIGHTNESS_MIN      1

typedef struct
{
    uint16_t version;           // SETTINGS_VERSION
    int16_t  heat_sp_f10;       // heat setpoint, tenths F
    int16_t  cool_sp_f10;       // cool setpoint, tenths F
    int16_t  cal_offset_f10;    // added to the sensor reading, tenths F
    uint16_t fan_purge_s;       // G run-on after a call; 0 = none
    uint16_t display_timeout_s; // dim after this long idle; 0 = never
    uint16_t screen_off_s;      // panel off after this long idle; 0 = never
    uint8_t  mode;              // hvac_mode_t
    uint8_t  fan;               // hvac_fan_t
    uint8_t  units;             // settings_units_t, display only
    uint8_t  brightness;        // panel brightness while awake, 1..255
} settings_t;

#endif /* SETTINGS_TYPES_H */
