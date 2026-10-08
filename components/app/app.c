/**
 * @file  app.c
 * @brief openthermo - heat pump thermostat on the Seeed XIAO ESP32-C6.
 *
 * Start-up order only. The control loop is components/control; the UI loop
 * is app_ui.c.
 */
#include "app.h"

#include "app_ui.h"
#include "board.h"
#include "boot_guard.h"
#include "buttons.h"
#include "control.h"
#include "display.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "i2c_bus.h"
#include "nvs_flash.h"
#include "relays.h"
#include "sensor.h"
#include "settings.h"
#include <stddef.h>

#define LOG_TAG "app"

// The crash-loop breaker's record (boot_guard.h). RTC memory: kept across
// CPU and system resets, garbage after power-on (which reads as no record).
static RTC_NOINIT_ATTR boot_guard_rec_t g_boot_rec;
static esp_timer_handle_t               g_h_boot_clear = NULL;

static bool app_crash_reset(esp_reset_reason_t reason)
{
    return ((ESP_RST_PANIC == reason) || (ESP_RST_INT_WDT == reason) ||
            (ESP_RST_TASK_WDT == reason) || (ESP_RST_WDT == reason) ||
            (ESP_RST_BROWNOUT == reason));
}

// Runs once, BOOT_GUARD_CLEAR_MS after boot: this boot did not crash.
static void app_boot_clear(void * p_arg)
{
    (void)p_arg;
    boot_guard_clear(&g_boot_rec);
}

static void nvs_init(void)
{
    esp_err_t err = nvs_flash_init();

    if ((ESP_ERR_NVS_NO_FREE_PAGES == err) ||
        (ESP_ERR_NVS_NEW_VERSION_FOUND == err))
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);
}

static char const * reset_reason_name(esp_reset_reason_t reason)
{
    char const * p_name = "other";

    switch (reason)
    {
        case ESP_RST_POWERON:
            p_name = "power-on";
            break;
        case ESP_RST_SW:
            p_name = "software";
            break;
        case ESP_RST_USB:
            p_name = "USB (flash or monitor)";
            break;
        case ESP_RST_PANIC:
            p_name = "panic";
            break;
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
            p_name = "WATCHDOG";
            break;
        case ESP_RST_BROWNOUT:
            p_name = "BROWN-OUT";
            break;
        default:
            // "other".
            break;
    }

    return p_name;
}

// Start-up failures abort through ESP_ERROR_CHECK on purpose: the reset that
// follows starts with every relay output low again, which is the safe state.
// See docs/CODING_STANDARD.md, deviation D3.
void app_start(app_net_start_t p_net_start)
{
    settings_t                    cfg     = { 0 };
    esp_reset_reason_t            reason  = ESP_RST_UNKNOWN;
    uint32_t                      crashes = 0U;
    bool                          b_loop  = false;
    esp_timer_create_args_t const clear   = {
          .callback = app_boot_clear,
          .name     = "boot_clear",
    };

    // Hard safety rule: the relay pins go low before anything else runs.
    ESP_ERROR_CHECK(relays_init());
    board_init();

    reason  = esp_reset_reason();
    crashes = boot_guard_on_boot(&g_boot_rec, app_crash_reset(reason));
    b_loop  = boot_guard_tripped(crashes);
    ESP_LOGI(LOG_TAG, "reset reason: %s", reset_reason_name(reason));
    if (b_loop)
    {
        ESP_LOGE(LOG_TAG,
                 "CRASH LOOP: %u crash resets in a row. Matter stays off and "
                 "every output is held off for %u s; the thermostat runs "
                 "locally.",
                 (unsigned)crashes, BOOT_GUARD_HOLD_MS / 1000U);
    }
    // After a clean stretch the count starts over.
    if ((ESP_OK == esp_timer_create(&clear, &g_h_boot_clear)) &&
        (ESP_OK != esp_timer_start_once(g_h_boot_clear,
                                        (uint64_t)BOOT_GUARD_CLEAR_MS * 1000U)))
    {
        ESP_LOGW(LOG_TAG, "crash count clear timer not started");
    }

    nvs_init();
    ESP_ERROR_CHECK(settings_init());
    settings_get(&cfg);

    i2c_bus_line_check(); // logs whether SDA/SCL reach a powered module
    ESP_ERROR_CHECK(i2c_bus_init());
    i2c_bus_scan(); // logs what is wired: OLED 0x3C, SHT40 0x44
    ESP_ERROR_CHECK(display_init(cfg.brightness));
    display_status("openthermo", "starting...", "");

    ESP_ERROR_CHECK(sensor_init());
    ESP_ERROR_CHECK(buttons_init());
    ESP_ERROR_CHECK(control_start(b_loop ? BOOT_GUARD_HOLD_MS : 0U));

    // After the control loop, so a network that is slow or failing to come
    // up never delays the relays being under control. Skipped in a crash
    // loop: Matter is the likeliest cause, and heating and cooling do not
    // need it.
    if ((NULL != p_net_start) && !b_loop)
    {
        p_net_start();
    }

    app_ui_run();
}
