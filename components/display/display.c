/**
 * @file  display.c
 * @brief Panel ownership: bus bring-up, brightness, power, status text.
 */
#include "display.h"

#include "board.h"
#include "esp_log.h"
#include "display_hal.h"
#include "esp_timer.h"

#define LOG_TAG "display"

// Brightness thresholds where the pre-charge/VCOMH pair steps up.
#define BRIGHT_MID_MIN  86U
#define BRIGHT_HIGH_MIN 171U

// SSD1306 command bytes, sent through u8g2's "caca" (cmd, arg, cmd, arg).
#define CMD_SET_PRECHARGE 0x0d9U
#define CMD_SET_VCOMH     0x0dbU

#define PRECHARGE_DIM  0x11U // phase 2 = 1, dimmest
#define PRECHARGE_MID  0x41U
#define PRECHARGE_HIGH 0xf1U // phase 2 = 15
#define VCOMH_DIM      0x30U // 0.83 x Vcc, dimmest
#define VCOMH_MID      0x20U
#define VCOMH_HIGH     0x00U // 0.65 x Vcc

// Baselines for the three status lines.
#define STATUS_LINE1_Y 18
#define STATUS_LINE2_Y 34
#define STATUS_LINE3_Y 50

static u8g2_t g_u8g2        = { 0 };
static bool   g_b_panel_off = false;

void display_set_brightness(uint8_t level)
{
    uint8_t precharge = PRECHARGE_DIM;
    uint8_t vcomh     = VCOMH_DIM;

    // The contrast register (0x81) alone barely moves the needle on a number
    // of modules sold as SSD1306 - SSD1315 clones especially - so the
    // pre-charge period (0xD9) and the VCOMH deselect level (0xDB) are driven
    // from the same setting. A shorter phase-2 pre-charge and a higher VCOMH
    // both dim the panel, so stepping all three together gives a range you
    // can actually see.
    if (level >= BRIGHT_HIGH_MIN)
    {
        precharge = PRECHARGE_HIGH;
        vcomh     = VCOMH_HIGH;
    }
    else if (level >= BRIGHT_MID_MIN)
    {
        precharge = PRECHARGE_MID;
        vcomh     = VCOMH_MID;
    }
    else
    {
        // Leave the dim pair in place.
    }

    u8g2_SetContrast(&g_u8g2, level);
    u8g2_SendF(&g_u8g2, "caca", CMD_SET_PRECHARGE, precharge, CMD_SET_VCOMH,
               vcomh);

    ESP_LOGD(LOG_TAG,
             "brightness: contrast=0x%02x precharge=0x%02x vcomh=0x%02x", level,
             precharge, vcomh);
}

void display_status(char const * p_line1, char const * p_line2,
                    char const * p_line3)
{
    u8g2_ClearBuffer(&g_u8g2);
    u8g2_SetFont(&g_u8g2, DISPLAY_FONT_SMALL);
    (void)u8g2_DrawStr(&g_u8g2, 0, STATUS_LINE1_Y,
                       (NULL != p_line1) ? p_line1 : "");
    (void)u8g2_DrawStr(&g_u8g2, 0, STATUS_LINE2_Y,
                       (NULL != p_line2) ? p_line2 : "");
    (void)u8g2_DrawStr(&g_u8g2, 0, STATUS_LINE3_Y,
                       (NULL != p_line3) ? p_line3 : "");
    u8g2_SendBuffer(&g_u8g2);
}

void display_power(bool b_off)
{
    if (b_off != g_b_panel_off)
    {
        u8g2_SetPowerSave(&g_u8g2, b_off ? 1U : 0U);
        g_b_panel_off = b_off;
    }
}

void display_send(void)
{
    uint8_t dx = 0U;
    uint8_t dy = 0U;

    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    display_shift_offsets((uint64_t)(esp_timer_get_time() / 1000), &dx, &dy);
    // The full-frame buffer is u8g2's tile layout: 8 rows of 128 columns.
    display_shift_buffer(u8g2_GetBufferPtr(&g_u8g2), DISPLAY_WIDTH,
                         DISPLAY_HEIGHT / 8U, dx, dy);
    u8g2_SendBuffer(&g_u8g2);
}

u8g2_t * display_u8g2(void)
{
    return &g_u8g2;
}

esp_err_t display_init(uint8_t brightness)
{
    esp_err_t err = ESP_FAIL;

    err = display_hal_init(BOARD_OLED_ADDR);
    if (ESP_OK != err)
    {
        goto done;
    }

    BOARD_U8G2_SETUP_FN(&g_u8g2, U8G2_R0, display_hal_i2c_byte_cb,
                        display_hal_gpio_and_delay_cb);
    u8g2_InitDisplay(&g_u8g2);
    u8g2_SetPowerSave(&g_u8g2, 0U); // wake the panel
    g_b_panel_off = false;

    display_set_brightness(brightness);
    u8g2_ClearBuffer(&g_u8g2);
    u8g2_SendBuffer(&g_u8g2);

done:
    return err;
}
