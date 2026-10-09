/**
 * @file  main.c
 * @brief ESP-IDF entry point for the Matter build. Hands over to the app
 *        component, with the Matter bridge as its network.
 *
 * The first thing app_start() does is drive every relay output low; Matter
 * starts only once the control loop is running.
 */
#include "app.h"
#include "matter_bridge.h"

// ESP-IDF calls this by name, so it cannot carry a module prefix.
void app_main(void);

void app_main(void)
{
    static app_net_t const g_net = {
        .p_start         = matter_bridge_start,
        .p_factory_reset = matter_bridge_factory_reset,
        .p_pairing       = matter_bridge_pairing,
    };

    app_start(&g_net);
}
