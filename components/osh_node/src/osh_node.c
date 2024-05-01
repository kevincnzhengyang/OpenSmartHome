/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:10:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-01 20:35:51
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "osh_node_comm.h"
#include "osh_node_fsm.h"
#include "osh_node_network.h"


// node svc task

// nvs
/* Initialize NVS partition */
// esp_err_t ret = nvs_flash_init();
// if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//     /* NVS partition was truncated
//         * and needs to be erased */
//     ESP_ERROR_CHECK(nvs_flash_erase());

//     /* Retry nvs_flash_init */
//     ESP_ERROR_CHECK(nvs_flash_init());
// }

/* Initialize the event loop */
// ESP_ERROR_CHECK(esp_event_loop_create_default());

// init fsm

// register events and callback

// set to OSH_FSM_STATE_INIT

// init status

// init reset button

// init network


/* invoke poweron */
// osh_invoke_event(OSH_NODE_EVENT_POWERON);

// loop
