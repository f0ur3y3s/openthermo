/**
 * @file  unity_config.h
 * @brief Unity configuration for the host tests.
 *
 * Its presence stops PlatformIO generating its own unity_config.c, which
 * would be compiled with the strict test flags (build_src_flags) and fails
 * -Wconversion. On the host, Unity's built-in defaults (putchar and fflush)
 * are all the tests need, so nothing is overridden here.
 */
#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#endif /* UNITY_CONFIG_H */
