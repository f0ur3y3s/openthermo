/**
 * @file  board.c
 * @brief Board set-up. See board.h.
 */
#include "board.h"

#include "driver/gpio.h"

void board_init(void)
{
#if BOARD_HAS_RF_SWITCH
    gpio_config_t const cfg = {
        .pin_bit_mask =
            (1ULL << BOARD_PIN_RF_SWITCH_EN) | (1ULL << BOARD_PIN_RF_ANT_SEL),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    (void)gpio_config(&cfg);
    (void)gpio_set_level(BOARD_PIN_RF_SWITCH_EN, 0U); // switch enabled
    (void)gpio_set_level(BOARD_PIN_RF_ANT_SEL, 0U);   // built-in antenna
#endif
}
