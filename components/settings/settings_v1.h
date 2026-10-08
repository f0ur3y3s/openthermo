/**
 * @file  settings_v1.h
 * @brief The version 1 settings record, frozen: the on-flash layout written
 *        by firmware before Oct 2026 (rule 10). Read only by the migration
 *        in settings_logic.c; never change it.
 */
#ifndef SETTINGS_V1_H
#define SETTINGS_V1_H

#include <stdint.h>

#define SETTINGS_V1_VERSION 1U

typedef struct
{
    uint16_t version;
    int16_t  heat_sp_f10;
    int16_t  cool_sp_f10;
    int16_t  cal_offset_f10;
    uint16_t fan_purge_s;
    uint16_t display_timeout_s;
    uint8_t  mode;
    uint8_t  fan;
    uint8_t  units;
    uint8_t  brightness;
} settings_v1_t;

#endif /* SETTINGS_V1_H */
