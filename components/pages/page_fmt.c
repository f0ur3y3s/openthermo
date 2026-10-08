/**
 * @file  page_fmt.c
 * @brief Pure text formatting for the pages. See page_fmt.h.
 */
#include "page_fmt.h"

#include <stddef.h>
#include <stdio.h>

#define F10_FREEZING 320 // 32.0 F
#define C_PER_F_NUM  5
#define C_PER_F_DEN  9
#define TENTHS       10
#define SEC_PER_MIN  60U

// num / den, rounded to nearest with halves away from zero. den > 0.
static int32_t page_fmt_div_round(int32_t num, int32_t den)
{
    int32_t result = 0;

    if (num >= 0)
    {
        result = (num + (den / 2)) / den;
    }
    else
    {
        result = (num - (den / 2)) / den;
    }

    return result;
}

int16_t page_fmt_temp(int16_t f10, bool b_celsius)
{
    int32_t out = f10;

    if (b_celsius)
    {
        out =
            page_fmt_div_round((out - F10_FREEZING) * C_PER_F_NUM, C_PER_F_DEN);
    }

    // Celsius tenths are smaller in magnitude than (F tenths + 320), and
    // the result of 5/9 of an int16_t range shift stays inside int16_t.
    return (int16_t)out;
}

int16_t page_fmt_delta(int16_t f10, bool b_celsius)
{
    int32_t out = f10;

    if (b_celsius)
    {
        out = page_fmt_div_round(out * C_PER_F_NUM, C_PER_F_DEN);
    }

    // 5/9 of an int16_t is still an int16_t.
    return (int16_t)out;
}

void page_fmt_tenths(char * p_buf, uint32_t len, int16_t tenths, bool b_signed)
{
    int32_t      mag    = tenths;
    char const * p_sign = "";

    if ((NULL == p_buf) || (0U == len))
    {
        goto done;
    }

    if (mag < 0)
    {
        mag    = -mag;
        p_sign = "-";
    }
    else if (b_signed && (mag > 0))
    {
        p_sign = "+";
    }
    else
    {
        // Unsigned display, or zero.
    }

    (void)snprintf(p_buf, len, "%s%ld.%ld", p_sign, (long)(mag / TENTHS),
                   (long)(mag % TENTHS));

done:
    return;
}

void page_fmt_mmss(char * p_buf, uint32_t len, uint32_t seconds)
{
    if ((NULL == p_buf) || (0U == len))
    {
        goto done;
    }

    (void)snprintf(p_buf, len, "%lu:%02lu",
                   (unsigned long)(seconds / SEC_PER_MIN),
                   (unsigned long)(seconds % SEC_PER_MIN));

done:
    return;
}
