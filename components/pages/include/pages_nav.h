/**
 * @file  pages_nav.h
 * @brief Pure UI logic: which page is showing, and what each D-pad key does
 *        to it and to the settings. No ESP-IDF, host-tested; app_ui.c feeds
 *        it key events and saves the settings it changes.
 *
 * Keys (docs/CONTROL_SPEC.md, local UI):
 *   left / right  previous / next page, wrapping; press only, no repeat
 *   main          up / down move the active setpoint (1 F, or 0.5 C);
 *                 in Auto, centre switches between heat and cool setpoint
 *   mode, fan     up / down move the cursor; centre applies and returns to
 *                 the main page
 *   settings      up / down move the cursor; centre starts editing, then
 *                 up / down change the value and centre stops
 *   info          no keys
 *
 * Setpoint moves go through hvac_setpoints_apply(), so the auto deadband
 * holds after every key press.
 */
#ifndef PAGES_NAV_H
#define PAGES_NAV_H

#include "button_fsm.h"
#include "buttons_keys.h"
#include "settings_types.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PAGES_MAIN = 0,
    PAGES_MODE,
    PAGES_FAN,
    PAGES_SETTINGS,
    PAGES_INFO,
    PAGES_COUNT
} pages_id_t;

typedef enum
{
    PAGES_SET_UNITS = 0,
    PAGES_SET_CAL,
    PAGES_SET_PURGE,
    PAGES_SET_DIM,
    PAGES_SET_OFF,
    PAGES_SET_BRIGHT,
    PAGES_SET_COUNT
} pages_setting_t;

// Edit steps and the ranges the UI offers (inside what settings accepts).
#define PAGES_STEP_SP_F10   10 // 1.0 F
#define PAGES_STEP_SP_C_F10 9  // 0.5 C = 0.9 F
#define PAGES_STEP_CAL_F10  1
#define PAGES_STEP_PURGE_S  10
#define PAGES_STEP_DIM_S    10
#define PAGES_DIM_MAX_S     600
#define PAGES_STEP_OFF_S    60 // screen off: whole minutes
#define PAGES_STEP_BRIGHT   16

typedef struct
{
    pages_id_t page;
    uint8_t    cursor;          // mode, fan and settings list position
    bool       b_editing;       // settings: up / down change the value
    bool       b_cool_selected; // main, in Auto: up / down move cool
} pages_nav_t;

/**
 * Main page, nothing selected or being edited.
 */
void pages_nav_reset(pages_nav_t * p_nav);

/**
 * Applies one key event.
 *
 * @param p_nav navigation state, updated in place
 * @param p_cfg settings, updated in place
 * @param key   which key
 * @param event what it did; BUTTON_EVENT_NONE does nothing
 * @return true if p_cfg changed and should be saved
 */
bool pages_nav_key(pages_nav_t * p_nav, settings_t * p_cfg, buttons_key_t key,
                   button_event_t event);

/**
 * True if the main page's up / down would move the cool setpoint in this
 * mode (for drawing the selection marker).
 */
bool pages_nav_cool_active(pages_nav_t const * p_nav, settings_t const * p_cfg);

#endif /* PAGES_NAV_H */
