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

#include "osh_node_proto.h"

#define OSH_ERR_NET_INNER               (OSH_ERR_NET_BASE +     1)
#define OSH_ERR_NET_PROTO_BASE          (OSH_ERR_NET_BASE + 0x10000)

// transport
#ifdef CONFIG_NODE_NETWORK_TRANSPORT_WIFI
#include "osh_node_wifi.h"

/* config node network with WiFi */
#define osh_node_network_init(node_bb, conf_arg) osh_node_wifi_init(node_bb, conf_arg)

#define osh_node_network_start(run_arg) osh_node_wifi_start(run_arg)

#define osh_node_network_reset() osh_node_wifi_reset()

#elif CONFIG_NODE_NETWORK_TRANSPORT_BLE
#include "osh_node_ble.h"


/* config node network with BLE */
#define osh_node_network_create()
#define osh_node_network_get()
#else
#error "Unknow Node Network Transport"
#endif /* CONFIG_NODE_NETWORK_TRANSPORT_WIFI */


#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_NETWORK_H */

