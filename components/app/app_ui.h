/**
 * @file  app_ui.h
 * @brief The UI loop: D-pad in, pages out, settings saved, display dimmed.
 *        Runs on the main task, which owns the display (rule 9).
 */
#ifndef APP_UI_H
#define APP_UI_H

/**
 * Runs the UI loop on the calling task. Does not return.
 */
void app_ui_run(void);

#endif /* APP_UI_H */
