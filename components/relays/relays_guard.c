/**
 * @file  relays_guard.c
 * @brief Independent interlock on the relay outputs. See relays_guard.h.
 */
#include "relays_guard.h"

#include <stddef.h>
#include <string.h>

#define BIT_Y1 0x01U
#define BIT_G  0x02U
#define BIT_O  0x04U
#define BIT_W  0x08U

static uint8_t guard_pack(relays_outputs_t const * p_out)
{
    uint32_t bits = 0U;

    bits |= p_out->b_y1 ? BIT_Y1 : 0U;
    bits |= p_out->b_g ? BIT_G : 0U;
    bits |= p_out->b_o ? BIT_O : 0U;
    bits |= p_out->b_w ? BIT_W : 0U;

    // Four flag bits: fits uint8_t.
    return (uint8_t)bits;
}

// Records what is driven and when Y1 went off, with the inverted copies.
static void guard_store(relays_guard_t *         p_guard,
                        relays_outputs_t const * p_out, uint64_t off_since_ms)
{
    p_guard->applied         = *p_out;
    p_guard->y1_off_since_ms = off_since_ms;
    // Inverting a uint8_t promotes to int; the cast keeps the low 8 bits.
    p_guard->applied_inv   = (uint8_t)~guard_pack(p_out);
    p_guard->off_since_inv = ~off_since_ms;
}

static bool guard_intact(relays_guard_t const * p_guard)
{
    // The XOR of a value and its complement is all ones.
    return (((uint32_t)guard_pack(&p_guard->applied) ^ p_guard->applied_inv) ==
            0xFFU) &&
           ((p_guard->y1_off_since_ms ^ p_guard->off_since_inv) == UINT64_MAX);
}

void relays_guard_init(relays_guard_t * p_guard, uint64_t now_ms)
{
    relays_outputs_t off = { 0 };

    if (NULL == p_guard)
    {
        goto done;
    }

    guard_store(p_guard, &off, now_ms);
    p_guard->resyncs = 0U;

done:
    return;
}

bool relays_guard_step(relays_guard_t * p_guard, relays_outputs_t const * p_req,
                       uint64_t now_ms, relays_outputs_t * p_out)
{
    bool             b_pass    = false;
    bool             b_off_ok  = false;
    bool             b_all_off = false;
    relays_outputs_t out       = { 0 };
    uint64_t         off_ms    = 0U;

    if (NULL == p_out)
    {
        goto done;
    }
    if ((NULL == p_guard) || (NULL == p_req))
    {
        memset(p_out, 0, sizeof(*p_out));
        goto done;
    }
    if (!guard_intact(p_guard))
    {
        // State corrupted: trust nothing. All off, minimum-off from now.
        guard_store(p_guard, &out, now_ms);
        p_guard->resyncs++;
        *p_out = out;
        goto done;
    }

    out      = *p_req;
    off_ms   = p_guard->y1_off_since_ms;
    b_off_ok = (!p_guard->applied.b_y1 && (now_ms > off_ms) &&
                ((now_ms - off_ms) >= RELAYS_GUARD_MIN_OFF_MS));

    b_all_off = (!p_req->b_y1 && !p_req->b_g && !p_req->b_o && !p_req->b_w);

    if ((out.b_o != p_guard->applied.b_o) && !b_off_ok)
    {
        if (b_all_off)
        {
            // An all-off request (sensor fault, NULL) is honoured in full,
            // as a reset would be: O drops with everything else, and the
            // minimum-off time restarts now, so neither Y1 nor O may change
            // again for RELAYS_GUARD_MIN_OFF_MS.
            off_ms = now_ms;
        }
        else
        {
            out.b_o  = p_guard->applied.b_o;
            out.b_y1 = false;
        }
    }
    if (out.b_y1 && !p_guard->applied.b_y1 && !b_off_ok)
    {
        out.b_y1 = false;
    }
    if (out.b_w && out.b_y1 && out.b_o)
    {
        out.b_w = false;
    }
    if (out.b_y1 || out.b_w)
    {
        out.b_g = true;
    }

    if (p_guard->applied.b_y1 && !out.b_y1)
    {
        off_ms = now_ms;
    }
    guard_store(p_guard, &out, off_ms);
    *p_out = out;

    b_pass = ((out.b_y1 == p_req->b_y1) && (out.b_g == p_req->b_g) &&
              (out.b_o == p_req->b_o) && (out.b_w == p_req->b_w));

done:
    return b_pass;
}
