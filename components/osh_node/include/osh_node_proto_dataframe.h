/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-06-02 20:10:45
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-06-02 20:10:47
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_proto_dataframe.h
 * @Description : private protocol data frame definitions
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_PROTO_DATAFRAME_H
#define OSH_NODE_PROTO_DATAFRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#define OSH_TOKEN_MAX_LEN              4

/* type of request/response */
typedef enum {
    OSH_REQUEST_CONFIRM            =  0,
    OSH_REQUEST_NON_CONFIRM        =  1,
    OSH_RESPONSE_ACK               =  2,
    OSH_RESPONSE_RESET             =  3,
    OSH_PDU_BUTT
} OSH_PDU_TYPE_ENUM;

/* class of code */
typedef enum {
    OSH_CC_METHOD                  =  0,
    OSH_CC_SUCCESS                 =  2,
    OSH_CC_CLIENT_ERR              =  4,
    OSH_CC_SERVER_ERR              =  5,
    OSH_CC_SGINAL                  =  7,
    OSH_CC_BUTT
} OSH_CODE_CLASS_ENUM;

typedef enum {
    OSH_METHOD_EMPTY               =  0,
    OSH_METHOD_GET                 =  1,
    OSH_METHOD_POST                =  2,
    OSH_METHOD_PUT                 =  3,
    OSH_METHOD_DELETE              =  4,
    OSH_METHOD_FETCH               =  5,
    OSH_METHOD_PATCH               =  6,
    OSH_METHOD_IPATCH              =  7,
    OSH_METHOD_CONNECT             =  8, /* HTTP1.1 pip proxy */
    OSH_METHOD_OPTIONS             =  9, /* HTTP1.1 query server */
    OSH_METHOD_TRACE               = 10, /* HTTP1.1 diagoise server */
    OSH_METHOD_BUTT
} OSH_CODE_METHOD_ENUM;

typedef enum {
    OSH_SUCCESS_CREATED            =  1,
    OSH_SUCCESS_DELETED            =  2,
    OSH_SUCCESS_VALID              =  3,
    OSH_SUCCESS_CHANGED            =  4,
    OSH_SUCCESS_CONTENT            =  5,
    OSH_SUCCESS_CONTINUE           = 31,
    OSH_SUCCESS_BUTT
} OSH_CODE_SUCCESS_ENUM;

typedef enum {
    OSH_CERR_BAD_REQUEST           =  0,
    OSH_CERR_UNAUTHORIZED          =  1,
    OSH_CERR_BAD_OPTION            =  2,
    OSH_CERR_FORBIDDEN             =  3,
    OSH_CERR_NOT_FOUND             =  4,
    OSH_CERR_METHOD_NOT_ALLOWED    =  5,
    OSH_CERR_NOT_ACCEPTABLE        =  6,
    OSH_CERR_ENTITY_INCOMPLETE     =  8,
    OSH_CERR_CONFLICT              =  9,
    OSH_CERR_PRECONDITION_FAILED   = 12,
    OSH_CERR_ENTITY_TOO_LARGE      = 13,
    OSH_CERR_UNSUPPORTED_FORMAT    = 15,
    OSH_CERR_BUTT
} OSH_CODE_ERR_CLIENT_ENUM;

typedef enum {
    OSH_SERR_INTERNAL_ERR          =  0,
    OSH_SERR_NOT_IMPLEMENTED       =  1,
    OSH_SERR_BAD_GATEWAY           =  2,
    OSH_SERR_UNAVAILABLE           =  3,
    OSH_SERR_TIMEOUT               =  4,
    OSH_SERR_BUTT
} OSH_CODE_ERR_SERVER_ENUM;

typedef enum {
    OSH_SIGNAL_UNASSIGNED          =  0,
    OSH_SIGNAL_CSM                 =  1,
    OSH_SIGNAL_PING                =  2,
    OSH_SIGNAL_PONG                =  3,
    OSH_SIGNAL_RELEASE             =  4,
    OSH_SIGNAL_ABORT               =  5,
    OSH_SIGNAL_SHAKEHAND           = 24, /* secure connect */
    OSH_SIGNAL_UPDATE              = 25, /* OTA */
    OSH_SIGNAL_HEARTBEAT           = 26,
    OSH_SIGNAL_BUTT
} OSH_CODE_SIGNAL_ENUM;

/* content type */
typedef enum {
    OSH_CONTENT_TEXT               =  0,
    OSH_CONTENT_HTML               =  1,
    OSH_CONTENT_XML                =  2,
    OSH_CONTENT_JSON               =  3,
    OSH_CONTENT_OCTETS             =  4,
    OSH_CONTENT_MPEG               =  5,
    OSH_CONTENT_JPEG               =  6,
    OSH_CONTENT_SVG                =  7,
    OSH_CONTENT_MP4                =  8,
    OSH_CONTENT_BUTT
} OSH_CONTENT_TYPE_ENUM;

/* session state */
typedef enum {
    OSH_SESSION_STATE_NONE         =  0,
    OSH_SESSION_STATE_CONNECTING   =  1,
    OSH_SESSION_STATE_HANDSHAKE    =  2,
    OSH_SESSION_STATE_CSM          =  3,
    OSH_SESSION_STATE_ESTABLISHED  =  4,
    OSH_SESSION_STATE_BUTT
} OSH_SESSION_STATE_ENUM;

/* session */
typedef struct {
    OSH_SESSION_STATE_ENUM        state;
    uint32_t                        ref;    // reference counter
    struct sockaddr_in      remote_addr;
    struct sockaddr_in       local_addr;
    int                            sock;    // socket
    uint32_t                ack_timeout;    // tick for ack timeout
    uint32_t                 last_token;
    uint16_t               last_ack_mid;
    uint16_t               last_con_mid;
} osh_node_proto_session_t;

/* pdu */
typedef struct {
    uint8_t                     version;
    OSH_PDU_TYPE_ENUM              type;
    uint8_t                   token_ind;    // token indicator
    uint8_t                    hash_ind;    // hash indicator
    uint8_t                   entry_ind;    // hash indicator
    OSH_CODE_CLASS_ENUM      code_class;    // 3 bits class
    uint8_t                   code_code;    // 5 bits code
    uint16_t                        mid;    // message ID
    OSH_CONTENT_TYPE_ENUM      con_type;    // content type
    uint32_t                    con_len;    // content length
    uint32_t                      token;
    uint32_t                       hash;
    uint32_t                      entry;
    void                          *data;    // data for App

    osh_node_proto_session_t   *session;

    uint8_t                     *octets;    // octets for sending or receiving
    size_t                  octets_size;    // size of octets
    uint8_t                     *oct_rd;    // read ptr for octets
    uint8_t                     *oct_wr;    // write ptr for octets
} osh_node_proto_pdu_t;

// minimal pdu length
#define OSH_NODE_PROTO_PDU_HEADER_MIN_LEN     8

// max pdu header length
#define OSH_NODE_PROTO_PDU_HEADER_MAX_LEN    24

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_PROTO_DATAFRAME_H */

