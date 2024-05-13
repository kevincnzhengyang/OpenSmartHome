/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:10:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-13 23:05:04
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "osh_node.h"

#include "nvs_flash.h"

#include "osh_node_fsm.h"
#include "osh_node_network.h"
#include "osh_node_proto.h"
#include "osh_node_status.h"


// node svc task

// register events and callback

// loop

/**
 * Init OSH Node
 *
 * @proto_buff_size: size for COAP proto buffer
 * @conf_arg: user define argument
*/
esp_err_t osh_node_init(size_t proto_buff_size, void *conf_arg) {
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
            * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Init Status LED */
    ESP_ERROR_CHECK(osh_node_status_init());

    /* Init Reset BUtton */
    ESP_ERROR_CHECK(osh_node_reset_btn_init());

    /* Init Network */
    ESP_ERROR_CHECK(osh_node_wifi_init(proto_buff_size, conf_arg));

    return ESP_OK;
}


/**
 * start running of OSH Node
*/
esp_err_t osh_node_start(void *run_arg) {
    // start status
    ESP_ERROR_CHECK(osh_node_status_start());

    // start WiFi
    ESP_ERROR_CHECK(osh_node_network_start(run_arg));

    while (1) {
        // move on the FSM
        if (ESP_OK != osh_node_fsm_loop_step(run_arg)) break;
    }
    return ESP_OK;
}
