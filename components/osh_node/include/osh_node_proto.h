/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-08 11:47:00
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-08 11:47:01
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_proto.h
 * @Description :
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_PROTO_H
#define OSH_NODE_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <sys/socket.h>

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"

#include "coap3/coap.h"

#define OSH_ERR_PROTO_BASE              (OSH_ERR_APP_BASE + 0x10000)
#define OSH_ERR_PROTO_INNER             (OSH_ERR_PROTO_BASE +     1)

/* callback */
typedef coap_method_handler_t coap_route_cb;

/* init proto */
esp_err_t osh_node_proto_init(size_t buff_size, void *conf_arg);

/* fini proto */
esp_err_t osh_node_proto_fini(void);

/* start proto */
esp_err_t osh_node_proto_start(void);

/* stop proto */
esp_err_t osh_node_proto_stop(void);

/* register route callback */
esp_err_t osh_node_proto_register_route(const char* uri,
                                        coap_request_t method,
                                        coap_route_cb handler);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_PROTO_H */
