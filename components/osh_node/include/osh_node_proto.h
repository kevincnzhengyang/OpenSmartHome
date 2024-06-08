/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-06-02 20:04:10
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-06-02 20:04:12
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_scoap.h
 * @Description : private protocol
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_PROTO_H
#define OSH_NODE_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"

#include "osh_node.h"
#include "osh_node_proto_dataframe.h"

#define OSH_NODE_PROTO_VER          0

#define OSH_ERR_PROTO_BASE              (OSH_ERR_NODE_BASE + 0x10000)
#define OSH_ERR_PROTO_INNER             (OSH_ERR_PROTO_BASE +     1)
#define OSH_ERR_PROTO_PDU_LEN           (OSH_ERR_PROTO_BASE +     2)
#define OSH_ERR_PROTO_BUFF_LEN          (OSH_ERR_PROTO_BASE +     3)
#define OSH_ERR_PROTO_SOCKET            (OSH_ERR_PROTO_BASE +     4)
#define OSH_ERR_PROTO_PDU_FMT           (OSH_ERR_PROTO_BASE +     5)
#define OSH_ERR_PROTO_INVALID_ENTRY     (OSH_ERR_PROTO_BASE +     6)
#define OSH_ERR_PROTO_NOT_FOUND         (OSH_ERR_PROTO_BASE +     7)


typedef esp_err_t (*osh_node_proto_handler_t) (uint32_t entry,
            osh_node_bb_t *node_bb,
            osh_node_proto_session_t *session,
            const osh_node_proto_pdu_t *request,
            osh_node_proto_pdu_t *response);

#define proto_response_err_head(req, rsp, cls, code) \
do {\
    (rsp)->version = OSH_NODE_PROTO_VER; \
    (rsp)->type = OSH_RESPONSE_RESET; \
    (rsp)->code_class = (cls); \
    (rsp)->code_code = (code); \
    (rsp)->token_ind = (req)->token_ind; \
    (rsp)->hash_ind = (req)->hash_ind; \
    (rsp)->con_type = OSH_CONTENT_OCTETS; \
    (rsp)->con_len = 0; \
} while(0)

#define proto_response_ack_head(req, rsp, cls, code) \
do {\
    (rsp)->version = OSH_NODE_PROTO_VER; \
    (rsp)->type = OSH_RESPONSE_ACK; \
    (rsp)->code_class = (cls); \
    (rsp)->code_code = (code); \
    (rsp)->token_ind = (req)->token_ind; \
    (rsp)->hash_ind = (req)->hash_ind; \
} while(0)


/* init proto */
esp_err_t osh_node_proto_init(osh_node_bb_t *node_bb, void *conf_arg);

/* fini proto */
esp_err_t osh_node_proto_fini(void);

/* start proto */
esp_err_t osh_node_proto_start(void *run_arg);

/* stop proto */
esp_err_t osh_node_proto_stop(void);

/* register route callback */
esp_err_t osh_node_route_register(uint32_t entry,
                                  OSH_CODE_METHOD_ENUM method,
                                  osh_node_proto_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_PROTO_H */
