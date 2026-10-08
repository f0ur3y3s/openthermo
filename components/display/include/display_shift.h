/**
 * @file  display_shift.h
 * @brief Pure burn-in protection: slowly creeps the whole frame a few pixels,
 *        so no OLED pixel shows the same static image (the mode label, the
 *        output boxes) around the clock. After deskmate's clock face.
 *
 * No ESP-IDF, host-tested. display_send() applies it to every frame just
 * before it reaches the panel, so it covers every page. Pages keep their
 * content inside DISPLAY_SHIFT_X_MAX / DISPLAY_SHIFT_Y_MAX of the right and
 * bottom edges, so the creep never pushes anything off-screen.
 *
 * The offsets follow triangle waves: x steps one pixel every
 * DISPLAY_SHIFT_X_PERIOD_S out to the maximum and back; y the same, slower.
 */
#ifndef DISPLAY_SHIFT_H
#define DISPLAY_SHIFT_H

#include <stdint.h>

#define DISPLAY_SHIFT_X_MAX      3U   // pixels
#define DISPLAY_SHIFT_Y_MAX      2U   // pixels
#define DISPLAY_SHIFT_X_PERIOD_S 60U  // seconds per x step
#define DISPLAY_SHIFT_Y_PERIOD_S 450U // seconds per y step

/**
 * The creep offsets at now_ms: dx in 0..DISPLAY_SHIFT_X_MAX, dy in
 * 0..DISPLAY_SHIFT_Y_MAX. NULL outputs are ignored.
 */
void display_shift_offsets(uint64_t now_ms, uint8_t * p_dx, uint8_t * p_dy);

/**
 * Moves a frame dx pixels right and dy pixels down, in place. Pixels pushed
 * past the right or bottom edge are dropped; the strips uncovered on the
 * left and top are cleared.
 *
 * @param p_buf  frame in SSD1306 page layout: `pages` rows of `width`
 *               bytes, each byte one column of 8 pixels, bit 0 at the top
 *               (u8g2's full-frame buffer)
 * @param width  columns (128)
 * @param pages  8-pixel rows (8 for 64 pixels; at most 8)
 * @param dx     pixels right, below width
 * @param dy     pixels down, at most 8 * pages
 */
void display_shift_buffer(uint8_t * p_buf, uint32_t width, uint32_t pages,
                          uint8_t dx, uint8_t dy);

#endif /* DISPLAY_SHIFT_H */
