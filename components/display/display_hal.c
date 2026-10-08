/**
 * @file  display_hal.c
 * @brief Minimal ESP-IDF HAL for U8g2. See display_hal.h.
 */
#include "display_hal.h"

#include "i2c_bus.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define LOG_TAG "display_hal"

#define I2C_TIMEOUT_MS 100
#define US_PER_MS      1000U
#define US_PER_10US    10U

// u8x8_cad_ssd13xx_i2c splits display data into 24-byte chunks and prefixes a
// control byte, so a single transaction never exceeds 25 bytes.
#define XFER_BUF_SIZE 32U

static i2c_master_dev_handle_t g_h_dev                   = NULL;
static uint8_t                 g_xfer_buf[XFER_BUF_SIZE] = { 0 };
static size_t                  g_xfer_len                = 0U;
static bool                    g_b_tx_failing            = false;

esp_err_t display_hal_init(uint8_t i2c_addr_7bit)
{
    return i2c_bus_add_device(i2c_addr_7bit, &g_h_dev);
}

uint8_t display_hal_i2c_byte_cb(u8x8_t * p_u8x8, uint8_t msg, uint8_t arg_int,
                                void * p_arg)
{
    uint8_t   result = 1U;
    esp_err_t err    = ESP_FAIL;

    (void)p_u8x8;

    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
            // The bus is already up; display_hal_init() did the work.
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            g_xfer_len = 0U;
            break;

        case U8X8_MSG_BYTE_SEND:
            if (NULL == p_arg)
            {
                ESP_LOGE(LOG_TAG, "BYTE_SEND with no payload");
                result = 0U;
            }
            else if ((g_xfer_len + arg_int) > XFER_BUF_SIZE)
            {
                ESP_LOGE(LOG_TAG,
                         "I2C transfer of %zu bytes overflows %u-byte buffer",
                         g_xfer_len + arg_int, XFER_BUF_SIZE);
                result = 0U;
            }
            else
            {
                memcpy(&g_xfer_buf[g_xfer_len], p_arg, arg_int);
                g_xfer_len += arg_int;
            }
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            if (g_xfer_len > 0U)
            {
                err = i2c_master_transmit(g_h_dev, g_xfer_buf, g_xfer_len,
                                          I2C_TIMEOUT_MS);
                g_xfer_len = 0U;
                if (ESP_OK != err)
                {
                    // A missing panel fails every chunk of every frame: say
                    // so once, and again only after it has recovered.
                    if (!g_b_tx_failing)
                    {
                        ESP_LOGE(LOG_TAG, "i2c_master_transmit: 0x%x", err);
                    }
                    g_b_tx_failing = true;
                    result         = 0U;
                }
                else
                {
                    g_b_tx_failing = false;
                }
            }
            break;

        case U8X8_MSG_BYTE_SET_DC:
            // Not used on I2C: the control byte carries command/data select.
            break;

        default:
            result = 0U;
            break;
    }

    return result;
}

uint8_t display_hal_gpio_and_delay_cb(u8x8_t * p_u8x8, uint8_t msg,
                                      uint8_t arg_int, void * p_arg)
{
    // Every message below is either serviced or harmless to ignore, so result
    // only ever stays 1. It is kept as a variable so that a future case can
    // fail without reintroducing an early return.
    uint8_t result = 1U;

    (void)p_u8x8;
    (void)p_arg;

    switch (msg)
    {
        case U8X8_MSG_DELAY_MILLI:
            // Short waits inside the init sequence are better spent busy than
            // rounded up to a whole tick by vTaskDelay().
            if (arg_int < portTICK_PERIOD_MS)
            {
                // Widening uint8_t to uint32_t is lossless; 255 ms fits.
                esp_rom_delay_us((uint32_t)arg_int * US_PER_MS);
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(arg_int));
            }
            break;

        case U8X8_MSG_DELAY_10MICRO:
            // Widening uint8_t to uint32_t is lossless; 2550 us fits.
            esp_rom_delay_us((uint32_t)arg_int * US_PER_10US);
            break;

        case U8X8_MSG_DELAY_100NANO:
            esp_rom_delay_us(1U);
            break;

        case U8X8_MSG_GPIO_AND_DELAY_INIT:
        case U8X8_MSG_GPIO_RESET:
        default:
            // Nothing to set up, no reset line on a bare I2C OLED module, and
            // every other GPIO message is harmless to ignore.
            break;
    }

    return result;
}
