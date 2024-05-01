/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 11:05:34
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 11:05:35
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_network.h
 * @Description :
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_NETWORK_H
#define OSH_NODE_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_network.inc"

// transport
#ifdef CONFIG_NODE_NETWORK_TRANSPORT_WIFI
#include "osh_node_wifi.h"


/* init node network */
#define osh_node_init_network(msg_callback, arg) osh_node_init_wifi(msg_callback, arg)

/* reset node network */
#define osh_node_reset_network() osh_node_reset_wifi()

/* get node name */
#define osh_node_get_name() osh_node_get_name_wifi()


/** -------------------------------
 *         events
 *  -------------------------------
*/

/**
 * state: OSH_FSM_STATE_INIT
 * event: OSH_NODE_EVENT_POWERON
 * next state:
 *   - OSH_FSM_STATE_INIT: wait for network provision
*/
#define osh_node_network_init_poweron(config, arg) \
    osh_node_wifi_init_poweron(config, arg)

/**
 * state: OSH_FSM_STATE_INIT
 * event: OSH_NODE_EVENT_CONNECT
 * next state: OSH_FSM_STATE_IDLE, wait for request
*/
#define osh_node_network_init_connect(config, arg) \
    osh_node_wifi_init_connect(config, arg)

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_DISCONNECT
 * next state: OSH_FSM_STATE_INIT, wait for connect
*/
#define osh_node_network_idle_disconnect(config, arg) \
    osh_node_wifi_idle_disconnect(config, arg)

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_T_SAVE
 * next state: OSH_FSM_STATE_SAVING, wait for wakeup
*/
#define osh_node_network_idle_t_save(config, arg) \
    osh_node_wifi_idle_t_save(config, arg)

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_REQUEST
 * next state: OSH_FSM_STATE_ONGOING, wait for request or invoke
*/
#define osh_node_network_idle_request(config, arg) \
    osh_node_wifi_idle_request(config, arg)

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_UPDATE
 * next state: OSH_FSM_STATE_UPGRADING, wait for update
*/
#define osh_node_network_idle_update(config, arg) \
    osh_node_wifi_idle_update(config, arg)

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_T_IDLE
 * next state: OSH_FSM_STATE_IDLE, wait for request or invoke
*/
#define osh_node_network_ongoing_t_idle(config, arg) \
    osh_node_wifi_ongoing_t_idle(config, arg)

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_REQUEST
 * next state: OSH_FSM_STATE_ONGOING, wait for request or invoke
*/
#define osh_node_network_ongoing_request(config, arg) \
    osh_node_wifi_ongoing_request(config, arg)

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_DISCONNECT
 * next state: OSH_FSM_STATE_INIT, wait for network provision and connect
*/
#define osh_node_network_ongoing_disconnect(config, arg) \
    osh_node_wifi_ongoing_disconnect(config, arg)

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_UPDATE
 * next state: OSH_FSM_STATE_UPGRADING, wait for update
*/
#define osh_node_network_ongoing_update(config, arg) \
    osh_node_wifi_ongoing_update(config, arg)

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_DOWNLOAD
 * next state: OSH_FSM_STATE_UPGRADING, wait for ota completed or failed
*/
#define osh_node_upgrading_download(config, arg) \
    osh_node_wifi_upgrading_download(config, arg)

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_OTA_COMPLETE
 * next state: OSH_FSM_STATE_INIT, restart
*/
#define osh_node_upgrading_complet(config, arg) \
    osh_node_wifi_upgrading_complete(config, arg)

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_OTA_ROLLBACK
 * next state: OSH_FSM_STATE_INIT, restart
*/
#define osh_node_network_upgrading_rollback(config, arg) \
    osh_node_wifi_upgrading_rollback(config, arg)

/**
 * state: OSH_FSM_STATE_SAVING
 * event: OSH_NODE_EVENT_T_WAKEUP
 * next state: OSH_FSM_STATE_IDLE, wait for request or invoke
*/
#define osh_node_network_saving_t_wakeup(config, arg) \
    osh_node_wifi_saving_t_wakeup(void *config, void *arg)


#elif CONFIG_NODE_NETWORK_TRANSPORT_BLE
#include "osh_node_ble.h"

#else
#error "Unknow Node Network Transport"
#endif /* CONFIG_NODE_NETWORK_TRANSPORT_WIFI */


#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_NETWORK_H */

