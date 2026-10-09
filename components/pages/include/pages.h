/**
 * @file  pages.h
 * @brief Draws the current page: main, mode, fan, settings, info, pair.
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
#include <stdbool.h>

// Longest pairing texts kept: the QR payload ("MT:" plus base-38) and the
// manual code (11 digits, or 21 with a vendor and product ID).
#define PAGES_PAIR_QR_LEN     64U
#define PAGES_PAIR_MANUAL_LEN 24U

/**
 * What the pair page shows. A NULL p_qr means there is no network to pair
 * (Matter off); otherwise the QR payload and manual code, and whether the
 * thermostat is already paired. The QR code is encoded here, once per new
 * payload. UI task only.
 */
void pages_set_pairing(char const * p_qr, char const * p_manual, bool b_paired);

/**
 * Draws one complete frame for the page in p_nav and sends it. NULL
 * arguments draw nothing.
 */
void pages_draw(pages_nav_t const * p_nav, settings_t const * p_cfg,
                control_status_t const * p_status);

#endif /* PAGES_H */
