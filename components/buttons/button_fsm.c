/**
 * @file  button_fsm.c
 * @brief Pure logic for one D-pad key. See button_fsm.h.
 */
#include "button_fsm.h"

#include <stddef.h>

void button_fsm_reset(button_fsm_t * p_fsm)
{
    if (NULL == p_fsm)
    {
        goto done;
    }

    p_fsm->b_pressed      = false;
    p_fsm->disagree_count = 0U;
    p_fsm->next_repeat_ms = 0U;

done:
    return;
}

button_event_t button_fsm_step(button_fsm_t * p_fsm, bool b_raw_pressed,
                               uint64_t now_ms)
{
    button_event_t event = BUTTON_EVENT_NONE;

    if (NULL == p_fsm)
    {
        goto done;
    }

    if (b_raw_pressed == p_fsm->b_pressed)
    {
        p_fsm->disagree_count = 0U;
        if (p_fsm->b_pressed && (now_ms >= p_fsm->next_repeat_ms))
        {
            event                 = BUTTON_EVENT_REPEAT;
            p_fsm->next_repeat_ms = now_ms + BUTTON_FSM_REPEAT_MS;
        }
    }
    else
    {
        p_fsm->disagree_count++;
        if (p_fsm->disagree_count >= BUTTON_FSM_DEBOUNCE_POLLS)
        {
            p_fsm->b_pressed      = b_raw_pressed;
            p_fsm->disagree_count = 0U;
            if (b_raw_pressed)
            {
                event                 = BUTTON_EVENT_PRESS;
                p_fsm->next_repeat_ms = now_ms + BUTTON_FSM_REPEAT_DELAY_MS;
            }
        }
    }

done:
    return event;
}
