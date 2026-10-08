/**
 * @file  display.h
 * @brief Owns the u8g2 instance and everything that reaches the panel.
 *
 * All of it belongs to one task, the UI task. Nothing here takes a lock, so a
 * second caller would race on u8g2's internal font and buffer state; other
 * tasks raise an app event instead and let the UI task do the drawing. Page
 * modules draw through display_u8g2(), on the UI task only.
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "display_shift.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <u8g2.h>

#define DISPLAY_FONT_SMALL u8g2_font_6x10_tr
#define DISPLAY_FONT_BIG   u8g2_font_logisoso24_tn // digits, '.', '-' only
#define DISPLAY_WIDTH      128
#define DISPLAY_HEIGHT     64

// The area a page may draw in. The burn-in creep (display_shift.h) moves the
// whole frame up to DISPLAY_SHIFT_X_MAX right and DISPLAY_SHIFT_Y_MAX down,
// so content beyond these would be pushed off the panel.
#define DISPLAY_USABLE_W (DISPLAY_WIDTH - (int32_t)DISPLAY_SHIFT_X_MAX)
#define DISPLAY_USABLE_H (DISPLAY_HEIGHT - (int32_t)DISPLAY_SHIFT_Y_MAX)

/**
 * Brings up the panel at the given brightness, on the shared bus
 * (i2c_bus_init() must have run). The panel is left powered on with a
 * cleared frame.
 */
esp_err_t display_init(uint8_t brightness);

/**
 * Sets brightness 1..255. Drives contrast, pre-charge and VCOMH together,
 * because contrast alone barely changes many SSD1306-compatible panels.
 */
void display_set_brightness(uint8_t level);

/**
 * Three lines of small text, for boot, setup and the address overlay. Sends
 * the frame immediately.
 */
void display_status(char const * p_line1, char const * p_line2,
                    char const * p_line3);

/**
 * Powers the panel down or up, acting only on a change, so it is cheap to
 * call every second. Send the frame you want *before* powering up: the panel
 * retains whatever was last written, so powering up first shows a stale
 * frame.
 */
void display_power(bool b_off);

/**
 * Sends the frame drawn in the u8g2 buffer, crept by the burn-in offsets
 * for the current time (display_shift.h). Pages call this instead of
 * u8g2_SendBuffer(). UI task only.
 */
void display_send(void);

/**
 * The u8g2 instance, for page modules drawing on the UI task.
 */
u8g2_t * display_u8g2(void);

#endif /* DISPLAY_H */
