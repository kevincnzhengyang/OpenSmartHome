
#include <stdio.h>

#include "osh_node.h"
#include "osh_node_status.h"
#include "osh_node_wifi.h"
#include "osh_node_proto.h"

static osh_node_module_t modules[] = {
    //  name    init_cb      conf_arg      start_cb       run_arg
    {"status",   osh_node_status_init,     NULL,   osh_node_status_start, NULL},
    {"reset", osh_node_reset_btn_init,    NULL, NULL, NULL},
    {"network", osh_node_wifi_init,  NULL, osh_node_wifi_start, NULL}
};


static const char *APP_TAG = "APP";

#define TEST_ENTRY      0x12345

static char content[] = "Hello World!";

static esp_err_t test_entry(uint32_t entry,
            osh_node_bb_t *node_bb,
            osh_node_proto_session_t *session,
            const osh_node_proto_pdu_t *request,
            osh_node_proto_pdu_t *response) {
    if (TEST_ENTRY != entry) {
        ESP_LOGE(APP_TAG, "wroint engry 0x%lx", entry);
        return ESP_FAIL;
    }

    proto_response_ack_head(request, response, OSH_CC_SUCCESS, OSH_SUCCESS_CONTENT);
    response->con_type = OSH_CONTENT_TEXT;
    response->con_len = strlen(content) + 1;
    response->data = (uint8_t *)content;
    return ESP_OK;
}

/* Hello World Example */
void app_main(void)
{
    /* init node */
    ESP_ERROR_CHECK(osh_node_init());

    /* init modules */
    ESP_ERROR_CHECK(osh_node_modules_init(modules,
                        (sizeof(modules) / sizeof(osh_node_module_t))));

    /* register route */
    ESP_ERROR_CHECK(osh_node_route_register(TEST_ENTRY, OSH_METHOD_GET, test_entry));

    /* start modules */
    ESP_ERROR_CHECK(osh_node_modules_start());

    /* wait forever */
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}
