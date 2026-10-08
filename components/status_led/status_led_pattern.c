/**
 * @file  status_led_pattern.c
 * @brief BENCH ONLY: state -> blink pattern for the on-board LED. Pure, so the
 *        host tests cover it. See status_led.h.
 */
#include "status_led.h"

#if OPENTHERMO_STATUS_LED

uint16_t status_led_pattern(bool b_fault, bool b_waiting, bool b_y1, bool b_g,
                            bool b_o, bool b_w)
{
    uint16_t pattern = STATUS_LED_IDLE;

    if (b_fault)
    {
        pattern = STATUS_LED_FAULT;
    }
    else if (b_w)
    {
        pattern = STATUS_LED_AUX;
    }
    else if (b_y1 && b_o)
    {
        pattern = STATUS_LED_COOLING;
    }
    else if (b_y1)
    {
        pattern = STATUS_LED_HEATING;
    }
    else if (b_waiting)
    {
        pattern = STATUS_LED_WAITING;
    }
    else if (b_g)
    {
        pattern = STATUS_LED_FAN;
    }
    else
    {
        // Idle: the heartbeat blip.
    }

    return pattern;
}

bool status_led_lit(uint16_t pattern, uint64_t now_ms)
{
    // The slot index is below STATUS_LED_SLOTS (16), so it fits uint32_t.
    uint32_t slot =
        (uint32_t)((now_ms / STATUS_LED_SLOT_MS) % STATUS_LED_SLOTS);

    return (0U != (((uint32_t)pattern >> slot) & 1U));
}

#endif /* OPENTHERMO_STATUS_LED */
