/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-09 10:28:18
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-11 21:30:50
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_proto.inc
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_PROTO_INC
#define OSH_NODE_PROTO_INC

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"

#include <freertos/list.h>

#include "osh_node_proto.h"

/* state item of FSM */
typedef struct {
    coap_request_t                 method;
    coap_route_cb                route_cb;
    ListItem_t                 route_item;
} osh_node_proto_route_t;

typedef struct {
    char                             *uri;
    List_t                     route_list;
    ListItem_t                 entry_item;
} osh_node_proto_entry_t;

/* event item of state */
typedef struct {
    List_t                     entry_list;
    TaskHandle_t                coap_task;
    size_t                      buff_size;
    size_t                       buff_len;
    uint8_t                         *buff;
    void                        *conf_arg;
} osh_node_proto_t;

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_PROTO_INC */

