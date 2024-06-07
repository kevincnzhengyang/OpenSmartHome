/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-06-02 20:03:54
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-06-07 22:47:52
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_proto.inc
 * @Description : private protocol
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_PROTO_INC
#define OSH_NODE_PROTO_INC

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/timers.h"

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"

#include "osh_node_proto_dataframe.h"
#include "osh_node_proto.h"

// MDM Entry start with bit31&30 set to 1
#define MDM_ENTRY_MASK        0xC0000000

// APP Entry start with bit31 set to 0
#define APP_ENTRY_MASK        0x10000000

#define OSH_NODE_ENTRY_IS_MDM(e) \
    (MDM_ENTRY_MASK == (((uint32_t)(e)) & MDM_ENTRY_MASK))

#define OSH_NODE_ENTRY_IS_APP(e) \
    (0 == (((uint32_t)(e)) & APP_ENTRY_MASK))

/* route method */
typedef struct {
    OSH_CODE_METHOD_ENUM         method;
    osh_node_proto_handler_t   route_cb;
    ListItem_t               route_item;
} osh_node_proto_route_t;

/* entry route */
typedef struct {
    uint32_t                      entry;
    List_t                   route_list;
    ListItem_t               entry_item;
} osh_node_proto_entry_t;

/* data buff */
typedef struct {
    size_t                         size;
    size_t                          len;
    uint8_t                       *base;
} osh_node_proto_buff_t;

/* proto */
typedef struct {
    osh_node_bb_t              *node_bb;
    osh_node_proto_session_t    session;
    List_t                   entry_list;
    TaskHandle_t             proto_task;
    TimerHandle_t              hb_timer;
    osh_node_proto_buff_t     recv_buff;
    osh_node_proto_buff_t     send_buff;
    osh_node_proto_pdu_t        request;
    osh_node_proto_pdu_t       response;
    int                     report_sock;
    int                        mdm_sock;
    int                        app_sock;
    struct sockaddr_in      report_addr;
    void                      *conf_arg;
} osh_node_proto_t;

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_PROTO_INC */
