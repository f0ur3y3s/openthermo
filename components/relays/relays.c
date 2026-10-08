/**
 * @file  relays.c
 * @brief GPIO glue around relays_guard.c. See relays.h.
 */
#include "relays.h"

#include "board.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define LOG_TAG "relays"

// Every relay pin is below 32 on the C6, so the mask fits uint32_t.
#define RELAYS_PIN_MASK                                                        \
    ((uint32_t)((1ULL << BOARD_PIN_RELAY_Y1) | (1ULL << BOARD_PIN_RELAY_G) |   \
                (1ULL << BOARD_PIN_RELAY_O) | (1ULL << BOARD_PIN_RELAY_W)))

// The BT controller's own panic-handler wrap would collide with ours.
#if CONFIG_BT_LE_CONTROLLER_LOG_WRAP_PANIC_HANDLER_ENABLE
#error                                                                         \
    "relays.c wraps esp_panic_handler; the BT controller log wrap must stay off"
#endif

#define US_PER_MS 1000

static relays_guard_t g_guard       = { 0 };
static bool           g_b_inhibited = false; // relays_inhibit() was called

static void relays_write(gpio_num_t pin, bool b_on)
{
    (void)gpio_set_level(pin, b_on ? 1U : 0U);
}

void IRAM_ATTR relays_drop_all_now(void)
{
    REG_WRITE(GPIO_OUT_W1TC_REG, RELAYS_PIN_MASK);
}

// Linker wrap of the panic handler (-Wl,--wrap=esp_panic_handler, in this
// component's CMakeLists.txt). Every panic, abort() and task-watchdog timeout
// goes through esp_panic_handler(), so the relays drop at the instant of the
// panic, before the backtrace prints and the CPU-only reset that keeps the
// GPIO latches. The names are fixed by the linker (CODING_STANDARD.md, D10).
// NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
void __real_esp_panic_handler(void * p_info);
void __wrap_esp_panic_handler(void * p_info);

void IRAM_ATTR __wrap_esp_panic_handler(void * p_info)
{
    relays_drop_all_now();
    __real_esp_panic_handler(p_info);
}
// NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)

esp_err_t relays_init(void)
{
    esp_err_t           err  = ESP_FAIL;
    uint32_t            held = 0U;
    gpio_config_t const cfg  = {
         .pin_bit_mask =
            (1ULL << BOARD_PIN_RELAY_Y1) | (1ULL << BOARD_PIN_RELAY_G) |
            (1ULL << BOARD_PIN_RELAY_O) | (1ULL << BOARD_PIN_RELAY_W),
         .mode         = GPIO_MODE_OUTPUT,
         .pull_up_en   = GPIO_PULLUP_DISABLE,
         .pull_down_en = GPIO_PULLDOWN_DISABLE,
         .intr_type    = GPIO_INTR_DISABLE,
    };

    // What the pins were doing when the app started, read before anything
    // touches them. A CPU-only reset keeps the GPIO latches; the panic wrap,
    // the shutdown handler and the bootloader hook (relay_boot_safe.c) each
    // drop the relays first, so a relay pin still enabled and high here
    // means all three failed. Logged below, once the pins are low.
    held = REG_READ(GPIO_OUT_REG) & REG_READ(GPIO_ENABLE_REG) & RELAYS_PIN_MASK;

    // Set the output latches low before enabling the drivers, so no pin is
    // ever driven high on the way to becoming an output.
    relays_write(BOARD_PIN_RELAY_Y1, false);
    relays_write(BOARD_PIN_RELAY_G, false);
    relays_write(BOARD_PIN_RELAY_O, false);
    relays_write(BOARD_PIN_RELAY_W, false);

    err = gpio_config(&cfg);

    relays_write(BOARD_PIN_RELAY_Y1, false);
    relays_write(BOARD_PIN_RELAY_G, false);
    relays_write(BOARD_PIN_RELAY_O, false);
    relays_write(BOARD_PIN_RELAY_W, false);

    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    relays_guard_init(&g_guard, (uint64_t)(esp_timer_get_time() / US_PER_MS));

    // esp_restart() (a factory reset, the console, IDF itself) runs the
    // shutdown handlers before its CPU-only reset: drop the relays there.
    if (ESP_OK != esp_register_shutdown_handler(relays_drop_all_now))
    {
        err = ESP_FAIL;
    }

    if (0U != held)
    {
        ESP_LOGW(LOG_TAG,
                 "relay pins were still driven at start-up (Y1 %d G %d O %d "
                 "W %d): the reset kept them energised until now",
                 (0U != (held & (1UL << BOARD_PIN_RELAY_Y1))),
                 (0U != (held & (1UL << BOARD_PIN_RELAY_G))),
                 (0U != (held & (1UL << BOARD_PIN_RELAY_O))),
                 (0U != (held & (1UL << BOARD_PIN_RELAY_W))));
    }
    else
    {
        ESP_LOGI(LOG_TAG, "relay pins idle at start-up");
    }

    return err;
}

void relays_apply(relays_outputs_t const * p_req)
{
    relays_outputs_t req     = { 0 };
    relays_outputs_t out     = { 0 };
    bool             b_pass  = false;
    uint32_t         resyncs = g_guard.resyncs;
    // The guard's own read of the clock, independent of the control task's.
    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    uint64_t now_ms = (uint64_t)(esp_timer_get_time() / US_PER_MS);

    if ((NULL != p_req) && !g_b_inhibited)
    {
        req = *p_req;
    }

    b_pass = relays_guard_step(&g_guard, &req, now_ms, &out);
    if (resyncs != g_guard.resyncs)
    {
        ESP_LOGE(LOG_TAG, "guard state corrupted: all off, min-off restarted");
    }
    if (!b_pass)
    {
        ESP_LOGW(LOG_TAG,
                 "guard changed request Y1%u G%u O%u W%u -> Y1%u G%u O%u W%u",
                 req.b_y1, req.b_g, req.b_o, req.b_w, out.b_y1, out.b_g,
                 out.b_o, out.b_w);
    }

    // Offs first, then the valve and blower, then W, and Y1 last.
    if (!out.b_y1)
    {
        relays_write(BOARD_PIN_RELAY_Y1, false);
    }
    if (!out.b_w)
    {
        relays_write(BOARD_PIN_RELAY_W, false);
    }
    relays_write(BOARD_PIN_RELAY_O, out.b_o);
    relays_write(BOARD_PIN_RELAY_G, out.b_g);
    if (out.b_w)
    {
        relays_write(BOARD_PIN_RELAY_W, true);
    }
    if (out.b_y1)
    {
        relays_write(BOARD_PIN_RELAY_Y1, true);
    }
}

void relays_inhibit(void)
{
    g_b_inhibited = true;
}

void relays_applied(relays_outputs_t * p_out)
{
    if (NULL != p_out)
    {
        *p_out = g_guard.applied;
    }
}

uint8_t relays_energised_count(void)
{
    uint32_t count = 0U;

    count += g_guard.applied.b_y1 ? 1U : 0U;
    count += g_guard.applied.b_g ? 1U : 0U;
    count += g_guard.applied.b_o ? 1U : 0U;
    count += g_guard.applied.b_w ? 1U : 0U;

    // At most 4.
    return (uint8_t)count;
}
