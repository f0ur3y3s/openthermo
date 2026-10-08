/**
 * @file  relay_boot_safe.c
 * @brief Drives the four relay pins low at the very start of the 2nd-stage
 *        bootloader, on every reset.
 *
 * A panic, a task-watchdog timeout and esp_restart() all end in a CPU-only
 * reset on the ESP32-C6, which keeps the GPIO output latches: without this,
 * a relay that was on stays on through the ROM, the bootloader and the app's
 * start-up until relays_init() (about 0.67 s measured on the bench, Oct
 * 2026), and indefinitely in a crash loop before app_main(). This runs tens
 * of ms after any reset, before flash init and image validation.
 *
 * It only ever drives the pins LOW (tools/check_safety_sources.py checks
 * that this file never sets an output). The pin map comes from board.h, so
 * it stays single-sourced. After a power-on reset the pins are inputs held
 * low by the 10 k pull-downs until here; from here on they are driven low.
 */
#include "board.h"
#include "esp_rom_gpio.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"
#include <stdint.h>

// Every relay pin is below 32 on the C6, so the mask fits uint32_t.
#define RELAY_PIN_MASK                                                         \
    ((uint32_t)((1ULL << BOARD_PIN_RELAY_Y1) | (1ULL << BOARD_PIN_RELAY_G) |   \
                (1ULL << BOARD_PIN_RELAY_O) | (1ULL << BOARD_PIN_RELAY_W)))

// The bootloader links this component only if something references a symbol
// in it; IDF's bootloader references this one (see IDF's bootloader_hooks
// example).
void bootloader_hooks_include(void);
void bootloader_before_init(void);

void bootloader_hooks_include(void)
{
}

void bootloader_before_init(void)
{
    // gpio_num_t values for these pins are 1..21, so the casts are lossless.
    static uint32_t const s_pins[] = {
        (uint32_t)BOARD_PIN_RELAY_Y1,
        (uint32_t)BOARD_PIN_RELAY_G,
        (uint32_t)BOARD_PIN_RELAY_O,
        (uint32_t)BOARD_PIN_RELAY_W,
    };
    uint32_t idx = 0U;

    // Latch low first, so no pin can go high on the way to being an output.
    REG_WRITE(GPIO_OUT_W1TC_REG, RELAY_PIN_MASK);
    for (idx = 0U; idx < (sizeof(s_pins) / sizeof(s_pins[0])); idx++)
    {
        esp_rom_gpio_pad_select_gpio(s_pins[idx]);
    }
    REG_WRITE(GPIO_ENABLE_W1TS_REG, RELAY_PIN_MASK);
}
