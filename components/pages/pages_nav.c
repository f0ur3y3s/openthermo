/**
 * @file  pages_nav.c
 * @brief Pure UI logic. See pages_nav.h.
 */
#include "pages_nav.h"

#include "hvac_logic.h"
#include <stddef.h>

static int32_t nav_clamp(int32_t value, int32_t min, int32_t max)
{
    int32_t clamped = value;

    if (clamped < min)
    {
        clamped = min;
    }
    else if (clamped > max)
    {
        clamped = max;
    }
    else
    {
        // In range.
    }

    return clamped;
}

static bool nav_is_step(button_event_t event)
{
    return ((BUTTON_EVENT_PRESS == event) || (BUTTON_EVENT_REPEAT == event));
}

// Shows `page` with its cursor on the current value.
static void nav_enter(pages_nav_t * p_nav, settings_t const * p_cfg,
                      pages_id_t page)
{
    p_nav->page      = page;
    p_nav->b_editing = false;

    switch (page)
    {
        case PAGES_MODE:
            p_nav->cursor = p_cfg->mode;
            break;
        case PAGES_FAN:
            p_nav->cursor = p_cfg->fan;
            break;
        default:
            p_nav->cursor = 0U;
            break;
    }
}

static bool nav_main(pages_nav_t * p_nav, settings_t * p_cfg, buttons_key_t key,
                     button_event_t event)
{
    bool    b_changed    = false;
    bool    b_heat_leads = true;
    int32_t step         = PAGES_STEP_SP_F10;
    int16_t heat         = p_cfg->heat_sp_f10;
    int16_t cool         = p_cfg->cool_sp_f10;

    if ((BUTTONS_KEY_CENTER == key) && (BUTTON_EVENT_PRESS == event) &&
        ((uint8_t)HVAC_MODE_AUTO == p_cfg->mode)) // 3 fits
    {
        p_nav->b_cool_selected = !p_nav->b_cool_selected;
        goto done;
    }
    if (((BUTTONS_KEY_UP != key) && (BUTTONS_KEY_DOWN != key)) ||
        ((uint8_t)HVAC_MODE_OFF == p_cfg->mode)) // 0 fits
    {
        goto done;
    }

    if ((uint8_t)SETTINGS_UNITS_C == p_cfg->units) // 1 fits
    {
        step = PAGES_STEP_SP_C_F10;
    }
    if (BUTTONS_KEY_DOWN == key)
    {
        step = -step;
    }

    // Both setpoints are inside their limits (settings sanitises them), so
    // one step either way stays well inside int16_t.
    if (pages_nav_cool_active(p_nav, p_cfg))
    {
        cool         = (int16_t)(cool + step);
        b_heat_leads = false;
    }
    else
    {
        heat = (int16_t)(heat + step);
    }
    hvac_setpoints_apply(&heat, &cool, b_heat_leads);

    b_changed = ((heat != p_cfg->heat_sp_f10) || (cool != p_cfg->cool_sp_f10));
    p_cfg->heat_sp_f10 = heat;
    p_cfg->cool_sp_f10 = cool;

done:
    return b_changed;
}

// Mode and fan pages: pick one of `count` values for *p_field.
static bool nav_list(pages_nav_t * p_nav, settings_t const * p_cfg,
                     buttons_key_t key, button_event_t event, uint8_t count,
                     uint8_t * p_field)
{
    bool b_changed = false;

    if ((BUTTONS_KEY_UP == key) && (p_nav->cursor > 0U))
    {
        p_nav->cursor--;
    }
    else if ((BUTTONS_KEY_DOWN == key) && ((p_nav->cursor + 1U) < count))
    {
        p_nav->cursor++;
    }
    else if ((BUTTONS_KEY_CENTER == key) && (BUTTON_EVENT_PRESS == event))
    {
        b_changed = (*p_field != p_nav->cursor);
        *p_field  = p_nav->cursor;
        nav_enter(p_nav, p_cfg, PAGES_MAIN);
    }
    else
    {
        // At the end of the list, or a key this page does not use.
    }

    return b_changed;
}

// Changes one setting by one step in direction `dir` (+1 or -1).
static bool nav_adjust(settings_t * p_cfg, uint8_t item, int32_t dir,
                       bool b_repeat)
{
    settings_t before = *p_cfg;

    // Each value below is clamped to a range inside its field's type before
    // the cast, so every narrowing is lossless.
    switch (item)
    {
        case PAGES_SET_UNITS:
            if (!b_repeat)
            {
                p_cfg->units = ((uint8_t)SETTINGS_UNITS_F == p_cfg->units)
                                   ? (uint8_t)SETTINGS_UNITS_C
                                   : (uint8_t)SETTINGS_UNITS_F;
            }
            break;
        case PAGES_SET_CAL:
            p_cfg->cal_offset_f10 = (int16_t)nav_clamp(
                p_cfg->cal_offset_f10 + (dir * PAGES_STEP_CAL_F10),
                -SETTINGS_CAL_LIMIT_F10, SETTINGS_CAL_LIMIT_F10);
            break;
        case PAGES_SET_PURGE:
            p_cfg->fan_purge_s = (uint16_t)nav_clamp(
                p_cfg->fan_purge_s + (dir * PAGES_STEP_PURGE_S), 0,
                HVAC_FAN_PURGE_MAX_S);
            break;
        case PAGES_SET_DIM:
            p_cfg->display_timeout_s = (uint16_t)nav_clamp(
                p_cfg->display_timeout_s + (dir * PAGES_STEP_DIM_S), 0,
                PAGES_DIM_MAX_S);
            break;
        case PAGES_SET_OFF:
            p_cfg->screen_off_s = (uint16_t)nav_clamp(
                p_cfg->screen_off_s + (dir * PAGES_STEP_OFF_S), 0,
                SETTINGS_SCREEN_OFF_MAX);
            break;
        case PAGES_SET_BRIGHT:
            p_cfg->brightness = (uint8_t)nav_clamp(
                p_cfg->brightness + (dir * PAGES_STEP_BRIGHT),
                SETTINGS_BRIGHTNESS_MIN, UINT8_MAX);
            break;
        default:
            // No such item.
            break;
    }

    return ((before.units != p_cfg->units) ||
            (before.cal_offset_f10 != p_cfg->cal_offset_f10) ||
            (before.fan_purge_s != p_cfg->fan_purge_s) ||
            (before.display_timeout_s != p_cfg->display_timeout_s) ||
            (before.screen_off_s != p_cfg->screen_off_s) ||
            (before.brightness != p_cfg->brightness));
}

static bool nav_settings(pages_nav_t * p_nav, settings_t * p_cfg,
                         buttons_key_t key, button_event_t event)
{
    bool    b_changed = false;
    int32_t dir       = (BUTTONS_KEY_UP == key) ? 1 : -1;

    if ((BUTTONS_KEY_CENTER == key) && (BUTTON_EVENT_PRESS == event))
    {
        p_nav->b_editing = !p_nav->b_editing;
    }
    else if (p_nav->b_editing &&
             ((BUTTONS_KEY_UP == key) || (BUTTONS_KEY_DOWN == key)))
    {
        b_changed =
            nav_adjust(p_cfg, p_nav->cursor, dir, BUTTON_EVENT_REPEAT == event);
    }
    else if (!p_nav->b_editing && (BUTTONS_KEY_UP == key) &&
             (p_nav->cursor > 0U))
    {
        p_nav->cursor--;
    }
    else if (!p_nav->b_editing && (BUTTONS_KEY_DOWN == key) &&
             ((p_nav->cursor + 1U) < (uint32_t)PAGES_SET_COUNT)) // 6 fits
    {
        p_nav->cursor++;
    }
    else
    {
        // At the end of the list, or a key this page does not use.
    }

    return b_changed;
}

void pages_nav_reset(pages_nav_t * p_nav)
{
    if (NULL != p_nav)
    {
        p_nav->page            = PAGES_MAIN;
        p_nav->cursor          = 0U;
        p_nav->b_editing       = false;
        p_nav->b_cool_selected = false;
    }
}

bool pages_nav_key(pages_nav_t * p_nav, settings_t * p_cfg, buttons_key_t key,
                   button_event_t event)
{
    bool     b_changed = false;
    uint32_t page      = 0U;

    if ((NULL == p_nav) || (NULL == p_cfg) || !nav_is_step(event))
    {
        goto done;
    }

    if ((BUTTONS_KEY_LEFT == key) || (BUTTONS_KEY_RIGHT == key))
    {
        if (BUTTON_EVENT_PRESS == event)
        {
            // Pages are a small enum; stepping modulo the count wraps both
            // ways and the result always fits back into pages_id_t.
            page = ((uint32_t)p_nav->page +
                    ((BUTTONS_KEY_RIGHT == key) ? 1U : (PAGES_COUNT - 1U))) %
                   (uint32_t)PAGES_COUNT;
            nav_enter(p_nav, p_cfg, (pages_id_t)page);
        }
        goto done;
    }

    switch (p_nav->page)
    {
        case PAGES_MAIN:
            b_changed = nav_main(p_nav, p_cfg, key, event);
            break;
        case PAGES_MODE:
            b_changed = nav_list(p_nav, p_cfg, key, event,
                                 (uint8_t)HVAC_MODE_COUNT, &p_cfg->mode);
            break;
        case PAGES_FAN:
            b_changed = nav_list(p_nav, p_cfg, key, event,
                                 (uint8_t)HVAC_FAN_COUNT, &p_cfg->fan);
            break;
        case PAGES_SETTINGS:
            b_changed = nav_settings(p_nav, p_cfg, key, event);
            break;
        case PAGES_INFO:
            break;
        default:
            pages_nav_reset(p_nav); // corrupted: back to a known page
            break;
    }

done:
    return b_changed;
}

bool pages_nav_cool_active(pages_nav_t const * p_nav, settings_t const * p_cfg)
{
    bool b_cool = false;

    if ((NULL != p_nav) && (NULL != p_cfg))
    {
        b_cool = (((uint8_t)HVAC_MODE_COOL == p_cfg->mode) ||
                  (((uint8_t)HVAC_MODE_AUTO == p_cfg->mode) &&
                   p_nav->b_cool_selected));
    }

    return b_cool;
}
