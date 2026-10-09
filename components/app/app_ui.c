/**
 * @file  app_ui.c
 * @brief The UI loop. See app_ui.h.
 *
 * Every tick it polls the D-pad, hands key events to pages_nav (which
 * changes the page and the settings), saves changed settings, and redraws
 * when a key was pressed or UI_REDRAW_MS has passed. After the display
 * timeout it dims the panel and returns to the main page; the next key only
 * wakes it.
 */
#include "app_ui.h"

#include "buttons.h"
#include "control.h"
#include "display.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pages.h"
#include "pages_nav.h"
#include "settings.h"
#include "status_led.h"
#include <stdbool.h>
#include <stdint.h>

#define LOG_TAG "app_ui"

#define UI_TICK_MS        20U
#define UI_REDRAW_MS      250U
#define UI_PAIR_POLL_MS   2000U // pairing state for the pair page
#define UI_RESET_WAIT_MS  5000U // for the network's own restart
#define UI_DIM_BRIGHTNESS 1U
#define US_PER_MS         1000
#define MS_PER_S          1000U

typedef struct
{
    pages_nav_t nav;
    bool        b_dimmed;
    bool        b_off;       // panel switched off (burn-in)
    uint8_t     brightness;  // what the panel is set to now
    uint64_t    last_key_ms; // for the dim timeout
    uint64_t    last_draw_ms;
    uint64_t    last_pair_ms;
    bool        b_pair_shown; // the pair page was shown at boot already
} app_ui_t;

static app_net_t const * gp_net = NULL; // set once by app_ui_run()

static uint64_t app_ui_now_ms(void)
{
    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    return (uint64_t)(esp_timer_get_time() / US_PER_MS);
}

// One key event, applied to the settings under their lock.
typedef struct
{
    pages_nav_t *  p_nav;
    buttons_key_t  key;
    button_event_t event;
} app_ui_key_edit_t;

static bool app_ui_key_edit(settings_t * p_cfg, void * p_ctx)
{
    app_ui_key_edit_t const * p_edit = (app_ui_key_edit_t const *)p_ctx;

    // pages_nav_key() is pure: no blocking, logging or locks.
    return pages_nav_key(p_edit->p_nav, p_cfg, p_edit->key, p_edit->event);
}

// Applies this tick's key events. Returns true if anything was pressed.
static bool app_ui_keys(app_ui_t * p_ui, button_event_t const * p_events,
                        uint64_t now_ms)
{
    bool              b_any   = false;
    uint32_t          key_idx = 0U;
    app_ui_key_edit_t edit    = { 0 };

    for (key_idx = 0U; key_idx < (uint32_t)BUTTONS_KEY_COUNT; key_idx++)
    {
        if (BUTTON_EVENT_NONE != p_events[key_idx])
        {
            b_any = true;
        }
    }
    if (!b_any)
    {
        goto done;
    }

    p_ui->last_key_ms = now_ms;
    if (p_ui->b_dimmed)
    {
        // The first key only wakes the panel.
        if (p_ui->b_off)
        {
            display_power(false);
            p_ui->b_off = false;
        }
        p_ui->b_dimmed   = false;
        p_ui->brightness = 0U; // forces the brightness to be re-applied
        goto done;
    }

    edit.p_nav = &p_ui->nav;
    for (key_idx = 0U; key_idx < (uint32_t)BUTTONS_KEY_COUNT; key_idx++)
    {
        if (BUTTON_EVENT_NONE != p_events[key_idx])
        {
            // key_idx < BUTTONS_KEY_COUNT, a valid buttons_key_t enumerator.
            edit.key   = (buttons_key_t)key_idx;
            edit.event = p_events[key_idx];
            (void)settings_update(app_ui_key_edit, &edit, now_ms);
        }
    }

done:
    return b_any;
}

// Refreshes the pair page's codes. At boot, an unpaired thermostat opens on
// the pair page, so the QR code is the first thing shown.
static void app_ui_pairing(app_ui_t * p_ui)
{
    char qr[PAGES_PAIR_QR_LEN]         = { 0 };
    char manual[PAGES_PAIR_MANUAL_LEN] = { 0 };
    bool b_paired                      = false;

    if ((NULL == gp_net) || (NULL == gp_net->p_pairing))
    {
        pages_set_pairing(NULL, NULL, false);
    }
    else if (gp_net->p_pairing(qr, sizeof(qr), manual, sizeof(manual),
                               &b_paired))
    {
        pages_set_pairing(qr, manual, b_paired);
        if (!p_ui->b_pair_shown && !b_paired && !p_ui->b_dimmed)
        {
            p_ui->nav.page = PAGES_PAIR;
        }
        p_ui->b_pair_shown = true;
    }
    else
    {
        // Not started yet: the page keeps saying so.
    }
}

// The confirmed factory reset: the thermostat's settings back to their
// defaults (mode Off), then the network forgets its pairing and restarts the
// chip. Every relay drops at that restart (relays.h), and the next boot
// re-arms the minimum-off time as any boot does.
static void app_ui_factory_reset(void)
{
    ESP_LOGW(LOG_TAG, "factory reset from the settings page");
    display_status("Factory reset", "resetting...", "");
    if (ESP_OK != settings_reset())
    {
        ESP_LOGE(LOG_TAG, "settings reset failed: restarting anyway");
    }
    if ((NULL != gp_net) && (NULL != gp_net->p_factory_reset))
    {
        gp_net->p_factory_reset(); // schedules its own restart
        vTaskDelay(pdMS_TO_TICKS(UI_RESET_WAIT_MS));
    }
    esp_restart(); // no network, or it did not restart
}

// Switches the panel off after the screen-off time with no key, which stops
// OLED wear entirely; dimming only slows it. A sensor fault keeps (or turns)
// the panel on, dimmed, so the fault stays on show. Any key wakes it.
static void app_ui_power(app_ui_t * p_ui, settings_t const * p_cfg,
                         bool b_fault, uint64_t now_ms)
{
    uint64_t off_ms = (uint64_t)p_cfg->screen_off_s * MS_PER_S;

    if (!p_ui->b_off && !b_fault && (0U != off_ms) &&
        ((now_ms - p_ui->last_key_ms) >= off_ms))
    {
        display_power(true);
        p_ui->b_off    = true;
        p_ui->b_dimmed = true; // so the waking key does nothing else
        pages_nav_reset(&p_ui->nav);
    }
    else if (p_ui->b_off && b_fault)
    {
        display_power(false);
        p_ui->b_off = false; // still dimmed
    }
    else
    {
        // Nothing to change.
    }
}

// Dims after the timeout; keeps the panel at the configured brightness
// otherwise.
static void app_ui_brightness(app_ui_t * p_ui, settings_t const * p_cfg,
                              uint64_t now_ms)
{
    uint64_t timeout_ms = (uint64_t)p_cfg->display_timeout_s * MS_PER_S;

    if (!p_ui->b_dimmed && (0U != timeout_ms) &&
        ((now_ms - p_ui->last_key_ms) >= timeout_ms))
    {
        p_ui->b_dimmed = true;
        pages_nav_reset(&p_ui->nav);
        display_set_brightness(UI_DIM_BRIGHTNESS);
        p_ui->brightness = UI_DIM_BRIGHTNESS;
    }
    else if (!p_ui->b_dimmed && (p_cfg->brightness != p_ui->brightness))
    {
        display_set_brightness(p_cfg->brightness);
        p_ui->brightness = p_cfg->brightness;
    }
    else
    {
        // Nothing to change.
    }
}

void app_ui_run(app_net_t const * p_net)
{
    app_ui_t         ui                        = { 0 };
    button_event_t   events[BUTTONS_KEY_COUNT] = { BUTTON_EVENT_NONE };
    settings_t       cfg                       = { 0 };
    control_status_t status                    = { 0 };
    uint64_t         now_ms                    = app_ui_now_ms();
    bool             b_redraw                  = true;

    gp_net = p_net;
    pages_nav_reset(&ui.nav);
#if OPENTHERMO_STATUS_LED
    status_led_init();
#endif
    settings_get(&cfg);
    ui.brightness  = cfg.brightness; // display_init() already applied it
    ui.last_key_ms = now_ms;

    for (;;)
    {
        now_ms = app_ui_now_ms();
        buttons_poll(now_ms, events);
        b_redraw = app_ui_keys(&ui, events, now_ms);
        if (pages_nav_take_reset(&ui.nav))
        {
            app_ui_factory_reset(); // does not return
        }
        if ((now_ms - ui.last_pair_ms) >= UI_PAIR_POLL_MS)
        {
            app_ui_pairing(&ui);
            ui.last_pair_ms = now_ms;
        }

        settings_get(&cfg);
        control_get(&status);
        app_ui_power(&ui, &cfg, status.hvac.b_fault, now_ms);
        app_ui_brightness(&ui, &cfg, now_ms);

        // Nothing to draw while the panel is off.
        if (!ui.b_off &&
            (b_redraw || ((now_ms - ui.last_draw_ms) >= UI_REDRAW_MS)))
        {
            pages_draw(&ui.nav, &cfg, &status);
            ui.last_draw_ms = now_ms;
        }

#if OPENTHERMO_STATUS_LED
        // Bench build only: the on-board LED shows the outputs as a rhythm,
        // stepped by this tick (status_led.h).
        control_get(&status);
        status_led_update(
            status_led_pattern(status.hvac.b_fault, status.hvac.b_waiting,
                               status.applied.b_y1, status.applied.b_g,
                               status.applied.b_o, status.applied.b_w),
            now_ms);
#endif

        (void)settings_commit_due(now_ms);
        vTaskDelay(pdMS_TO_TICKS(UI_TICK_MS));
    }
}
