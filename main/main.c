
#include <stdio.h>

#include "osh_node.h"
#include "osh_node_status.h"
#include "osh_node_wifi.h"

static osh_node_module_t modules[] = {
    //  name    init_cb      conf_arg      start_cb       run_arg
    {"status",   osh_node_status_init,     NULL,   osh_node_status_start, NULL},
    {"reset", osh_node_reset_btn_init,    NULL, NULL, NULL},
    {"network", osh_node_wifi_init,  NULL, osh_node_wifi_start, NULL}
};

/* Hello World Example */
void app_main(void)
{
    /* init node */
    ESP_ERROR_CHECK(osh_node_init());

    /* init modules */
    ESP_ERROR_CHECK(osh_node_modules_init(modules,
                        (sizeof(modules) / sizeof(osh_node_module_t))));
    /* start modules */
    ESP_ERROR_CHECK(osh_node_modules_start());

    /* wait forever */
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}
