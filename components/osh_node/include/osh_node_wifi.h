/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:12:39
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 00:12:44
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_wifi.h
 * @Description :
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_WIFI_H
#define OSH_NODE_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"
#include "osh_node_status.h"
#include "osh_node_proto.h"
#include "osh_node.h"

/** -------------------------------
 *            functions
 *  -------------------------------
*/

/* init WiFi */
esp_err_t osh_node_wifi_init(osh_node_bb_t *node_bb, void *conf_arg);

/* start WiFi */
esp_err_t osh_node_wifi_start(void *run_arg);

/* reset WiFi */
esp_err_t osh_node_wifi_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_WIFI_H */