/**
 * @file  sensor.c
 * @brief SHT40 driver glue around sensor_logic.c. See sensor.h.
 */
#include "sensor.h"

#include "board.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "sensor_logic.h"
#if OPENTHERMO_SIM_SENSOR
#include "relays.h"
#include "sensor_sim.h"
#endif
#include <inttypes.h>
#include <stddef.h>

#define LOG_TAG "sensor"

#define CMD_MEASURE_HIGH 0xFDU // high precision, no heater; 8.3 ms max
#define CMD_SOFT_RESET   0x94U // 1 ms max
#define I2C_TIMEOUT_MS   50
#define MEASURE_WAIT_MS  20U // rounded up to whole ticks, so >= 10 ms
#define RESET_WAIT_MS    10U
#define US_PER_MS        1000

static sensor_logic_t   g_logic       = { 0 };
static sensor_reading_t g_reading     = { 0 };
static bool             g_b_reported  = false; // logged a state yet
static bool             g_b_was_valid = false; // the state logged
#if OPENTHERMO_SIM_SENSOR
static sensor_sim_t g_sim = { 0 };
#endif

#if OPENTHERMO_SIM_SENSOR

esp_err_t sensor_init(void)
{
    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    uint64_t now_ms = (uint64_t)(esp_timer_get_time() / US_PER_MS);

    sensor_logic_init(&g_logic, now_ms);
    sensor_sim_init(&g_sim, now_ms);
    g_reading.b_simulated = true;
    ESP_LOGW(LOG_TAG, "SIMULATED SENSOR: bench build only, never on the wall");

    // A real SHT40 on the bus means this board is a real thermostat: the
    // simulated room must never drive its HVAC. Relays stay off until the
    // real firmware is flashed.
    if (i2c_bus_probe(BOARD_SHT40_ADDR))
    {
        relays_inhibit();
        ESP_LOGE(LOG_TAG, "SHT40 found: this is a real thermostat. The bench "
                          "build will not drive its relays; flash the real "
                          "firmware.");
    }

    return ESP_OK;
}

// The simulated room, pushed by what the relays are driving now.
static bool sensor_measure(int16_t * p_temp_f10, int16_t * p_rh_x10)
{
    relays_outputs_t out = { 0 };
    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    uint64_t now_ms = (uint64_t)(esp_timer_get_time() / US_PER_MS);

    relays_applied(&out);
    *p_temp_f10 = sensor_sim_step(&g_sim, out.b_y1, out.b_o, out.b_w, now_ms);
    *p_rh_x10   = SENSOR_SIM_RH_X10;

    return true;
}

#else

static i2c_master_dev_handle_t g_h_dev = NULL;

esp_err_t sensor_init(void)
{
    esp_err_t     err = ESP_FAIL;
    uint8_t const cmd = CMD_SOFT_RESET;

    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    sensor_logic_init(&g_logic, (uint64_t)(esp_timer_get_time() / US_PER_MS));

    err = i2c_bus_add_device(BOARD_SHT40_ADDR, &g_h_dev);
    if (ESP_OK != err)
    {
        goto done;
    }

    // A missing or stuck sensor must not stop start-up: the 2 min fault rule
    // turns everything off and the display says why.
    if (ESP_OK != i2c_master_transmit(g_h_dev, &cmd, 1U, I2C_TIMEOUT_MS))
    {
        ESP_LOGW(LOG_TAG, "no answer at 0x%02x", BOARD_SHT40_ADDR);
    }
    vTaskDelay(pdMS_TO_TICKS(RESET_WAIT_MS));

done:
    return err;
}

// One measurement; true and the decoded values if it worked.
static bool sensor_measure(int16_t * p_temp_f10, int16_t * p_rh_x10)
{
    bool          b_ok                    = false;
    esp_err_t     err                     = ESP_FAIL;
    uint8_t const cmd                     = CMD_MEASURE_HIGH;
    uint8_t       frame[SENSOR_FRAME_LEN] = { 0 };

    if (NULL == g_h_dev)
    {
        goto done;
    }

    err = i2c_master_transmit(g_h_dev, &cmd, 1U, I2C_TIMEOUT_MS);
    if (ESP_OK != err)
    {
        goto done;
    }

    vTaskDelay(pdMS_TO_TICKS(MEASURE_WAIT_MS));

    err = i2c_master_receive(g_h_dev, frame, sizeof(frame), I2C_TIMEOUT_MS);
    if (ESP_OK != err)
    {
        goto done;
    }

    b_ok = sensor_decode(frame, p_temp_f10, p_rh_x10);

done:
    return b_ok;
}

#endif /* OPENTHERMO_SIM_SENSOR */

void sensor_poll(uint64_t now_ms, uint8_t relays_on, int16_t cal_f10)
{
    int16_t temp_f10 = 0;
    int16_t rh_x10   = 0;
    bool    b_valid  = false;

    if (sensor_measure(&temp_f10, &rh_x10))
    {
        sensor_logic_reading(&g_logic, temp_f10, rh_x10, now_ms);
    }
    else
    {
        g_reading.read_fails++;
    }

    sensor_logic_self_heat(&g_logic, relays_on, SENSOR_SELF_HEAT_PER_RELAY_F10,
                           now_ms);

    b_valid = sensor_logic_valid(&g_logic, now_ms);
    if (!g_b_reported || (b_valid != g_b_was_valid))
    {
        // Log each change of state once, not every second.
        g_b_reported  = true;
        g_b_was_valid = b_valid;
        if (b_valid)
        {
            ESP_LOGI(LOG_TAG, "reading valid");
        }
        else
        {
            ESP_LOGE(LOG_TAG, "FAULT: no valid reading (%" PRIu32 " fails)",
                     g_reading.read_fails);
        }
    }

    g_reading.b_valid  = b_valid;
    g_reading.temp_f10 = sensor_logic_room_f10(&g_logic, cal_f10);
    g_reading.raw_f10  = g_logic.raw_f10;
    g_reading.heat_f10 = sensor_logic_heat_f10(&g_logic);
    g_reading.rh_x10   = g_logic.rh_x10;
}

void sensor_get(sensor_reading_t * p_out)
{
    if (NULL != p_out)
    {
        *p_out = g_reading;
    }
}
