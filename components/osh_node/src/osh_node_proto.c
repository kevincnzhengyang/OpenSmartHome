/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-08 19:20:33
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-08 23:53:33
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_proto.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "osh_node_proto.h"

#include <freertos/list.h>

static const char *PROTO_TAG = "COAP";

#define OSH_COAP_VERSION    1

/* decode from octects */
#define GET_VERSION(data) ((0xC0 & (data)[0]) >> 6)
#define GET_TYPE(data) ((0x30 & (data)[0]) >> 4)
#define GET_TOKEN_LEN(data) (0x0F & (data)[0])
#define GET_CLASS(data) (((data)[1] >> 5) & 0x07)
#define GET_CODE(data)  ((data)[1] & 0x1F)
#define GET_MSG_ID(data) (((data)[2] << 8) | (data)[3])
#define GET_CONTENT_TYPE(data) ((data)[4])
#define GET_CONTENT_LEN(data) (((data)[5] << 16) | ((data)[6] << 8) | (data)[7])
#define GET_TOKEN_POS(data) ((uint8_t *)&(data)[8])
#define GET_CONTENT_POS(data) ((uint8_t *)&(data[8+GET_TOKEN_LEN(data)]))

/* encode to octects */
#define SET_VERSION(data, ver) ((data)[0] |= (((ver) & 0x03) << 6))
#define SET_TYPE(data, type) ((data)[0] |= (((type) & 0x03) << 4))
#define SET_TOKEN_LEN(data, len) ((data)[0] |= ((len) & 0x0F))
#define SET_CLASS(data, cls) ((data)[1] |= (((cls) & 0x07) << 5))
#define SET_CODE(data, code) ((data)[1] |= ((code) & 0x1F))
#define SET_MSG_ID(data, id) ((data)[2] = (((id) & 0xFF00) >> 8), (data)[3] = ((id) & 0xFF))
#define SET_CONTENT_TYPE(data, type) ((data)[4] = ((type) & 0xFF))
#define SET_CONTENT_LEN(data, len) ((data)[5] = (((len) & 0xFF0000) >> 16), (data)[6] = (((len) & 0xFF00) >> 8), (data)[7] = ((len) & 0xFF))

/* request handler item */
typedef struct {
    uint8_t               request_code;
    handle_request             handler;
    ListItem_t            request_item;
} osh_request_handler_stru;

typedef struct {
    int32_t                   finished;     // flag for session finished
    uint32_t                  sequence;
    List_t                    handlers;
    void                          *arg;
} osh_coap_proto;

static osh_coap_proto g_coap_proto;

static esp_err_t osh_coap_buff_malloc(void **pbuff, size_t size) {
    *pbuff = malloc(size);
    if (NULL == *pbuff) {
        ESP_LOGE(PROTO_TAG, "failed to malloc");
        return ESP_ERR_NO_MEM;
    }
    memset(pbuff, 0, size);
    return ESP_OK;
}

static esp_err_t osh_coap_buff_free(void *buff) {
    if (NULL != buff) free(buff);
    return ESP_OK;
}

static esp_err_t osh_coap_init_proto(void *arg) {
    memset(&g_coap_proto, 0, sizeof(osh_coap_proto));
    g_coap_proto.finished = 1;
    vListInitialise(&g_coap_proto.handlers);
    g_coap_proto.arg = arg;
    return ESP_OK;
}

static esp_err_t osh_coap_fini_proto(void) {
    if (!listLIST_IS_EMPTY(&g_coap_proto.handlers)) {
        // not empty list
        osh_request_handler_stru *handler = NULL;
        listFOR_EACH_ENTRY(&g_coap_proto.handlers, osh_request_handler_stru, handler) {
            ESP_LOGI(PROTO_TAG, "free request %d handler",
                    (int)handler->request_code);
            uxListRemove(&handler->request_item);   // remove from list
        }
    }
    return ESP_OK;
}

static esp_err_t osh_coap_reset_proto(void) {
    g_coap_proto.finished = 1;
    g_coap_proto.sequence++;
    return ESP_OK;
}

static esp_err_t osh_coap_new_msg(osh_node_proto_msg **pmsg, size_t token_len,
                        size_t delta_len, size_t value_len, size_t payload_len) {

    return ESP_OK;
}

static esp_err_t osh_coap_free_msg(osh_node_proto_msg *msg) {
    return ESP_OK;
}

static esp_err_t osh_coap_decode_msg(uint8_t *buff, size_t len, void *arg) {
    return ESP_OK;
}

static esp_err_t osh_coap_encode_msg(osh_node_proto_msg *msg,
                        uint8_t *buff, size_t buff_size, size_t *len) {
    return ESP_OK;
}


osh_node_proto *create_default_proto(void) {
    osh_node_proto_msg *msg = malloc(sizeof(osh_node_proto_msg));
    if (NULL == msg) {
        ESP_LOGE(PROTO_TAG, "failed to malloc msg");
        return NULL;
    }
    osh_node_proto *proto = malloc(sizeof(osh_node_proto));
    if (NULL == proto) {
        ESP_LOGE(PROTO_TAG, "failed to malloc proto");
        goto clear;
    }
    proto->malloc = osh_coap_buff_malloc;
    proto->free = osh_coap_buff_free;
    proto->init = osh_coap_init_proto;
    proto->fini = osh_coap_fini_proto;
    proto->reset = osh_coap_reset_proto;
    proto->new_msg = osh_coap_new_msg;
    proto->free_msg = osh_coap_free_msg;
    proto->decode = osh_coap_decode_msg;
    proto->encode = osh_coap_encode_msg;
    return proto;
clear:
    if (NULL != msg) {
        free(msg);
    }
    return NULL;
}

esp_err_t register_request(uint8_t request_code, handle_request handler) {
    // create
    osh_request_handler_stru *req_handler = malloc(sizeof(osh_request_handler_stru));
    if (NULL == req_handler) {
        ESP_LOGE(PROTO_TAG, "failedto malloc request %d", (int)request_code);
        return ESP_ERR_NO_MEM;
    }

    // init
    memset(req_handler, 0, sizeof(osh_request_handler_stru));
    vListInitialiseItem(&req_handler->request_item);
    listSET_LIST_ITEM_OWNER(&req_handler->request_item, req_handler);
    req_handler->request_code = request_code;
    req_handler->handler = handler;

    // insert to list
    req_handler->request_item.xItemValue = (int)request_code;  // using bits as value
    vListInsert(&g_coap_proto.handlers, &req_handler->request_item);

    ESP_LOGI(PROTO_TAG, "create handler for request %d", (int)request_code);
    return ESP_OK;
}
