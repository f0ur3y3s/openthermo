/**
 * @file  boot_guard.c
 * @brief Pure crash-loop breaker. See boot_guard.h.
 */
#include "boot_guard.h"

#include <stddef.h>

static void boot_guard_store(boot_guard_rec_t * p_rec, uint32_t crashes)
{
    p_rec->magic       = BOOT_GUARD_MAGIC;
    p_rec->crashes     = crashes;
    p_rec->crashes_inv = ~crashes;
}

uint32_t boot_guard_on_boot(boot_guard_rec_t * p_rec, bool b_crash_boot)
{
    uint32_t crashes = 0U;

    if (NULL == p_rec)
    {
        goto done;
    }

    if ((BOOT_GUARD_MAGIC == p_rec->magic) &&
        (p_rec->crashes_inv == ~p_rec->crashes) &&
        (p_rec->crashes <= BOOT_GUARD_MAX))
    {
        crashes = p_rec->crashes;
    }

    if (!b_crash_boot)
    {
        crashes = 0U;
    }
    else if (crashes < BOOT_GUARD_MAX)
    {
        crashes++;
    }
    else
    {
        // Saturated.
    }

    boot_guard_store(p_rec, crashes);

done:
    return crashes;
}

bool boot_guard_tripped(uint32_t crashes)
{
    return (crashes >= BOOT_GUARD_TRIP);
}

void boot_guard_clear(boot_guard_rec_t * p_rec)
{
    if (NULL != p_rec)
    {
        boot_guard_store(p_rec, 0U);
    }
}
