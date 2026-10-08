/**
 * @file  pages.h
 * @brief Draws the current page: main, mode, fan, settings, info.
 *
 * UI task only (rule 9): it draws through display_u8g2() and sends the
 * frame. What is showing and what the keys do is pages_nav.h; text
 * formatting is page_fmt.h.
 */
#ifndef PAGES_H
#define PAGES_H

#include "control.h"
#include "pages_nav.h"
#include "settings_types.h"

/**
 * Draws one complete frame for the page in p_nav and sends it. NULL
 * arguments draw nothing.
 */
void pages_draw(pages_nav_t const * p_nav, settings_t const * p_cfg,
                control_status_t const * p_status);

#endif /* PAGES_H */
