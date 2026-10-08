/**
 * @file  settings.h
 * @brief Persisted settings in NVS: mode, setpoints, fan, units, calibration
 *        and display. The record itself is in settings_types.h.
 *
 * The settings module owns one copy in RAM behind a mutex. Every task reads
 * it as a snapshot with settings_get() (rule 9), never field by field.
 * Every change is one settings_update(): the read, the edit and the write
 * happen under the mutex, so the D-pad (UI task) and Matter (CHIP task)
 * cannot lose each other's changes. There is deliberately no plain setter.
 * Changes go to RAM at once and reach flash after SETTINGS_COMMIT_DELAY_MS
 * without further changes, so holding a key to walk a setpoint costs one
 * flash write, not one per step. A failed flash write is retried after
 * SETTINGS_RETRY_MS.
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include "esp_err.h"
#include "settings_types.h"
#include <stdbool.h>
#include <stdint.h>

#define SETTINGS_COMMIT_DELAY_MS 3000U
#define SETTINGS_RETRY_MS        60000U

/**
 * An edit for settings_update(): changes *p_cfg in place and returns true if
 * it may have changed anything. It runs with the settings mutex held, so it
 * must be short and must not block, log, or take any other lock.
 */
typedef bool (*settings_edit_fn_t)(settings_t * p_cfg, void * p_ctx);

/**
 * Loads the record from NVS (nvs_flash_init() must have run). A missing,
 * wrong-sized or wrong-version record gives the defaults; out-of-range
 * fields are reset to their defaults. Both cases are logged.
 */
esp_err_t settings_init(void);

/**
 * Copies out a consistent snapshot. Safe from any task.
 */
void settings_get(settings_t * p_out);

/**
 * Applies p_fn to the current record under the mutex, range-checks the
 * result, makes it current and schedules the flash write if anything
 * changed. Safe from any task.
 *
 * @param p_fn   the edit; NULL does nothing
 * @param p_ctx  passed to p_fn
 * @param now_ms monotonic time, for the commit delay
 * @return true if the record changed
 */
bool settings_update(settings_edit_fn_t p_fn, void * p_ctx, uint64_t now_ms);

/**
 * Writes the record to flash if it changed and has been quiet for
 * SETTINGS_COMMIT_DELAY_MS. Call it from the UI loop.
 *
 * @return ESP_OK if nothing was due or the write worked
 */
esp_err_t settings_commit_due(uint64_t now_ms);

#endif /* SETTINGS_H */
