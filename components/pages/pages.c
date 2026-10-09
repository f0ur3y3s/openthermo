/**
 * @file  pages.c
 * @brief Page drawing on the 128x64 OLED. See pages.h.
 */
#include "pages.h"

#include "display.h"
#include "esp_app_desc.h"
#include "page_fmt.h"
#include "qrcodegen.h"
#include <stdio.h>
#include <string.h>

// A formatted number ("-3276.8", "71582788:59") fits NUM_LEN. A line fits
// TEXT_LEN even at the widest each %s and %lu could expand to, so GCC can
// prove no snprintf truncates; the panel itself shows 21 characters.
#define NUM_LEN  12U
#define TEXT_LEN 48U

// Layout, in pixels. Small-font baselines are 10 px apart.
#define ROW_TITLE_Y   8
#define ROW_FIRST_Y   19
#define ROW_PITCH     10
#define LIST_MARK_X   0
#define LIST_TEXT_X   12
#define BIG_TEMP_Y    44
#define BIG_UNIT_GAP  3
#define DEGREE_R      2
#define SETPOINT_X    86
#define SETPOINT_MK_X 78
#define SETPOINT_Y1   27
#define SETPOINT_Y2   41
// Inside DISPLAY_USABLE_W x DISPLAY_USABLE_H, so the burn-in creep never
// pushes them off the panel: four 29 px boxes end at x 122, row 61.
#define OUT_BOX_Y     51
#define OUT_BOX_W     29
#define OUT_BOX_H     11
#define OUT_BOX_PITCH 31
#define OUT_TEXT_Y    60
#define OUTPUT_COUNT  4U
#define INFO_LINES    6U
#define SETTINGS_ROWS 5U // rows that fit under the title

// Scrollbar down the right edge of a list that has more rows than fit: a
// 1 px track with a 3 px thumb whose length is the share of rows shown and
// whose position is how far the list has scrolled. It spans the rows (y 11
// to 60) and ends at the usable width, so the values beside it are
// right-aligned SCROLLBAR_SPACE px further in.
#define SCROLLBAR_W     3
#define SCROLLBAR_SPACE 5 // the bar plus a 2 px gap
#define SCROLLBAR_TOP   (ROW_FIRST_Y - 8)
#define SCROLLBAR_H     ((int32_t)SETTINGS_ROWS * ROW_PITCH)

// Factory reset prompt: the message on three rows, then No and Yes stacked
// on the last two (No first), so up and down move between them.
#define CONFIRM_NO_Y  (ROW_FIRST_Y + (3 * ROW_PITCH))
#define CONFIRM_YES_Y (ROW_FIRST_Y + (4 * ROW_PITCH))

// Pair page. A Matter QR payload (22 alphanumeric characters) is a version 1
// code, 21 modules; version 2 (25) is allowed for longer ones. Drawn 2 px a
// module with a 2-module quiet zone: at most 58 px, inside the usable 62.
// Lit background, dark modules, as printed codes are, for any scanner.
#define PAIR_QR_MAX_VERSION 2
#define PAIR_QR_SCALE       2
#define PAIR_QR_QUIET       2
#define PAIR_TEXT_X         62
#define SEC_PER_HOUR        3600U
#define SEC_PER_DAY         86400U
#define SEC_PER_MIN         60U
#define MS_PER_SEC          1000U

static char const * const g_mode_names[HVAC_MODE_COUNT] = {
    "Off", "Heat", "Cool", "Auto", "Em heat",
};

static char const * const g_fan_names[HVAC_FAN_COUNT] = {
    "Auto",
    "On",
};

static char const * const g_output_names[OUTPUT_COUNT] = {
    "Y1",
    "G",
    "O",
    "W",
};

static char const * const g_setting_names[PAGES_SET_COUNT] = {
    "Units",      "Calibrate", "Fan purge",      "Dim after",     "Screen off",
    "Brightness", "Info",      "Pair with Home", "Factory reset",
};

// The pair page's state (UI task only).
static bool    g_b_pair_known                       = false; // net reported
static bool    g_b_pair_net                         = false; // Matter running
static bool    g_b_paired                           = false;
static bool    g_b_qr_ok                            = false; // g_qr is valid
static char    g_pair_qr[PAGES_PAIR_QR_LEN]         = { 0 };
static char    g_pair_manual[PAGES_PAIR_MANUAL_LEN] = { 0 };
static uint8_t g_qr[qrcodegen_BUFFER_LEN_FOR_VERSION(PAIR_QR_MAX_VERSION)] = {
    0
};

static char const * pages_mode_name(uint8_t mode)
{
    return (mode < (uint8_t)HVAC_MODE_COUNT) ? g_mode_names[mode] : "?";
}

static bool pages_celsius(settings_t const * p_cfg)
{
    return ((uint8_t)SETTINGS_UNITS_C == p_cfg->units); // 1 fits
}

// Draws p_text so it ends at x = right.
static void pages_text_right_at(u8g2_t * p_u8g2, int32_t right, int32_t y,
                                char const * p_text)
{
    u8g2_uint_t width = u8g2_GetStrWidth(p_u8g2, p_text);

    // Text on this panel is at most 128 px wide, so both fit u8g2_uint_t.
    (void)u8g2_DrawStr(p_u8g2, (u8g2_uint_t)(right - (int32_t)width),
                       (u8g2_uint_t)y, p_text);
}

static void pages_text_right(u8g2_t * p_u8g2, int32_t y, char const * p_text)
{
    pages_text_right_at(p_u8g2, DISPLAY_USABLE_W, y, p_text);
}

// The main page's status text: what the system is doing right now.
static void pages_status_text(char * p_buf, control_status_t const * p_status)
{
    char wait[NUM_LEN] = { 0 };

    if (p_status->hvac.b_fault)
    {
        (void)snprintf(p_buf, TEXT_LEN, "SENSOR FAULT");
    }
    else if (p_status->hvac.b_waiting)
    {
        page_fmt_mmss(wait, NUM_LEN, p_status->hvac.wait_s);
        (void)snprintf(p_buf, TEXT_LEN, "Wait %s", wait);
    }
    else if (HVAC_CALL_HEAT == p_status->hvac.call)
    {
        (void)snprintf(p_buf, TEXT_LEN, "%s",
                       p_status->hvac.b_aux ? "Heat+aux" : "Heating");
    }
    else if (HVAC_CALL_COOL == p_status->hvac.call)
    {
        (void)snprintf(p_buf, TEXT_LEN, "Cooling");
    }
    else if (HVAC_CALL_EHEAT == p_status->hvac.call)
    {
        (void)snprintf(p_buf, TEXT_LEN, "Em heat");
    }
    else
    {
        (void)snprintf(p_buf, TEXT_LEN, "%s",
                       p_status->applied.b_g ? "Fan" : "Idle");
    }
}

// The four relay outputs as boxes along the bottom, filled when energised.
static void pages_outputs(u8g2_t * p_u8g2, hvac_outputs_t const * p_out)
{
    bool const  b_on[OUTPUT_COUNT] = { p_out->b_y1, p_out->b_g, p_out->b_o,
                                       p_out->b_w };
    uint32_t    out_idx            = 0U;
    u8g2_uint_t box_x              = 0U;
    u8g2_uint_t width              = 0U;

    for (out_idx = 0U; out_idx < OUTPUT_COUNT; out_idx++)
    {
        // At most 3 * 31 + 29 = 122 px: inside DISPLAY_USABLE_W.
        box_x = (u8g2_uint_t)(out_idx * OUT_BOX_PITCH);
        width = u8g2_GetStrWidth(p_u8g2, g_output_names[out_idx]);
        if (b_on[out_idx])
        {
            u8g2_DrawBox(p_u8g2, box_x, OUT_BOX_Y, OUT_BOX_W, OUT_BOX_H);
            u8g2_SetDrawColor(p_u8g2, 0U);
        }
        else
        {
            u8g2_DrawFrame(p_u8g2, box_x, OUT_BOX_Y, OUT_BOX_W, OUT_BOX_H);
        }
        // The label is narrower than its box, so the offset is positive.
        (void)u8g2_DrawStr(p_u8g2,
                           (u8g2_uint_t)(box_x + ((OUT_BOX_W - width) / 2U)),
                           OUT_TEXT_Y, g_output_names[out_idx]);
        u8g2_SetDrawColor(p_u8g2, 1U);
    }
}

// The setpoint column on the right of the main page.
static void pages_setpoints(u8g2_t * p_u8g2, pages_nav_t const * p_nav,
                            settings_t const * p_cfg)
{
    char    text[TEXT_LEN] = { 0 };
    char    sp[NUM_LEN]    = { 0 };
    bool    b_c            = pages_celsius(p_cfg);
    bool    b_cool         = pages_nav_cool_active(p_nav, p_cfg);
    int16_t sp_f10         = 0;

    if ((uint8_t)HVAC_MODE_AUTO == p_cfg->mode) // 3 fits
    {
        page_fmt_tenths(sp, NUM_LEN, page_fmt_temp(p_cfg->heat_sp_f10, b_c),
                        false);
        (void)snprintf(text, TEXT_LEN, "H %s", sp);
        (void)u8g2_DrawStr(p_u8g2, SETPOINT_X, SETPOINT_Y1, text);
        page_fmt_tenths(sp, NUM_LEN, page_fmt_temp(p_cfg->cool_sp_f10, b_c),
                        false);
        (void)snprintf(text, TEXT_LEN, "C %s", sp);
        (void)u8g2_DrawStr(p_u8g2, SETPOINT_X, SETPOINT_Y2, text);
        (void)u8g2_DrawStr(p_u8g2, SETPOINT_MK_X,
                           b_cool ? SETPOINT_Y2 : SETPOINT_Y1, ">");
    }
    else if ((uint8_t)HVAC_MODE_OFF == p_cfg->mode) // 0 fits
    {
        (void)u8g2_DrawStr(p_u8g2, SETPOINT_X, SETPOINT_Y1, "Set");
        (void)u8g2_DrawStr(p_u8g2, SETPOINT_X, SETPOINT_Y2, "--");
    }
    else
    {
        sp_f10 = p_cfg->heat_sp_f10;
        if (b_cool)
        {
            sp_f10 = p_cfg->cool_sp_f10;
        }
        page_fmt_tenths(sp, NUM_LEN, page_fmt_temp(sp_f10, b_c), false);
        (void)u8g2_DrawStr(p_u8g2, SETPOINT_X, SETPOINT_Y1, "Set");
        (void)u8g2_DrawStr(p_u8g2, SETPOINT_X, SETPOINT_Y2, sp);
    }
}

static void pages_main(u8g2_t * p_u8g2, pages_nav_t const * p_nav,
                       settings_t const *       p_cfg,
                       control_status_t const * p_status)
{
    char        text[TEXT_LEN] = { 0 };
    bool        b_c            = pages_celsius(p_cfg);
    u8g2_uint_t width          = 0U;

    // A bench build says so on the page that matters, so it can never pass
    // for a real thermostat.
    (void)snprintf(text, TEXT_LEN, "%s%s", pages_mode_name(p_cfg->mode),
                   p_status->sensor.b_simulated ? " SIM" : "");
    (void)u8g2_DrawStr(p_u8g2, 0U, ROW_TITLE_Y, text);
    pages_status_text(text, p_status);
    pages_text_right(p_u8g2, ROW_TITLE_Y, text);

    if (p_status->sensor.b_valid)
    {
        page_fmt_tenths(text, TEXT_LEN,
                        page_fmt_temp(p_status->sensor.temp_f10, b_c), false);
    }
    else
    {
        (void)snprintf(text, TEXT_LEN, "--.-");
    }
    u8g2_SetFont(p_u8g2, DISPLAY_FONT_BIG);
    width = u8g2_DrawStr(p_u8g2, 0U, BIG_TEMP_Y, text);
    u8g2_SetFont(p_u8g2, DISPLAY_FONT_SMALL);

    // A small degree ring and the unit letter after the number. The big
    // font's width is at most a few glyphs, well inside the panel.
    u8g2_DrawCircle(p_u8g2, (u8g2_uint_t)(width + BIG_UNIT_GAP + DEGREE_R),
                    (u8g2_uint_t)(BIG_TEMP_Y - 21), DEGREE_R, U8G2_DRAW_ALL);
    (void)u8g2_DrawStr(p_u8g2,
                       (u8g2_uint_t)(width + BIG_UNIT_GAP + (2 * DEGREE_R) + 2),
                       (u8g2_uint_t)(BIG_TEMP_Y - 14), b_c ? "C" : "F");

    pages_setpoints(p_u8g2, p_nav, p_cfg);
    pages_outputs(p_u8g2, &p_status->applied);
}

// A titled list with a cursor ('>') and the current value marked ('*').
static void pages_list(u8g2_t * p_u8g2, char const * p_title,
                       char const * const * pp_items, uint8_t count,
                       uint8_t cursor, uint8_t current)
{
    char     text[TEXT_LEN] = { 0 };
    uint32_t item_idx       = 0U;
    int32_t  row_y          = ROW_FIRST_Y;

    (void)u8g2_DrawStr(p_u8g2, 0U, ROW_TITLE_Y, p_title);
    for (item_idx = 0U; item_idx < count; item_idx++)
    {
        (void)snprintf(text, TEXT_LEN, "%c %s",
                       (item_idx == current) ? '*' : ' ', pp_items[item_idx]);
        // At most 19 + 4 * 10 = 59: on the panel.
        (void)u8g2_DrawStr(p_u8g2, LIST_TEXT_X, (u8g2_uint_t)row_y, text);
        if (item_idx == cursor)
        {
            (void)u8g2_DrawStr(p_u8g2, LIST_MARK_X, (u8g2_uint_t)row_y, ">");
        }
        row_y += ROW_PITCH;
    }
}

static void pages_setting_value(char * p_buf, settings_t const * p_cfg,
                                uint32_t item)
{
    char num[NUM_LEN] = { 0 };
    bool b_c          = pages_celsius(p_cfg);

    switch (item)
    {
        case PAGES_SET_UNITS:
            (void)snprintf(p_buf, TEXT_LEN, "%s", b_c ? "C" : "F");
            break;
        case PAGES_SET_CAL:
            page_fmt_tenths(num, NUM_LEN,
                            page_fmt_delta(p_cfg->cal_offset_f10, b_c), true);
            (void)snprintf(p_buf, TEXT_LEN, "%s%s", num, b_c ? "C" : "F");
            break;
        case PAGES_SET_PURGE:
            (void)snprintf(p_buf, TEXT_LEN, "%u s", p_cfg->fan_purge_s);
            break;
        case PAGES_SET_DIM:
            if (0U == p_cfg->display_timeout_s)
            {
                (void)snprintf(p_buf, TEXT_LEN, "never");
            }
            else
            {
                (void)snprintf(p_buf, TEXT_LEN, "%u s",
                               p_cfg->display_timeout_s);
            }
            break;
        case PAGES_SET_OFF:
            if (0U == p_cfg->screen_off_s)
            {
                (void)snprintf(p_buf, TEXT_LEN, "never");
            }
            else
            {
                (void)snprintf(p_buf, TEXT_LEN, "%u min",
                               p_cfg->screen_off_s / SEC_PER_MIN);
            }
            break;
        case PAGES_SET_INFO:
        case PAGES_SET_PAIR:
        case PAGES_SET_RESET:
            // Not values: centre opens a page, or the reset prompt.
            (void)snprintf(p_buf, TEXT_LEN, ">");
            break;
        case PAGES_SET_BRIGHT:
            (void)snprintf(p_buf, TEXT_LEN, "%u", p_cfg->brightness);
            break;
        default:
            p_buf[0] = '\0';
            break;
    }
}

// The scrollbar for a list showing rows first..first + shown - 1 of count;
// nothing when every row fits.
static void pages_scrollbar(u8g2_t * p_u8g2, uint32_t first, uint32_t shown,
                            uint32_t count)
{
    int32_t const x      = DISPLAY_USABLE_W - SCROLLBAR_W;
    int32_t       thumb  = 0;
    int32_t       offset = 0;

    if (count > shown)
    {
        // count, shown and first are small list sizes (under 10), so every
        // product and cast below is exact in int32_t, and all coordinates
        // stay inside the panel (x 122..124, y 11..60).
        thumb = (SCROLLBAR_H * (int32_t)shown) / (int32_t)count;
        offset =
            ((SCROLLBAR_H - thumb) * (int32_t)first) / (int32_t)(count - shown);
        u8g2_DrawVLine(p_u8g2, (u8g2_uint_t)(x + (SCROLLBAR_W / 2)),
                       SCROLLBAR_TOP, (u8g2_uint_t)SCROLLBAR_H);
        u8g2_DrawBox(p_u8g2, (u8g2_uint_t)x,
                     (u8g2_uint_t)(SCROLLBAR_TOP + offset), SCROLLBAR_W,
                     (u8g2_uint_t)thumb);
    }
}

static void pages_settings(u8g2_t * p_u8g2, pages_nav_t const * p_nav,
                           settings_t const * p_cfg)
{
    char        value[TEXT_LEN] = { 0 };
    uint32_t    item_idx        = 0U;
    uint32_t    first           = 0U;
    int32_t     row_y           = ROW_FIRST_Y;
    u8g2_uint_t width           = 0U;

    // More items than rows: scroll so the cursor's row is always shown.
    if (p_nav->cursor >= SETTINGS_ROWS)
    {
        first = (uint32_t)p_nav->cursor - SETTINGS_ROWS + 1U;
    }

    (void)u8g2_DrawStr(p_u8g2, 0U, ROW_TITLE_Y, "Settings");
    pages_scrollbar(p_u8g2, first, SETTINGS_ROWS, (uint32_t)PAGES_SET_COUNT);
    for (item_idx = first; (item_idx < (uint32_t)PAGES_SET_COUNT) &&
                           (item_idx < (first + SETTINGS_ROWS));
         item_idx++)
    {
        // Rows end at 19 + 4 * 10 = 59: on the panel.
        (void)u8g2_DrawStr(p_u8g2, LIST_TEXT_X, (u8g2_uint_t)row_y,
                           g_setting_names[item_idx]);
        pages_setting_value(value, p_cfg, item_idx);
        if ((item_idx == p_nav->cursor) && p_nav->b_editing)
        {
            // Value being edited: drawn inverted.
            width = u8g2_GetStrWidth(p_u8g2, value);
            u8g2_DrawBox(p_u8g2,
                         (u8g2_uint_t)(DISPLAY_USABLE_W - SCROLLBAR_SPACE -
                                       (int32_t)width - 2),
                         (u8g2_uint_t)(row_y - 8), (u8g2_uint_t)(width + 2U),
                         ROW_PITCH);
            u8g2_SetDrawColor(p_u8g2, 0U);
            pages_text_right_at(p_u8g2, DISPLAY_USABLE_W - SCROLLBAR_SPACE,
                                row_y, value);
            u8g2_SetDrawColor(p_u8g2, 1U);
        }
        else
        {
            pages_text_right_at(p_u8g2, DISPLAY_USABLE_W - SCROLLBAR_SPACE,
                                row_y, value);
        }
        if (item_idx == p_nav->cursor)
        {
            (void)u8g2_DrawStr(p_u8g2, LIST_MARK_X, (u8g2_uint_t)row_y, ">");
        }
        row_y += ROW_PITCH;
    }
}

static void pages_info(u8g2_t * p_u8g2, settings_t const * p_cfg,
                       control_status_t const * p_status)
{
    char lines[INFO_LINES][TEXT_LEN] = { { 0 } };
    char num_a[NUM_LEN]              = { 0 };
    char num_b[NUM_LEN]              = { 0 };
    bool b_c                         = pages_celsius(p_cfg);
    // Uptime in seconds fits uint32_t for 136 years.
    uint32_t up_s     = (uint32_t)(p_status->now_ms / MS_PER_SEC);
    uint32_t line_idx = 0U;

    (void)snprintf(lines[0], TEXT_LEN, "openthermo %s",
                   esp_app_get_description()->version);
    (void)snprintf(lines[1], TEXT_LEN, "Up %lud %02lu:%02lu:%02lu",
                   (unsigned long)(up_s / SEC_PER_DAY),
                   (unsigned long)((up_s % SEC_PER_DAY) / SEC_PER_HOUR),
                   (unsigned long)((up_s % SEC_PER_HOUR) / SEC_PER_MIN),
                   (unsigned long)(up_s % SEC_PER_MIN));
    page_fmt_tenths(num_a, NUM_LEN,
                    page_fmt_temp(p_status->sensor.raw_f10, b_c), false);
    page_fmt_tenths(num_b, NUM_LEN, p_status->sensor.rh_x10, false);
    (void)snprintf(lines[2], TEXT_LEN, "Raw %s RH %s%%", num_a, num_b);
    page_fmt_tenths(num_a, NUM_LEN,
                    page_fmt_delta(p_status->sensor.heat_f10, b_c), false);
    page_fmt_tenths(num_b, NUM_LEN, page_fmt_delta(p_cfg->cal_offset_f10, b_c),
                    true);
    (void)snprintf(lines[3], TEXT_LEN, "Self %s Cal %s", num_a, num_b);
    page_fmt_mmss(num_a, NUM_LEN, p_status->hvac.wait_s);
    (void)snprintf(lines[4], TEXT_LEN, "Y1 min-off %s", num_a);
    if (p_status->sensor.b_simulated)
    {
        (void)snprintf(lines[5], TEXT_LEN, "SIMULATED room");
    }
    else
    {
        (void)snprintf(lines[5], TEXT_LEN, "Sensor fails %lu",
                       (unsigned long)p_status->sensor.read_fails);
    }

    for (line_idx = 0U; line_idx < INFO_LINES; line_idx++)
    {
        // At most 8 + 5 * 10 = 58: on the panel.
        (void)u8g2_DrawStr(p_u8g2, 0U,
                           (u8g2_uint_t)(ROW_TITLE_Y + (line_idx * ROW_PITCH)),
                           lines[line_idx]);
    }
}

// The factory reset prompt, over the settings page. Starts on No.
static void pages_reset_prompt(u8g2_t * p_u8g2, pages_nav_t const * p_nav)
{
    int32_t const sel_y = p_nav->b_confirm_yes ? CONFIRM_YES_Y : CONFIRM_NO_Y;
    u8g2_uint_t   width = 0U;
    // Text over the selection box is drawn in colour 0 (cleared). Colours
    // are 0 or 1, so the casts drop nothing.
    uint8_t const no_color  = (uint8_t)(p_nav->b_confirm_yes ? 1U : 0U);
    uint8_t const yes_color = (uint8_t)(p_nav->b_confirm_yes ? 0U : 1U);

    (void)u8g2_DrawStr(p_u8g2, 0U, ROW_TITLE_Y, "Factory reset?");
    (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y, "Unpairs from Home and");
    (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y + ROW_PITCH,
                       "resets all settings,");
    (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y + (2 * ROW_PITCH),
                       "then restarts.");

    // The choices, as a list: a marker on the chosen one, which is also
    // drawn inverted. Rows end at 59: on the panel.
    (void)u8g2_DrawStr(p_u8g2, LIST_MARK_X, (u8g2_uint_t)sel_y, ">");
    width = u8g2_GetStrWidth(p_u8g2, p_nav->b_confirm_yes ? "Yes" : "No");
    u8g2_DrawBox(p_u8g2, (u8g2_uint_t)(LIST_TEXT_X - 2),
                 (u8g2_uint_t)(sel_y - 8), (u8g2_uint_t)(width + 4U),
                 ROW_PITCH);
    u8g2_SetDrawColor(p_u8g2, no_color);
    (void)u8g2_DrawStr(p_u8g2, LIST_TEXT_X, CONFIRM_NO_Y, "No");
    u8g2_SetDrawColor(p_u8g2, yes_color);
    (void)u8g2_DrawStr(p_u8g2, LIST_TEXT_X, CONFIRM_YES_Y, "Yes");
    u8g2_SetDrawColor(p_u8g2, 1U);
}

void pages_set_pairing(char const * p_qr, char const * p_manual, bool b_paired)
{
    uint8_t temp[qrcodegen_BUFFER_LEN_FOR_VERSION(PAIR_QR_MAX_VERSION)] = { 0 };

    g_b_pair_known = true;
    g_b_pair_net   = (NULL != p_qr);
    g_b_paired     = b_paired;
    if (NULL == p_qr)
    {
        goto done;
    }

    (void)snprintf(g_pair_manual, sizeof(g_pair_manual), "%s",
                   (NULL != p_manual) ? p_manual : "");
    if (0 != strncmp(g_pair_qr, p_qr, sizeof(g_pair_qr)))
    {
        // A new payload: encode it once, not on every frame.
        (void)snprintf(g_pair_qr, sizeof(g_pair_qr), "%s", p_qr);
        g_b_qr_ok = qrcodegen_encodeText(
            g_pair_qr, temp, g_qr, qrcodegen_Ecc_LOW, qrcodegen_VERSION_MIN,
            PAIR_QR_MAX_VERSION, qrcodegen_Mask_AUTO, true);
    }

done:
    return;
}

// The QR code: a lit square with the dark modules cleared out of it.
static void pages_qr(u8g2_t * p_u8g2)
{
    int32_t size = qrcodegen_getSize(g_qr);
    int32_t side = (size + (2 * PAIR_QR_QUIET)) * PAIR_QR_SCALE;
    int32_t mx   = 0;
    int32_t my   = 0;

    // side is at most (25 + 4) * 2 = 58: every coordinate fits u8g2_uint_t.
    u8g2_DrawBox(p_u8g2, 0U, 0U, (u8g2_uint_t)side, (u8g2_uint_t)side);
    u8g2_SetDrawColor(p_u8g2, 0U);
    for (my = 0; my < size; my++)
    {
        for (mx = 0; mx < size; mx++)
        {
            if (qrcodegen_getModule(g_qr, mx, my))
            {
                u8g2_DrawBox(
                    p_u8g2, (u8g2_uint_t)((mx + PAIR_QR_QUIET) * PAIR_QR_SCALE),
                    (u8g2_uint_t)((my + PAIR_QR_QUIET) * PAIR_QR_SCALE),
                    PAIR_QR_SCALE, PAIR_QR_SCALE);
            }
        }
    }
    u8g2_SetDrawColor(p_u8g2, 1U);
}

static void pages_pair(u8g2_t * p_u8g2)
{
    char    line[TEXT_LEN] = { 0 };
    size_t  len            = strlen(g_pair_manual);
    int32_t split          = 0;

    if (!g_b_pair_known || !g_b_pair_net)
    {
        (void)u8g2_DrawStr(p_u8g2, 0U, ROW_TITLE_Y, "Pair");
        (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y,
                           g_b_pair_known ? "Matter is off." : "Starting...");
    }
    else if (g_b_paired)
    {
        (void)u8g2_DrawStr(p_u8g2, 0U, ROW_TITLE_Y, "Pair");
        (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y, "Paired with Home.");
        (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y + ROW_PITCH,
                           "To pair again:");
        (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y + (2 * ROW_PITCH),
                           "Settings, Factory");
        (void)u8g2_DrawStr(p_u8g2, 0U, ROW_FIRST_Y + (3 * ROW_PITCH), "reset.");
    }
    else
    {
        if (g_b_qr_ok)
        {
            pages_qr(p_u8g2);
        }
        (void)u8g2_DrawStr(p_u8g2, PAIR_TEXT_X, ROW_TITLE_Y, "Scan in");
        (void)u8g2_DrawStr(p_u8g2, PAIR_TEXT_X, ROW_TITLE_Y + ROW_PITCH,
                           "Apple Home");
        (void)u8g2_DrawStr(p_u8g2, PAIR_TEXT_X, ROW_FIRST_Y + (2 * ROW_PITCH),
                           "or enter:");
        // The 11-digit manual code is too wide for one line beside the QR
        // code: first eight digits, then the rest.
        split = (len > 8U) ? 8 : (int32_t)len; // len <= 23, fits int32_t
        (void)snprintf(line, TEXT_LEN, "%.*s", (int)split, g_pair_manual);
        (void)u8g2_DrawStr(p_u8g2, PAIR_TEXT_X, ROW_FIRST_Y + (3 * ROW_PITCH),
                           line);
        (void)u8g2_DrawStr(p_u8g2, PAIR_TEXT_X, ROW_FIRST_Y + (4 * ROW_PITCH),
                           &g_pair_manual[split]);
    }
}

void pages_draw(pages_nav_t const * p_nav, settings_t const * p_cfg,
                control_status_t const * p_status)
{
    u8g2_t * p_u8g2 = display_u8g2();

    if ((NULL == p_nav) || (NULL == p_cfg) || (NULL == p_status))
    {
        goto done;
    }

    u8g2_ClearBuffer(p_u8g2);
    u8g2_SetFont(p_u8g2, DISPLAY_FONT_SMALL);
    u8g2_SetDrawColor(p_u8g2, 1U);

    switch (p_nav->page)
    {
        case PAGES_MODE:
            pages_list(p_u8g2, "Mode", g_mode_names, (uint8_t)HVAC_MODE_COUNT,
                       p_nav->cursor, p_cfg->mode);
            break;
        case PAGES_FAN:
            pages_list(p_u8g2, "Fan", g_fan_names, (uint8_t)HVAC_FAN_COUNT,
                       p_nav->cursor, p_cfg->fan);
            break;
        case PAGES_SETTINGS:
            if (p_nav->b_confirm)
            {
                pages_reset_prompt(p_u8g2, p_nav);
            }
            else
            {
                pages_settings(p_u8g2, p_nav, p_cfg);
            }
            break;
        case PAGES_INFO:
            pages_info(p_u8g2, p_cfg, p_status);
            break;
        case PAGES_PAIR:
            pages_pair(p_u8g2);
            break;
        case PAGES_MAIN:
        default:
            pages_main(p_u8g2, p_nav, p_cfg, p_status);
            break;
    }

    display_send(); // creeps the frame against burn-in, then sends it

done:
    return;
}
