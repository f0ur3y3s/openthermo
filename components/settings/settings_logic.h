/**
 * @file  settings_logic.h
 * @brief Pure settings logic: defaults, range checks, and reading a stored
 *        record of this or an older version. Private to the settings
 *        component; the host tests compile it directly.
 */
#ifndef SETTINGS_LOGIC_H
#define SETTINGS_LOGIC_H

#include "settings_types.h"
#include <stddef.h>

/**
 * Fills p_cfg with the factory defaults. The default mode is Off, so a fresh
 * board never heats or cools until someone picks a mode.
 */
void settings_logic_defaults(settings_t * p_cfg);

/**
 * Resets every out-of-range field to its default (both setpoints together if
 * the pair breaks the auto deadband) and stamps the current version.
 *
 * @return how many fields were reset; 0 if p_cfg was already valid or NULL
 */
uint32_t settings_logic_sanitise(settings_t * p_cfg);

/**
 * Reads a record as stored in NVS. The current version is copied as is; a
 * version 1 record (settings_v1.h) is migrated, every v1 field kept and the
 * fields added since set to their defaults. The result is not range-checked:
 * call settings_logic_sanitise() next.
 *
 * @param p_blob the stored bytes
 * @param len    how many there are
 * @param p_out  receives the record; untouched when 0 is returned
 * @return the version that was read (SETTINGS_VERSION or an older one), or 0
 *         if the blob is no version this firmware knows, or a NULL argument
 */
uint32_t settings_logic_from_blob(void const * p_blob, size_t len,
                                  settings_t * p_out);

#endif /* SETTINGS_LOGIC_H */
