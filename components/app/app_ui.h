/**
 * @file  app_ui.h
 * @brief The UI loop: D-pad in, pages out, settings saved, display dimmed.
 *        Runs on the main task, which owns the display (rule 9).
 */
#ifndef APP_UI_H
#define APP_UI_H

#include "app.h"

/**
 * Runs the UI loop on the calling task. Does not return.
 *
 * @param p_net the network, for the pair page and the factory reset; NULL
 *              when it is not running (the pair page then says so, and the
 *              factory reset only resets the settings and restarts)
 */
void app_ui_run(app_net_t const * p_net);

#endif /* APP_UI_H */
