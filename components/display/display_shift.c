/**
 * @file  display_shift.c
 * @brief Pure burn-in creep. See display_shift.h.
 */
#include "display_shift.h"

#include <stddef.h>
#include <string.h>

#define MS_PER_S      1000ULL
#define BITS_PER_PAGE 8U
#define MAX_PAGES     8U // one column fits a uint64_t

// Triangle wave over 0..max: 0, 1, .., max, max - 1, .., 1, 0, 1, ..
static uint8_t display_shift_triangle(uint64_t step, uint8_t max)
{
    uint64_t span = 2ULL * max;
    uint64_t pos  = 0U;

    if (0U != span)
    {
        pos = step % span;
        if (pos > max)
        {
            pos = span - pos;
        }
    }

    // pos is at most max, a uint8_t.
    return (uint8_t)pos;
}

void display_shift_offsets(uint64_t now_ms, uint8_t * p_dx, uint8_t * p_dy)
{
    uint64_t now_s = now_ms / MS_PER_S;

    if (NULL != p_dx)
    {
        *p_dx = display_shift_triangle(now_s / DISPLAY_SHIFT_X_PERIOD_S,
                                       DISPLAY_SHIFT_X_MAX);
    }
    if (NULL != p_dy)
    {
        *p_dy = display_shift_triangle(now_s / DISPLAY_SHIFT_Y_PERIOD_S,
                                       DISPLAY_SHIFT_Y_MAX);
    }
}

// Each page row moves dx bytes (columns) right; the left strip clears.
static void display_shift_right(uint8_t * p_buf, uint32_t width, uint32_t pages,
                                uint8_t dx)
{
    uint32_t  page  = 0U;
    uint8_t * p_row = NULL;

    for (page = 0U; page < pages; page++)
    {
        p_row = &p_buf[page * width];
        memmove(&p_row[dx], p_row, width - dx);
        memset(p_row, 0, dx);
    }
}

// Each column, gathered into one word, moves dy bits down (towards the
// higher bits); the top strip clears and the bottom overflow drops.
static void display_shift_down(uint8_t * p_buf, uint32_t width, uint32_t pages,
                               uint8_t dy)
{
    uint32_t col    = 0U;
    uint32_t page   = 0U;
    uint64_t pixels = 0U;
    uint64_t keep   = 0U;

    // Low pages * 8 bits: the column's real height. pages <= 8 here.
    keep = (pages >= MAX_PAGES) ? UINT64_MAX
                                : ((1ULL << (pages * BITS_PER_PAGE)) - 1U);

    for (col = 0U; col < width; col++)
    {
        pixels = 0U;
        for (page = 0U; page < pages; page++)
        {
            pixels |= (uint64_t)p_buf[(page * width) + col]
                      << (page * BITS_PER_PAGE);
        }
        pixels = (dy >= 64U) ? 0U : ((pixels << dy) & keep);
        for (page = 0U; page < pages; page++)
        {
            // Masked to 8 bits, so the narrowing drops nothing.
            p_buf[(page * width) + col] =
                (uint8_t)((pixels >> (page * BITS_PER_PAGE)) & 0xFFU);
        }
    }
}

void display_shift_buffer(uint8_t * p_buf, uint32_t width, uint32_t pages,
                          uint8_t dx, uint8_t dy)
{
    if ((NULL == p_buf) || (0U == width) || (0U == pages) ||
        (pages > MAX_PAGES) || (dx >= width))
    {
        goto done;
    }

    if (0U != dx)
    {
        display_shift_right(p_buf, width, pages, dx);
    }
    if (0U != dy)
    {
        display_shift_down(p_buf, width, pages, dy);
    }

done:
    return;
}
