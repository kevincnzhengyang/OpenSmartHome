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
#include "osh_node_network.inc"

/** -------------------------------
 *            functions
 *  -------------------------------
*/

/* init node WiFi */
esp_err_t osh_node_init_wifi(osh_net_msg_callback msg_callback, void *arg);

/* reset node wifi */
esp_err_t osh_node_reset_wifi(void);

/* get node name */
const char* osh_node_get_name_wifi(void);

/** -------------------------------
 *         events
 *  -------------------------------
*/

/**
 * state: OSH_FSM_STATE_INIT
 * event: OSH_NODE_EVENT_POWERON
 * next state: OSH_FSM_STATE_INIT, wait for network provision and connect
*/
esp_err_t osh_node_wifi_init_poweron(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_INIT
 * event: OSH_NODE_EVENT_CONNECT
 * next state: OSH_FSM_STATE_IDLE, wait for request
*/
esp_err_t osh_node_wifi_init_connect(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_DISCONNECT
 * next state: OSH_FSM_STATE_INIT, wait for connect
*/
esp_err_t osh_node_wifi_idle_disconnect(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_T_SAVE
 * next state: OSH_FSM_STATE_SAVING, wait for wakeup
*/
esp_err_t osh_node_wifi_idle_t_save(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_REQUEST
 * next state: OSH_FSM_STATE_ONGOING, wait for request or invoke
*/
esp_err_t osh_node_wifi_idle_request(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_UPDATE
 * next state: OSH_FSM_STATE_UPGRADING, wait for update
*/
esp_err_t osh_node_wifi_idle_update(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_T_IDLE
 * next state: OSH_FSM_STATE_IDLE, wait for request or invoke
*/
esp_err_t osh_node_wifi_ongoing_t_idle(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_REQUEST
 * next state: OSH_FSM_STATE_ONGOING, wait for request or invoke
*/
esp_err_t osh_node_wifi_ongoing_request(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_DISCONNECT
 * next state: OSH_FSM_STATE_INIT, wait for network provision and connect
*/
esp_err_t osh_node_wifi_ongoing_disconnect(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_UPDATE
 * next state: OSH_FSM_STATE_UPGRADING, wait for update
*/
esp_err_t osh_node_wifi_ongoing_update(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_DOWNLOAD
 * next state: OSH_FSM_STATE_UPGRADING, wait for ota completed or failed
*/
esp_err_t osh_node_wifi_upgrading_download(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_OTA_COMPLETE
 * next state: OSH_FSM_STATE_INIT, restart
*/
esp_err_t osh_node_wifi_upgrading_complete(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_OTA_ROLLBACK
 * next state: OSH_FSM_STATE_INIT, restart
*/
esp_err_t osh_node_wifi_upgrading_rollback(void *config, void *arg);

/**
 * state: OSH_FSM_STATE_SAVING
 * event: OSH_NODE_EVENT_T_WAKEUP
 * next state: OSH_FSM_STATE_IDLE, wait for request or invoke
*/
esp_err_t osh_node_wifi_saving_t_wakeup(void *config, void *arg);
#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_WIFI_H */