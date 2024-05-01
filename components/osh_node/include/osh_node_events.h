/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 11:13:18
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 11:13:19
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_events.h
 * @Description : events definitions
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_EVENTS_H
#define OSH_NODE_EVENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include "esp_event.h"


// events for NODE_STATE_INIT
#define OSH_NODE_EVENT_POWERON              BIT0
#define OSH_NODE_EVENT_CONNECT              BIT1
#define OSH_NODE_EVENT_DISCONNECT           BIT2

#define OSH_NODE_EVENT_INVOKE               BIT16
#define OSH_NODE_EVENT_REQUEST              BIT17
#define OSH_NODE_EVENT_UPDATE               BIT18
#define OSH_NODE_EVENT_OTA_COMPLETE         BIT19
#define OSH_NODE_EVENT_OTA_ROLLBACK         BIT20

#define OSH_NODE_EVENT_T_IDLE               BIT32
#define OSH_NODE_EVENT_T_SAVE               BIT33
#define OSH_NODE_EVENT_T_WAKEUP             BIT34

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_EVENTS_H */


