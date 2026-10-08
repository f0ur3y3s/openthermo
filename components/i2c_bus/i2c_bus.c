/**
 * @file  i2c_bus.c
 * @brief The shared I2C bus. See i2c_bus.h.
 */
#include "i2c_bus.h"

#include "board.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include <inttypes.h>
#include <stddef.h>

#define LOG_TAG "i2c_bus"

#define I2C_GLITCH_COUNT 7U
#define I2C_ADDR_MIN     0x08U
#define I2C_ADDR_MAX     0x77U
#define PROBE_TIMEOUT_MS 20

#define LINE_SETTLE_US 200U // let the weak pull-down win against nothing

static i2c_master_bus_handle_t g_h_bus = NULL;

void i2c_bus_line_check(void)
{
    gpio_config_t const cfg = {
        .pin_bit_mask = (1ULL << BOARD_PIN_SDA) | (1ULL << BOARD_PIN_SCL),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    int32_t sda = 0;
    int32_t scl = 0;

    (void)gpio_config(&cfg);
    esp_rom_delay_us(LINE_SETTLE_US);
    sda = gpio_get_level(BOARD_PIN_SDA);
    scl = gpio_get_level(BOARD_PIN_SCL);
    (void)gpio_reset_pin(BOARD_PIN_SDA);
    (void)gpio_reset_pin(BOARD_PIN_SCL);

    ESP_LOGI(LOG_TAG, "wiring: SDA (GPIO%d) %s, SCL (GPIO%d) %s",
             (int)BOARD_PIN_SDA,
             (0 != sda) ? "pulled up: connected" : "low: open or unpowered",
             (int)BOARD_PIN_SCL,
             (0 != scl) ? "pulled up: connected" : "low: open or unpowered");
}

esp_err_t i2c_bus_init(void)
{
    esp_err_t               err     = ESP_FAIL;
    i2c_master_bus_config_t bus_cfg = { 0 };

    bus_cfg.i2c_port                     = I2C_NUM_0;
    bus_cfg.sda_io_num                   = BOARD_PIN_SDA;
    bus_cfg.scl_io_num                   = BOARD_PIN_SCL;
    bus_cfg.clk_source                   = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt            = I2C_GLITCH_COUNT;
    bus_cfg.flags.enable_internal_pullup = true;

    err = i2c_new_master_bus(&bus_cfg, &g_h_bus);
    if (ESP_OK != err)
    {
        ESP_LOGE(LOG_TAG, "i2c_new_master_bus: 0x%x", err);
    }

    return err;
}

esp_err_t i2c_bus_add_device(uint8_t                   addr_7bit,
                             i2c_master_dev_handle_t * p_h_dev)
{
    esp_err_t           err     = ESP_ERR_INVALID_ARG;
    i2c_device_config_t dev_cfg = { 0 };

    if (NULL == g_h_bus)
    {
        err = ESP_ERR_INVALID_STATE;
        goto done;
    }
    if ((NULL == p_h_dev) || (addr_7bit < I2C_ADDR_MIN) ||
        (addr_7bit > I2C_ADDR_MAX))
    {
        goto done;
    }

    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = addr_7bit;
    dev_cfg.scl_speed_hz    = BOARD_I2C_HZ;

    err = i2c_master_bus_add_device(g_h_bus, &dev_cfg, p_h_dev);
    if (ESP_OK != err)
    {
        ESP_LOGE(LOG_TAG, "add device 0x%02x: 0x%x", addr_7bit, err);
    }

done:
    return err;
}

bool i2c_bus_probe(uint8_t addr_7bit)
{
    return ((NULL != g_h_bus) &&
            (ESP_OK == i2c_master_probe(g_h_bus, addr_7bit, PROBE_TIMEOUT_MS)));
}

void i2c_bus_scan(void)
{
    esp_err_t err   = ESP_FAIL;
    uint32_t  addr  = 0U;
    uint32_t  found = 0U;

    if (NULL == g_h_bus)
    {
        goto done;
    }

    for (addr = I2C_ADDR_MIN; addr <= I2C_ADDR_MAX; addr++)
    {
        // addr runs 0x08..0x77, inside uint16_t.
        err = i2c_master_probe(g_h_bus, (uint16_t)addr, PROBE_TIMEOUT_MS);
        if (ESP_OK == err)
        {
            ESP_LOGI(LOG_TAG, "found device at 0x%02" PRIx32, addr);
            found++;
        }
        else if (ESP_ERR_TIMEOUT == err)
        {
            // SDA or SCL held low: no address can answer.
            ESP_LOGE(LOG_TAG, "bus stuck (SDA or SCL held low): check wiring");
            goto done;
        }
        else
        {
            // No answer at this address.
        }
    }

    if (0U == found)
    {
        ESP_LOGW(LOG_TAG, "no devices answered: check power and SDA/SCL");
    }

done:
    return;
}
