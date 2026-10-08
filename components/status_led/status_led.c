/**
 * @file  status_led.c
 * @brief BENCH ONLY: drives the on-board user LED. See status_led.h.
 */
#include "status_led.h"

#if OPENTHERMO_STATUS_LED

#include "board.h"
#include "driver/gpio.h"

#ifndef BOARD_PIN_STATUS_LED
#error "OPENTHERMO_STATUS_LED needs BOARD_PIN_STATUS_LED in board.h"
#endif

static bool g_b_lit   = false;
static bool g_b_known = false; // the pin has been written at least once

static void status_led_write(bool b_lit)
{
#if BOARD_STATUS_LED_ACTIVE_LOW
    (void)gpio_set_level(BOARD_PIN_STATUS_LED, b_lit ? 0U : 1U);
#else
    (void)gpio_set_level(BOARD_PIN_STATUS_LED, b_lit ? 1U : 0U);
#endif
    g_b_lit   = b_lit;
    g_b_known = true;
}

void status_led_init(void)
{
    gpio_config_t const cfg = {
        .pin_bit_mask = 1ULL << BOARD_PIN_STATUS_LED,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    status_led_write(false); // set the latch before the driver turns on
    (void)gpio_config(&cfg);
    status_led_write(false);
}

void status_led_update(uint16_t pattern, uint64_t now_ms)
{
    bool b_lit = status_led_lit(pattern, now_ms);

    if (!g_b_known || (b_lit != g_b_lit))
    {
        status_led_write(b_lit);
    }
}

#endif /* OPENTHERMO_STATUS_LED */
