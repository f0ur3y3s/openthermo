/**
 * @file  buttons_keys.h
 * @brief The five D-pad keys. Pure, so the host-tested UI logic can name
 *        them without pulling in ESP-IDF.
 */
#ifndef BUTTONS_KEYS_H
#define BUTTONS_KEYS_H

typedef enum
{
    BUTTONS_KEY_UP = 0,
    BUTTONS_KEY_DOWN,
    BUTTONS_KEY_LEFT,
    BUTTONS_KEY_RIGHT,
    BUTTONS_KEY_CENTER,
    BUTTONS_KEY_COUNT
} buttons_key_t;

#endif /* BUTTONS_KEYS_H */
