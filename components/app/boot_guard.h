/**
 * @file  boot_guard.h
 * @brief Pure crash-loop breaker: counts consecutive crash resets in a
 *        record that survives them, and says when to stop starting the
 *        network and to hold the outputs off for a while after boot.
 *
 * No ESP-IDF, host-tested; app.c keeps the record in RTC memory that a CPU
 * or system reset leaves alone (a power-on clears it, which reads as no
 * record). A crash is a panic, a watchdog or a brown-out; any other reset
 * (power-on, the reset button, a software restart, a flash) starts the count
 * over, and so does running BOOT_GUARD_CLEAR_MS without one.
 *
 * Once BOOT_GUARD_TRIP crashes in a row are counted, the boot skips Matter
 * (the likeliest cause, and not needed to heat or cool) and holds every
 * output off for BOOT_GUARD_HOLD_MS, so a relay cannot chatter at the crash
 * rate. Neither shortens any lockout; both only add off time.
 */
#ifndef BOOT_GUARD_H
#define BOOT_GUARD_H

#include <stdbool.h>
#include <stdint.h>

#define BOOT_GUARD_MAGIC    0x6F744247UL // "otBG"
#define BOOT_GUARD_TRIP     3U
#define BOOT_GUARD_MAX      255U
#define BOOT_GUARD_HOLD_MS  60000U           // 1 min all off once tripped
#define BOOT_GUARD_CLEAR_MS (15UL * 60000UL) // 15 min up clears the count

typedef struct
{
    uint32_t magic;       // BOOT_GUARD_MAGIC when the record is set
    uint32_t crashes;     // consecutive crash resets
    uint32_t crashes_inv; // ~crashes, a check copy
} boot_guard_rec_t;

/**
 * Updates the record for this boot and returns the consecutive crash count
 * including it. A record that fails its checks counts as zero. NULL gives 0.
 *
 * @param p_rec        the record kept across resets
 * @param b_crash_boot true if this boot follows a crash reset
 */
uint32_t boot_guard_on_boot(boot_guard_rec_t * p_rec, bool b_crash_boot);

/**
 * True once the count reaches BOOT_GUARD_TRIP.
 */
bool boot_guard_tripped(uint32_t crashes);

/**
 * Sets the count to zero (after BOOT_GUARD_CLEAR_MS of running). NULL is
 * ignored.
 */
void boot_guard_clear(boot_guard_rec_t * p_rec);

#endif /* BOOT_GUARD_H */
