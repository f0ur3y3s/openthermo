/**
 * @file  board.h
 * @brief Board wiring for the Seeed XIAO ESP32-C6. Every fact the wiring
 *        diagram shows lives here, so re-pinning is a one-file change. See
 *        docs/HARDWARE.md.
 *
 * Rules every relay pin must meet (CLAUDE.md, hard safety rules):
 *   - not a strapping pin, and not a UART TX pin, so nothing toggles it
 *     during reset or boot;
 *   - driven low by relays_init(), the first act of app_start().
 *
 * The relay module is H-trigger: high energises the coil. Between reset and
 * relays_init() the pins are inputs, so an IN line floats for the few hundred
 * milliseconds the ROM and bootloader take. A 10 k pull-down from each IN pin
 * to GND holds the relays off through that window.
 */
#ifndef BOARD_H
#define BOARD_H

#include "sdkconfig.h"
#include "soc/gpio_num.h"

#if CONFIG_IDF_TARGET_ESP32C6

/*
 * Seeed XIAO ESP32-C6. Checked against Seeed's pinout diagram (Oct 2026).
 * Strapping pins (4, 5, 8, 9, 15; 15 is also the user LED), USB (12, 13) and
 * flash (24..30) carry no relay; GPIO3/14 are the RF switch (below). D6 =
 * GPIO16 is U0TXD and D7 = GPIO17 U0RXD, used as keys: the console is on
 * USB. The ROM drives U0TXD at reset, so the right key has a 1 k series
 * resistor; holding it then only garbles the ROM's boot message.
 */
#define BOARD_PIN_RELAY_Y1 GPIO_NUM_1  // D1, IN1, compressor
#define BOARD_PIN_RELAY_G  GPIO_NUM_2  // D2, IN2, blower
#define BOARD_PIN_RELAY_O  GPIO_NUM_21 // D3, IN3, reversing valve (on = cool)
#define BOARD_PIN_RELAY_W  GPIO_NUM_18 // D10, IN4, aux / emergency strips

#define BOARD_PIN_SDA GPIO_NUM_22 // D4
#define BOARD_PIN_SCL GPIO_NUM_23 // D5

#define BOARD_PIN_KEY_UP            GPIO_NUM_0  // D0
#define BOARD_PIN_KEY_DOWN          GPIO_NUM_19 // D8
#define BOARD_PIN_KEY_LEFT          GPIO_NUM_20 // D9
#define BOARD_PIN_KEY_RIGHT         GPIO_NUM_16 // D6, also U0TXD: 1 k series
#define BOARD_PIN_KEY_CENTER        GPIO_NUM_17 // D7

/*
 * RF switch (Seeed wiki): GPIO3 low enables the switch, GPIO14 selects the
 * antenna, low = built-in ceramic, high = external u.FL. board_init()
 * drives both low so the radio path is set, not left to floating pins.
 */
#define BOARD_HAS_RF_SWITCH         1
#define BOARD_PIN_RF_SWITCH_EN      GPIO_NUM_3
#define BOARD_PIN_RF_ANT_SEL        GPIO_NUM_14

/*
 * On-board user LED (yellow), active low. Used only by the bench build
 * (status_led.h). GPIO15 is a strapping pin, but only sampled at reset;
 * driving it afterwards is fine.
 */
#define BOARD_PIN_STATUS_LED        GPIO_NUM_15
#define BOARD_STATUS_LED_ACTIVE_LOW 1

#else
#error "board.h: the firmware targets the XIAO ESP32-C6 only (esp32c6)"
#endif

/*
 * The D-pad switches pull to GND and use the internal pull-ups, so a chafed
 * switch lead shorts ground to ground rather than 3V3 to ground.
 */
#define BOARD_KEY_ACTIVE_LOW 1

/* One I2C bus shared by the OLED and the SHT40. */
#define BOARD_I2C_HZ     400000UL
#define BOARD_OLED_ADDR  0x3CU
#define BOARD_SHT40_ADDR 0x44U

/*
 * If the image looks shifted or garbled, the module is probably an SH1106:
 * swap to u8g2_Setup_sh1106_i2c_128x64_noname_f.
 */
#define BOARD_U8G2_SETUP_FN u8g2_Setup_ssd1306_i2c_128x64_noname_f

/**
 * Board set-up that is not a peripheral of its own: on the XIAO C6, the RF
 * switch. Call right after relays_init(), before any radio starts.
 */
void board_init(void);

#endif /* BOARD_H */
