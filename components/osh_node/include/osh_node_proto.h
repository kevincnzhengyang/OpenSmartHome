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

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"


#define OSH_ERR_PROTO_BASE              (OSH_ERR_APP_BASE + 0x10000)
#define OSH_ERR_PROTO_INNER             (OSH_ERR_PROTO_BASE +     1)


#define OSH_TOKEN_LEN                 8

/* type of request/response */
typedef enum {
    OSH_REQUEST_CONFIRM           =  0,
    OSH_REQUEST_NON_CONFIRM       =  1,
    OSH_RESPONSE_ACK              =  2,
    OSH_RESPONSE_RESET            =  3,
    OSH_REQRSP_BUTT
} OSH_REQRSP_TYPE_ENUM;

/* content type */
typedef enum {
    OSH_CONTENT_TEXT              =  0,
    OSH_CONTENT_HTML              =  1,
    OSH_CONTENT_XML               =  2,
    OSH_CONTENT_JSON              =  3,
    OSH_CONTENT_OCTETS            =  4,
    OSH_CONTENT_MPEG              =  5,
    OSH_CONTENT_JPEG              =  6,
    OSH_CONTENT_SVG               =  7,
    OSH_CONTENT_MP4               =  8,
    OSH_CONTENT_BUTT
} OSH_CONTENT_TYPE_ENUM;

/* class of code */
typedef enum {
    OSH_CC_METHOD                 =  0,
    OSH_CC_SUCCESS                =  2,
    OSH_CC_CLIENT_ERR             =  4,
    OSH_CC_SERVER_ERR             =  5,
    OSH_CC_SGINAL                 =  7,
    OSH_CC_BUTT
} OSH_CODE_CLASS_ENUM;

typedef enum {
    OSH_METHOC_EMPTY              =  0,
    OSH_METHOC_GET                =  1,
    OSH_METHOC_POST               =  2,
    OSH_METHOC_PUT                =  3,
    OSH_METHOC_DELETE             =  4,
    OSH_METHOC_FETCH              =  5,
    OSH_METHOC_PATCH              =  6,
    OSH_METHOC_IPATCH             =  7,
    OSH_METHOD_CONNECT            =  8, /* HTTP1.1 pip proxy */
    OSH_METHOD_OPTIONS            =  9, /* HTTP1.1 query server */
    OSH_METHOD_TRACE              = 10, /* HTTP1.1 diagoise server */
    OSH_METHOD_UPDATE             = 25, /* OTA */
    OSH_METHOD_BUTT
} OSH_CODE_METHOD_ENUM;

typedef enum {
    OSH_SUCCESS_CREATED           =  1,
    OSH_SUCCESS_DELETED           =  2,
    OSH_SUCCESS_VALID             =  3,
    OSH_SUCCESS_CHANGED           =  4,
    OSH_SUCCESS_CONTENT           =  5,
    OSH_SUCCESS_CONTINUE          =  6,
    OSH_SUCCESS_BUTT
} OSH_CODE_SUCCESS_ENUM;

typedef enum {
    OSH_CERR_BAD_REQUEST          =  0,
    OSH_CERR_UNAUTHORIZED         =  1,
    OSH_CERR_BAD_OPTION           =  2,
    OSH_CERR_FORBIDDEN            =  3,
    OSH_CERR_NOT_FOUND            =  4,
    OSH_CERR_METHOD_NOT_ALLOWED   =  5,
    OSH_CERR_NOT_ACCEPTABLE       =  6,
    OSH_CERR_ENTITY_INCOMPLETE    =  8,
    OSH_CERR_CONFLICT             =  9,
    OSH_CERR_PRECONDITION_FAILED  = 12,
    OSH_CERR_ENTITY_TOO_LARGE     = 13,
    OSH_CERR_UNSUPPORTED_FORMAT   = 15,
    OSH_CERR_BUTT
} OSH_CODE_ERR_CLIENT_ENUM;

typedef enum {
    OSH_SERR_INTERNAL_ERR         =  0,
    OSH_SERR_NOT_IMPLEMENTED      =  1,
    OSH_SERR_BAD_GATEWAY          =  2,
    OSH_SERR_UNAVAILABLE          =  3,
    OSH_SERR_TIMEOUT              =  4,
    OSH_SERR_BUTT
} OSH_CODE_ERR_SERVER_ENUM;

typedef enum {
    OSH_SIGNAL_UNASSIGNED         =  0,
    OSH_SIGNAL_CSM                =  1,
    OSH_SIGNAL_PING               =  2,
    OSH_SIGNAL_PONG               =  3,
    OSH_SIGNAL_RELEASE            =  4,
    OSH_SIGNAL_ABORT              =  5,
    OSH_SIGNAL_BUTT
} OSH_CODE_SIGNAL_ENUM;

/* message format */
typedef struct {
    uint8_t              request_class;
    uint8_t               request_code;
    uint8_t             response_class;
    uint8_t              response_code;

    uint8_t               request_type;
    uint8_t              response_type;
    uint16_t                    msg_id;

    uint8_t                  token_len;
    uint8_t               content_type;
    uint16_t               content_len;

    uint8_t       token[OSH_TOKEN_LEN];
    uint8_t                   *content;
} osh_node_proto_msg;

/* callback for proto */
typedef esp_err_t (* buff_malloc)(void **pbuff, size_t size);
typedef esp_err_t (* buff_free)(void *buff);
typedef esp_err_t (* init_proto)(void *arg);
typedef esp_err_t (* fini_proto)(void);
typedef esp_err_t (* reset_proto)(void);
typedef esp_err_t (* new_msg)(osh_node_proto_msg **pmsg, size_t token_len,
                        size_t delta_len, size_t value_len, size_t payload_len);
typedef esp_err_t (* free_msg)(osh_node_proto_msg *msg);
typedef esp_err_t (* decode_msg)(uint8_t *buff, size_t len, void *arg);
typedef esp_err_t (* encode_msg)(osh_node_proto_msg *msg,
                        uint8_t *buff, size_t buff_size, size_t *len);

/* Protocol */
typedef struct {
    buff_malloc                 malloc;
    buff_free                     free;
    init_proto                    init;
    fini_proto                    fini;
    reset_proto                  reset;
    new_msg                    new_msg;
    free_msg                  free_msg;
    decode_msg                  decode;
    encode_msg                  encode;
    osh_node_proto_msg            *msg;     // context
} osh_node_proto;

osh_node_proto *create_default_proto(void);

/* register request handler for state OSH_FSM_STATE_IDLE and OSH_FSM_STATE_ONGOING */
typedef esp_err_t (* handle_request)(osh_node_proto_msg *msg, void *arg);
esp_err_t register_request(uint8_t request_code, handle_request handler);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_PROTO_H */
