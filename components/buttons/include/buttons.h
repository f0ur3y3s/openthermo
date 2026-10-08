/**
 * @file  buttons.h
 * @brief The 5-way D-pad on the pins named by board.h.
 *
 * Polled from the UI task's tick rather than interrupt-driven: the tick is
 * fast enough to debounce in software, and that leaves no ISR to keep short.
 * Per-key logic lives in button_fsm.c.
 */
#ifndef BUTTONS_H
#define BUTTONS_H

#include "button_fsm.h"
#include "buttons_keys.h"
#include "esp_err.h"
#include <stdint.h>

/**
 * Configures the five pins as inputs with pull-ups. Call once before
 * polling.
 */
esp_err_t buttons_init(void);

/**
 * Samples every key once.
 *
 * @param now_ms   monotonic time of this poll
 * @param p_events receives one event per key, indexed by buttons_key_t;
 *                 must hold BUTTONS_KEY_COUNT entries
 */
void buttons_poll(uint64_t now_ms, button_event_t * p_events);

#endif /* BUTTONS_H */
