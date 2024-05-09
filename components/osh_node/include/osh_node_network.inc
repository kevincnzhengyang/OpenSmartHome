/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-01 23:24:02
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-08 20:44:14
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_network.inc
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_NETWORK_INC
#define OSH_NODE_NETWORK_INC

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"
#include "osh_node_proto.h"


#define OSH_ERR_NET_INNER               (OSH_ERR_NET_BASE +     1)
#define OSH_ERR_NET_LINK                (OSH_ERR_NET_BASE +     2)
#define OSH_ERR_NET_RECV                (OSH_ERR_NET_BASE +     3)
#define OSH_ERR_NET_SEND                (OSH_ERR_NET_BASE +     4)


#define NODE_NAME_LEN                       12

/* callback for network */
typedef esp_err_t (* init_network)(size_t rx_buff_size, size_t tx_buff_size,
                osh_node_proto *proto, void *arg);
typedef esp_err_t (* reset_network)(void);
typedef const char * (* get_node_name)(void);
typedef esp_err_t (* send_response)(void *arg);
typedef esp_err_t (* event_network)(void *config, void *arg);


/* network */
typedef struct
{
    char      node_name[NODE_NAME_LEN];
    TaskHandle_t           server_task;
    osh_node_proto              *proto;
    void                          *arg;

    uint8_t                   *rx_buff;
    uint8_t                   *tx_buff;
    size_t                     rx_size;
    size_t                      rx_len;
    size_t                     tx_size;
    size_t                      tx_len;

    init_network                  init;
    reset_network                reset;
    get_node_name             get_name;
    send_response              reponse;

    event_network         init_poweron;
    event_network         init_connect;
    event_network      idle_disconnect;
    event_network          idle_t_save;
    event_network         idle_request;
    event_network          idle_update;
    event_network       ongoing_t_idle;
    event_network      ongoing_request;
    event_network   ongoing_disconnect;
    event_network       ongoing_update;
    event_network   upgrading_download;
    event_network   upgrading_complete;
    event_network   upgrading_rollback;
    event_network      saving_t_wakeup;
} osh_node_network;

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_NETWORK_INC */
