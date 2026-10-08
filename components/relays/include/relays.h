/**
 * @file  relays.h
 * @brief The four relay outputs (IN1..IN4 = Y1, G, O, W) on the pins named by
 *        board.h. The module is H-trigger: high energises a coil.
 *
 * relays_init() runs on the main task before anything else; after that, only
 * the control task calls in. Every request passes through relays_guard, so
 * the hard safety rules are enforced here a second time.
 */
#ifndef RELAYS_H
#define RELAYS_H

#include "esp_err.h"
#include "relays_guard.h"
#include <stdint.h>

/**
 * Drives all four pins low, then makes them outputs, and arms the guard's
 * minimum-off timer. The first call in app_start().
 */
esp_err_t relays_init(void);

/**
 * Passes p_req through the guard and drives the result. Pins turning off are
 * written before pins turning on, and O and G before Y1, so the valve and the
 * blower are set before the compressor starts. A request the guard changes
 * is logged.
 *
 * The guard times its rules on its own read of the clock, not the caller's,
 * so a wrong time from the control task cannot loosen them.
 *
 * @param p_req requested outputs; NULL drives everything off
 */
void relays_apply(relays_outputs_t const * p_req);

/**
 * Drops all four outputs at once, with one register write: no driver, no
 * guard, no lock, so it is safe from the panic handler and the shutdown
 * path. Dropping is always allowed; the next boot re-arms minimum-off.
 * relays_init() registers it to run on esp_restart(), and every panic
 * calls it first (relays.c), so a CPU-only reset never carries a relay
 * through the reboot.
 */
void relays_drop_all_now(void);

/**
 * From now until the next reset, every request is driven as all off. For
 * the bench (sim) build when it finds a real SHT40 on the bus, which means
 * it is on an installed thermostat. Only ever removes energy.
 */
void relays_inhibit(void);

/**
 * Copies out what is being driven now.
 */
void relays_applied(relays_outputs_t * p_out);

/**
 * How many relays are energised now, 0..4 (for the sensor's self-heating
 * correction).
 */
uint8_t relays_energised_count(void);

#endif /* RELAYS_H */
