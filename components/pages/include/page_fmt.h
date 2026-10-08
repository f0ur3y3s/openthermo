/**
 * @file  page_fmt.h
 * @brief Pure text formatting for the pages: unit conversion at the display
 *        boundary, tenths and minutes:seconds. No ESP-IDF, host-tested.
 *
 * Everything is stored in tenths of a degree F (docs/CONTROL_SPEC.md);
 * Celsius exists only on screen.
 */
#ifndef PAGE_FMT_H
#define PAGE_FMT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * A temperature in tenths F, as tenths of the display unit (rounded to
 * nearest, halves away from zero).
 */
int16_t page_fmt_temp(int16_t f10, bool b_celsius);

/**
 * A temperature difference in tenths F, as tenths of the display unit.
 */
int16_t page_fmt_delta(int16_t f10, bool b_celsius);

/**
 * Writes tenths as a decimal, e.g. 713 -> "71.3", -5 -> "-0.5". With
 * b_signed, positive values get a '+'. Always NUL-terminates when len > 0.
 */
void page_fmt_tenths(char * p_buf, uint32_t len, int16_t tenths, bool b_signed);

/**
 * Writes seconds as "m:ss", e.g. 299 -> "4:59".
 */
void page_fmt_mmss(char * p_buf, uint32_t len, uint32_t seconds);

#endif /* PAGE_FMT_H */
