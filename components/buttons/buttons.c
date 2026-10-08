/**
 * @file  buttons.c
 * @brief GPIO glue around button_fsm.c. See buttons.h.
 */
#include "buttons.h"

#include "board.h"
#include "driver/gpio.h"
#include <stdbool.h>
#include <stddef.h>

static gpio_num_t const g_key_pins[BUTTONS_KEY_COUNT] = {
    BOARD_PIN_KEY_UP,    BOARD_PIN_KEY_DOWN,   BOARD_PIN_KEY_LEFT,
    BOARD_PIN_KEY_RIGHT, BOARD_PIN_KEY_CENTER,
};

static button_fsm_t g_keys[BUTTONS_KEY_COUNT] = { 0 };

esp_err_t buttons_init(void)
{
    gpio_config_t cfg     = { 0 };
    uint32_t      key_idx = 0U;

    for (key_idx = 0U; key_idx < BUTTONS_KEY_COUNT; key_idx++)
    {
        cfg.pin_bit_mask |= (1ULL << g_key_pins[key_idx]);
        button_fsm_reset(&g_keys[key_idx]);
    }
    cfg.mode      = GPIO_MODE_INPUT;
    cfg.intr_type = GPIO_INTR_DISABLE;
#if BOARD_KEY_ACTIVE_LOW
    cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
#else
    cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
#endif

    return gpio_config(&cfg);
}

void buttons_poll(uint64_t now_ms, button_event_t * p_events)
{
    uint32_t key_idx   = 0U;
    bool     b_pressed = false;

    if (NULL == p_events)
    {
        goto done;
    }

    for (key_idx = 0U; key_idx < BUTTONS_KEY_COUNT; key_idx++)
    {
#if BOARD_KEY_ACTIVE_LOW
        b_pressed = (0 == gpio_get_level(g_key_pins[key_idx]));
#else
        b_pressed = (0 != gpio_get_level(g_key_pins[key_idx]));
#endif
        p_events[key_idx] =
            button_fsm_step(&g_keys[key_idx], b_pressed, now_ms);
    }

done:
    return;
}
