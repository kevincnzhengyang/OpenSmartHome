/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-01 23:24:02
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-01 23:29:30
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


#define OSH_NODE_NET_INNER              (OSH_ERR_NET_BASE +     1)

/* callback for message received */
typedef esp_err_t (*osh_net_msg_callback)(const char* buff, const int length, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_NETWORK_INC */
