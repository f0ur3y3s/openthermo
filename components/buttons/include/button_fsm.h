/**
 * @file  button_fsm.h
 * @brief Pure logic for one D-pad key: debounce, press and auto-repeat.
 *
 * Takes the raw level and a timestamp, so it has no hardware or ESP-IDF
 * dependency and runs unchanged in the host tests. buttons.c runs one of
 * these per key.
 *
 *   press   reported once, on the debounced press
 *   repeat  while still held: first after BUTTON_FSM_REPEAT_DELAY_MS, then
 *           every BUTTON_FSM_REPEAT_MS, so holding up/down walks a setpoint
 */
#ifndef BUTTON_FSM_H
#define BUTTON_FSM_H

#include <stdbool.h>
#include <stdint.h>

// Consecutive agreeing polls needed to accept a new level. The debounce
// *duration* is this times the caller's poll period: 40 ms at a 20 ms tick.
#define BUTTON_FSM_DEBOUNCE_POLLS 2U

#define BUTTON_FSM_REPEAT_DELAY_MS 500U
#define BUTTON_FSM_REPEAT_MS       150U

typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESS,
    BUTTON_EVENT_REPEAT
} button_event_t;

typedef struct
{
    bool     b_pressed;      // debounced level: true while held
    uint8_t  disagree_count; // consecutive polls disagreeing with it
    uint64_t next_repeat_ms; // when the next repeat is due, while held
} button_fsm_t;

/**
 * Puts p_fsm in the released state.
 */
void button_fsm_reset(button_fsm_t * p_fsm);

/**
 * Advances the key by one poll and returns at most one event.
 *
 * @param p_fsm         key state; NULL yields BUTTON_EVENT_NONE
 * @param b_raw_pressed undebounced level for this poll
 * @param now_ms        monotonic time of this poll
 */
button_event_t button_fsm_step(button_fsm_t * p_fsm, bool b_raw_pressed,
                               uint64_t now_ms);

#endif /* BUTTON_FSM_H */
