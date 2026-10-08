/**
 * @file  display_hal.h
 * @brief Minimal ESP-IDF HAL for U8g2: hardware I2C byte transport plus the
 *        delay/GPIO callback. Private to the display component.
 */
#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include "esp_err.h"
#include <stdint.h>
#include <u8g2.h>

/**
 * Registers the display as a device on the shared bus (i2c_bus_init() must
 * have run). Call once before u8g2_InitDisplay().
 */
esp_err_t display_hal_init(uint8_t i2c_addr_7bit);

/**
 * Pass these to the u8g2 setup function as byte_cb and gpio_and_delay_cb.
 */
uint8_t display_hal_i2c_byte_cb(u8x8_t * p_u8x8, uint8_t msg, uint8_t arg_int,
                                void * p_arg);
uint8_t display_hal_gpio_and_delay_cb(u8x8_t * p_u8x8, uint8_t msg,
                                      uint8_t arg_int, void * p_arg);

#endif /* DISPLAY_HAL_H */
