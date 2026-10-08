/**
 * @file  status_led.h
 * @brief BENCH ONLY: shows the relay outputs on the XIAO C6's on-board user
 *        LED (BOARD_PIN_STATUS_LED), for a board with nothing wired to the
 *        IN pins.
 *
 * Everything here exists only when OPENTHERMO_STATUS_LED is 1, which only
 * the C6 bench build defines (`tools/matter.sh build esp32c6 sim`). The real
 * firmware compiles none of it.
 *
 * One colour, so states are rhythms. Each pattern is a 2 s frame of sixteen
 * 125 ms slots (bit 0 first), highest priority first:
 *
 *   sensor fault       slow blink, 1 s on / 1 s off
 *   W (aux, e-heat)    fast blink, 4 Hz
 *   Y1 + O (cooling)   on, with two short dips
 *   Y1 (heating)       solid on
 *   waiting (lockout)  three short blips
 *   G only (fan)       two short blips
 *   idle               one short blip (proves the firmware is alive)
 *
 * The UI task calls status_led_update() every tick; the pattern follows the
 * clock, so no timer or shared state is needed.
 */
#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>
#include <stdint.h>

#if OPENTHERMO_STATUS_LED

#define STATUS_LED_SLOT_MS 125U
#define STATUS_LED_SLOTS   16U

#define STATUS_LED_FAULT   0x00FFU
#define STATUS_LED_AUX     0x5555U
#define STATUS_LED_COOLING 0x7F7FU
#define STATUS_LED_HEATING 0xFFFFU
#define STATUS_LED_WAITING 0x0015U
#define STATUS_LED_FAN     0x0005U
#define STATUS_LED_IDLE    0x0001U

/**
 * The pattern for this state (pure; host-tested).
 */
uint16_t status_led_pattern(bool b_fault, bool b_waiting, bool b_y1, bool b_g,
                            bool b_o, bool b_w);

/**
 * Whether the LED is lit at now_ms in this pattern (pure; host-tested).
 */
bool status_led_lit(uint16_t pattern, uint64_t now_ms);

/**
 * Makes BOARD_PIN_STATUS_LED an output, LED off.
 */
void status_led_init(void);

/**
 * Lights or darkens the LED for this pattern at now_ms. Writes the pin only
 * when the level changes. UI task only.
 */
void status_led_update(uint16_t pattern, uint64_t now_ms);

#endif /* OPENTHERMO_STATUS_LED */

#endif /* STATUS_LED_H */
