/**
 * @file  settings.c
 * @brief NVS storage for the settings record. See settings.h.
 */
#include "settings.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hvac_logic.h"
#include "nvs.h"
#include "settings_logic.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define LOG_TAG "settings"

#define NVS_NAMESPACE "thermo"
#define NVS_KEY       "cfg"

static SemaphoreHandle_t g_h_lock     = NULL;
static StaticSemaphore_t g_lock_buf   = { 0 };
static settings_t        g_cfg        = { 0 };
static bool              g_b_dirty    = false;
static uint64_t          g_changed_ms = 0U;

// Reads the blob into p_cfg, migrating an older version. Leaves p_cfg alone
// unless a whole record of a known version was read. *p_version receives
// the version that was stored.
static esp_err_t settings_load(settings_t * p_cfg, uint32_t * p_version)
{
    esp_err_t    err                      = ESP_FAIL;
    nvs_handle_t h_nvs                    = 0U;
    uint8_t      blob[sizeof(settings_t)] = { 0 };
    size_t       len                      = sizeof(blob);

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h_nvs);
    if (ESP_OK != err)
    {
        goto done; // first boot: the namespace does not exist yet
    }

    // A blob larger than this version's (a newer firmware's) fails here.
    err = nvs_get_blob(h_nvs, NVS_KEY, blob, &len);
    if (ESP_OK != err)
    {
        goto close_nvs;
    }

    *p_version = settings_logic_from_blob(blob, len, p_cfg);
    if (0U == *p_version)
    {
        // len is at most sizeof(blob) (18), so it fits unsigned.
        ESP_LOGW(LOG_TAG,
                 "stored record (%u bytes) is no known version: "
                 "defaults",
                 (unsigned)len);
        err = ESP_ERR_INVALID_VERSION;
    }

close_nvs:
    nvs_close(h_nvs);
done:
    return err;
}

static esp_err_t settings_store(settings_t const * p_cfg)
{
    esp_err_t    err   = ESP_FAIL;
    nvs_handle_t h_nvs = 0U;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h_nvs);
    if (ESP_OK != err)
    {
        goto done;
    }

    err = nvs_set_blob(h_nvs, NVS_KEY, p_cfg, sizeof(*p_cfg));
    if (ESP_OK != err)
    {
        goto close_nvs;
    }

    err = nvs_commit(h_nvs);

close_nvs:
    nvs_close(h_nvs);
done:
    return err;
}

esp_err_t settings_init(void)
{
    esp_err_t err     = ESP_OK;
    uint32_t  fixed   = 0U;
    uint32_t  version = 0U;

    g_h_lock = xSemaphoreCreateMutexStatic(&g_lock_buf);

    settings_logic_defaults(&g_cfg);
#if OPENTHERMO_SIM_SENSOR
    // Bench build: start in Auto so LEDs on the outputs show the logic
    // working with no D-pad wired. Nothing reaches flash unless a setting
    // is changed, so a later real build still starts from Off.
    g_cfg.mode = (uint8_t)HVAC_MODE_AUTO; // 3 fits
#endif
    if (ESP_OK != settings_load(&g_cfg, &version))
    {
        ESP_LOGI(LOG_TAG, "no usable stored settings: using defaults");
    }
    else if (SETTINGS_VERSION != version)
    {
        // Migrated: save it in the new layout at the first commit.
        ESP_LOGI(LOG_TAG, "settings v%" PRIu32 " migrated to v%u", version,
                 SETTINGS_VERSION);
        g_b_dirty    = true;
        g_changed_ms = 0U;
    }
    else
    {
        // Current version, read as is.
    }

    fixed = settings_logic_sanitise(&g_cfg);
    if (fixed > 0U)
    {
        ESP_LOGW(LOG_TAG, "%" PRIu32 " stored field(s) out of range: reset",
                 fixed);
    }

    ESP_LOGI(LOG_TAG, "mode %u fan %u heat %d cool %d (tenths F)", g_cfg.mode,
             g_cfg.fan, g_cfg.heat_sp_f10, g_cfg.cool_sp_f10);

    return err;
}

void settings_get(settings_t * p_out)
{
    if ((NULL == p_out) || (NULL == g_h_lock))
    {
        goto done;
    }

    (void)xSemaphoreTake(g_h_lock, portMAX_DELAY);
    *p_out = g_cfg;
    (void)xSemaphoreGive(g_h_lock);

done:
    return;
}

bool settings_update(settings_edit_fn_t p_fn, void * p_ctx, uint64_t now_ms)
{
    bool       b_changed = false;
    settings_t cfg       = { 0 };

    if ((NULL == p_fn) || (NULL == g_h_lock))
    {
        goto done;
    }

    (void)xSemaphoreTake(g_h_lock, portMAX_DELAY);
    cfg = g_cfg;
    if (p_fn(&cfg, p_ctx))
    {
        (void)settings_logic_sanitise(&cfg);
        b_changed = (0 != memcmp(&cfg, &g_cfg, sizeof(cfg)));
        if (b_changed)
        {
            g_cfg        = cfg;
            g_b_dirty    = true;
            g_changed_ms = now_ms;
        }
    }
    (void)xSemaphoreGive(g_h_lock);

done:
    return b_changed;
}

esp_err_t settings_commit_due(uint64_t now_ms)
{
    esp_err_t  err   = ESP_OK;
    settings_t cfg   = { 0 };
    bool       b_due = false;

    if (NULL == g_h_lock)
    {
        goto done;
    }

    (void)xSemaphoreTake(g_h_lock, portMAX_DELAY);
    b_due = (g_b_dirty && (now_ms >= g_changed_ms) &&
             ((now_ms - g_changed_ms) >= SETTINGS_COMMIT_DELAY_MS));
    if (b_due)
    {
        cfg       = g_cfg;
        g_b_dirty = false;
    }
    (void)xSemaphoreGive(g_h_lock);

    if (b_due)
    {
        err = settings_store(&cfg);
        if (ESP_OK != err)
        {
            ESP_LOGE(LOG_TAG, "save failed: 0x%x; retrying in %u s", err,
                     SETTINGS_RETRY_MS / 1000U);
            // Retry later, unless a newer change already re-armed the save
            // (that one carries this change too).
            (void)xSemaphoreTake(g_h_lock, portMAX_DELAY);
            if (!g_b_dirty)
            {
                g_b_dirty = true;
                g_changed_ms =
                    now_ms + SETTINGS_RETRY_MS - SETTINGS_COMMIT_DELAY_MS;
            }
            (void)xSemaphoreGive(g_h_lock);
        }
        else
        {
            ESP_LOGI(LOG_TAG, "saved");
        }
    }

done:
    return err;
}
