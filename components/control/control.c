/**
 * @file  control.c
 * @brief The control task. See control.h.
 */
#include "control.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "relays.h"
#include "settings.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define LOG_TAG "control"

#define TASK_STACK    4096U
#define TASK_PRIORITY 5U // above the UI (main task, priority 1)
#define US_PER_MS     1000

// Run-time memory health, logged every HEALTH_PERIOD cycles (10 min) and
// once a minute after boot, when Matter has finished starting. The static
// size check (tools/check_size.py) cannot see the heap the radio stacks
// allocate; this is the number to watch on the C6.
#define HEALTH_FIRST_CYCLES  60U
#define HEALTH_PERIOD_CYCLES 600U
#define HEAP_WARN_BYTES      (24U * 1024U)
#define STACK_WARN_BYTES     512U

static SemaphoreHandle_t g_h_lock   = NULL;
static StaticSemaphore_t g_lock_buf = { 0 };
static control_status_t  g_status   = { 0 };
static hvac_state_t      g_hvac     = { 0 };
static uint64_t          g_hold_ms  = 0U; // all off until this time

static uint64_t control_now_ms(void)
{
    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    return (uint64_t)(esp_timer_get_time() / US_PER_MS);
}

static char const * control_call_name(hvac_call_t call)
{
    char const * p_name = "none";

    switch (call)
    {
        case HVAC_CALL_HEAT:
            p_name = "heat";
            break;
        case HVAC_CALL_COOL:
            p_name = "cool";
            break;
        case HVAC_CALL_EHEAT:
            p_name = "e-heat";
            break;
        default:
            // "none".
            break;
    }

    return p_name;
}

// One log line per change of the driven outputs, so the serial log is a
// complete record of every relay edge.
static void control_log_change(control_status_t const * p_old,
                               control_status_t const * p_new)
{
    hvac_outputs_t const * p_a = &p_old->applied;
    hvac_outputs_t const * p_b = &p_new->applied;

    if ((p_a->b_y1 != p_b->b_y1) || (p_a->b_g != p_b->b_g) ||
        (p_a->b_o != p_b->b_o) || (p_a->b_w != p_b->b_w) ||
        (p_old->hvac.call != p_new->hvac.call) ||
        (p_old->hvac.b_fault != p_new->hvac.b_fault))
    {
        ESP_LOGI(LOG_TAG, "Y1 %u G %u O %u W %u | call %s%s | %d tenths F",
                 p_b->b_y1, p_b->b_g, p_b->b_o, p_b->b_w,
                 control_call_name(p_new->hvac.call),
                 p_new->hvac.b_fault ? " FAULT" : "", p_new->sensor.temp_f10);
    }
}

static void control_cycle(void)
{
    uint64_t         now_ms  = control_now_ms();
    settings_t       cfg     = { 0 };
    hvac_input_t     in      = { 0 };
    relays_outputs_t req     = { 0 };
    relays_outputs_t applied = { 0 };
    control_status_t next    = { 0 };

    settings_get(&cfg);
    sensor_poll(now_ms, relays_energised_count(), cfg.cal_offset_f10);
    sensor_get(&next.sensor);

    // settings sanitises mode and fan to below their COUNTs, which are
    // valid enumerators; hvac_step range-checks them again regardless.
    in.mode         = (hvac_mode_t)cfg.mode;
    in.fan          = (hvac_fan_t)cfg.fan;
    in.heat_sp_f10  = cfg.heat_sp_f10;
    in.cool_sp_f10  = cfg.cool_sp_f10;
    in.fan_purge_s  = cfg.fan_purge_s;
    in.temp_f10     = next.sensor.temp_f10;
    in.b_temp_valid = next.sensor.b_valid;
    in.now_ms       = now_ms;

    // What the guard really drove last cycle: the logic takes it as the
    // truth, so a refused start is not timed as a run (hvac_logic.h).
    relays_applied(&applied);
    in.b_applied_valid = true;
    in.applied.b_y1    = applied.b_y1;
    in.applied.b_g     = applied.b_g;
    in.applied.b_o     = applied.b_o;
    in.applied.b_w     = applied.b_w;
    hvac_step(&g_hvac, &in, &next.hvac);

    req.b_y1 = next.hvac.out.b_y1;
    req.b_g  = next.hvac.out.b_g;
    req.b_o  = next.hvac.out.b_o;
    req.b_w  = next.hvac.out.b_w;
    // NULL drives everything off: the boot hold-off after a crash loop.
    relays_apply((now_ms < g_hold_ms) ? NULL : &req);
    relays_applied(&applied);

    next.applied.b_y1 = applied.b_y1;
    next.applied.b_g  = applied.b_g;
    next.applied.b_o  = applied.b_o;
    next.applied.b_w  = applied.b_w;
    next.now_ms       = now_ms;

    control_log_change(&g_status, &next);

    (void)xSemaphoreTake(g_h_lock, portMAX_DELAY);
    g_status = next;
    (void)xSemaphoreGive(g_h_lock);
}

// Logs free heap, its lowest point since boot, and this task's unused
// stack; warns when either runs low.
static void control_log_health(void)
{
    uint32_t free_now = (uint32_t)esp_get_free_heap_size();
    uint32_t free_min = (uint32_t)esp_get_minimum_free_heap_size();
    // ESP-IDF reports the high-water mark in bytes; it fits 32 bits.
    uint32_t stack_left = (uint32_t)uxTaskGetStackHighWaterMark(NULL);

    if ((free_min < HEAP_WARN_BYTES) || (stack_left < STACK_WARN_BYTES))
    {
        ESP_LOGW(LOG_TAG,
                 "LOW MEMORY: heap %" PRIu32 " B (low %" PRIu32
                 " B), control stack %" PRIu32 " B unused",
                 free_now, free_min, stack_left);
    }
    else
    {
        ESP_LOGI(LOG_TAG,
                 "health: heap %" PRIu32 " B (low %" PRIu32
                 " B), control stack %" PRIu32 " B unused",
                 free_now, free_min, stack_left);
    }
}

static void control_task(void * p_arg)
{
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t   cycles    = 0U;

    (void)p_arg;

    for (;;)
    {
        control_cycle();
        (void)esp_task_wdt_reset();
        cycles++;
        if ((HEALTH_FIRST_CYCLES == cycles) ||
            (0U == (cycles % HEALTH_PERIOD_CYCLES)))
        {
            control_log_health();
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    }
}

esp_err_t control_start(uint32_t hold_off_ms)
{
    esp_err_t    err    = ESP_OK;
    TaskHandle_t h_task = NULL;

    g_h_lock = xSemaphoreCreateMutexStatic(&g_lock_buf);
    hvac_init(&g_hvac, control_now_ms());
    g_hold_ms = control_now_ms() + hold_off_ms;

    if (pdPASS != xTaskCreate(control_task, "control", TASK_STACK, NULL,
                              TASK_PRIORITY, &h_task))
    {
        err = ESP_ERR_NO_MEM;
        goto done;
    }

    // Watched from its first cycle; a loop that stops cycling resets the
    // chip, which drops every relay. Not being watched is not an option.
    err = esp_task_wdt_add(h_task);
    if (ESP_OK != err)
    {
        ESP_LOGE(LOG_TAG, "task watchdog subscription failed: 0x%x", err);
    }

done:
    return err;
}

void control_get(control_status_t * p_out)
{
    if ((NULL == p_out) || (NULL == g_h_lock))
    {
        goto done;
    }

    (void)xSemaphoreTake(g_h_lock, portMAX_DELAY);
    *p_out = g_status;
    (void)xSemaphoreGive(g_h_lock);

done:
    return;
}
