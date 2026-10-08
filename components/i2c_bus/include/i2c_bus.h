/**
 * @file  i2c_bus.h
 * @brief The one I2C bus the OLED and the SHT40 share, on the pins named by
 *        board.h.
 *
 * ESP-IDF's i2c_master driver serialises transactions per bus, so the UI
 * task (display) and the control task (sensor) can each use their own device
 * handle without a lock of ours.
 */
#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Wiring check, before the bus exists: pulls SDA and SCL weakly to ground
 * and logs whether each reads high. A line that does is wired to a powered
 * module (its pull-up resistors win); one that reads low is open, or the
 * module has no power. Diagnostic only; call before i2c_bus_init().
 */
void i2c_bus_line_check(void);

/**
 * Creates the bus. Call once, before any device is added.
 */
esp_err_t i2c_bus_init(void);

/**
 * Probes every 7-bit address and logs what answers (the OLED is 0x3C, or
 * 0x3D on some modules; the SHT40 is 0x44), or that the bus is stuck, which
 * means a wiring fault. Diagnostic only; call after i2c_bus_init().
 */
void i2c_bus_scan(void);

/**
 * True if a device answers at addr_7bit. False before i2c_bus_init().
 */
bool i2c_bus_probe(uint8_t addr_7bit);

/**
 * Adds a 7-bit device at BOARD_I2C_HZ.
 *
 * @param addr_7bit device address, 0x08..0x77
 * @param p_h_dev   receives the device handle
 * @return ESP_ERR_INVALID_STATE if i2c_bus_init() has not run,
 *         ESP_ERR_INVALID_ARG for a bad address or a NULL p_h_dev
 */
esp_err_t i2c_bus_add_device(uint8_t                   addr_7bit,
                             i2c_master_dev_handle_t * p_h_dev);

#endif /* I2C_BUS_H */
