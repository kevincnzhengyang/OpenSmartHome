/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-06-02 20:04:36
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-06-08 22:03:54
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_proto.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "esp_wifi.h"

#include "osh_node_proto.h"
#include "osh_node_proto.inc"
#include "osh_node_proto_dataframe.h"

static const char *PROTO_TAG = "PROTO";

static osh_node_proto_t g_proto;

/** -------------------------------
 *            proto
 *  -------------------------------
*/
static esp_err_t proto_decode_pdu(osh_node_proto_session_t *session,
                        osh_node_proto_pdu_t *pdu,
                        uint8_t *buff,
                        size_t  buff_len) {
    if (NULL == pdu || NULL == buff) {
        ESP_LOGE(PROTO_TAG, "invalid param");
        return OSH_ERR_PROTO_INNER;
    }

    if (OSH_NODE_PROTO_PDU_HEADER_MIN_LEN > buff_len) {
        ESP_LOGE(PROTO_TAG, "invalid buff len %d", buff_len);
        return OSH_ERR_PROTO_PDU_LEN;
    }

    // init
    memset(pdu, 0, sizeof(osh_node_proto_pdu_t));
    pdu->octets = buff;
    pdu->octets_size = buff_len;
    pdu->oct_rd = buff;
    pdu->oct_wr = &buff[buff_len];
    pdu->session = session;

    // decode
    int offset = 0;
    pdu->version = (uint8_t)((buff[0] & 0xC0) >> 6);
    pdu->type = (OSH_PDU_TYPE_ENUM) ((buff[0] & 0x30) >> 4);
    // TODO
    if (OSH_REQUEST_CONFIRM == pdu->type) {
        // set ack timeout
        session->ack_timeout = 2000;
    }
    pdu->token_ind = (uint8_t)((buff[0] & 0x04) >> 2);
    pdu->hash_ind = (uint8_t)((buff[0] & 0x02) >> 1);
    pdu->entry_ind = (uint8_t)(buff[0] & 0x01);
    pdu->code_class = (OSH_CODE_CLASS_ENUM) ((buff[1] & 0xE0) >> 5);
    if (OSH_CC_BUTT <= pdu->code_class) {
        ESP_LOGE(PROTO_TAG, "invalid code class %d", (int)pdu->code_class);
        return OSH_ERR_PROTO_PDU_FMT;
    }
    pdu->code_code = buff[1] & 0x1F;
    pdu->mid = (uint16_t)((buff[2] << 8) | buff[3]);
    pdu->con_type = (OSH_CONTENT_TYPE_ENUM)buff[4];
    if (OSH_CONTENT_BUTT <= pdu->con_type) {
        ESP_LOGE(PROTO_TAG, "invalid content type %d", (int)pdu->con_type);
        return OSH_ERR_PROTO_PDU_FMT;
    }
    pdu->con_len = (uint32_t)((buff[5]<<16) | (buff[6] << 8) | buff[7]);
    offset += 8;
    if (0 != pdu->token_ind) {
        // decode token
        pdu->token = (uint32_t)((buff[offset]<<24) | (buff[offset+1] << 16)
                        | (buff[offset+2] << 8) | buff[offset+3]);
        if (2 > (uint8_t)pdu->type) {
            // request, save last token
            session->last_token = pdu->token;
        }
        offset += 4;
    }
    if (0 != pdu->hash_ind) {
        // decode hash
        pdu->hash = (uint32_t)((buff[offset]<<24) | (buff[offset+1] << 16)
                        | (buff[offset+2] << 8) | buff[offset+3]);
        offset += 4;
    }
    if (0 != pdu->entry_ind) {
        // decode entry
        pdu->entry = (uint32_t)((buff[offset]<<24) | (buff[offset+1] << 16)
                        | (buff[offset+2] << 8) | buff[offset+3]);
        offset += 4;
    }
    // set read ptr for app
    pdu->oct_rd += offset;

    if (offset + pdu->con_len != buff_len) {
        ESP_LOGE(PROTO_TAG, "invalid PDU length [%d] != [%d]",
                offset + (int)(pdu->con_len), buff_len);
        return OSH_ERR_PROTO_PDU_LEN;
    }
    return ESP_OK;
}

static void proto_make_token(osh_node_proto_pdu_t *pdu) {
    // TODO
    if (NULL == pdu) return;
    if (1 < (uint8_t)pdu->type) {
        pdu->token = pdu->session->last_token;
    } else {
        pdu->token = (uint32_t)pdu->mid;
    }
}

static void proto_make_hash(osh_node_proto_pdu_t *pdu) {
    if (NULL == pdu) return;
    pdu->hash = (uint32_t)pdu->mid;
}

static void proto_init_response(osh_node_proto_session_t *session,
                        osh_node_proto_pdu_t *pdu,
                        uint8_t *buff,
                        size_t  buff_len) {
    if (NULL == pdu || NULL == buff) return;

    memset(pdu, 0, sizeof(osh_node_proto_pdu_t));
    pdu->type = OSH_PDU_BUTT;
    pdu->session = session;
    pdu->octets = buff;
    pdu->octets_size = buff_len;
    pdu->oct_rd = pdu->oct_wr = buff;
}

static esp_err_t proto_encode_pdu(osh_node_proto_session_t *session,
                        osh_node_proto_pdu_t *pdu,
                        uint8_t *buff,
                        size_t  buff_len) {
    if (NULL == pdu || NULL == buff) {
        ESP_LOGE(PROTO_TAG, "invalid param");
        return OSH_ERR_PROTO_INNER;
    }

    if (OSH_NODE_PROTO_PDU_HEADER_MIN_LEN > buff_len) {
        ESP_LOGE(PROTO_TAG, "encode buff overflow [%d]", buff_len);
        return OSH_ERR_PROTO_BUFF_LEN;
    }

    // get mid
    pdu->mid = (uint16_t)(session->ref++);
    if (1 < (uint8_t)pdu->type && 0 != pdu->token_ind) {
        // request, create token
        proto_make_token(pdu);
    }
    if (0 != pdu->hash_ind) {
        proto_make_hash(pdu);
    }

    // encode
    int offset = 0;
    buff[0] = (uint8_t) (((pdu->version & 0x03) << 6) |
                        ((pdu->type & 0x03) << 4) |
                        ((pdu->token_ind & 0x01) << 2) |
                        ((pdu->hash_ind & 0x01) << 1) |
                        (pdu->entry_ind & 0x01));
    buff[1] = (((uint8_t)pdu->code_class & 0x03) << 5) |
                        (pdu->code_code & 0x1F);
    buff[2] = (uint8_t)((pdu->mid & 0xFF00) >> 8);
    buff[3] = (uint8_t)(pdu->mid & 0xFF);
    buff[4] = (uint8_t)pdu->con_type;
    buff[5] = (uint8_t)((pdu->con_len & 0xFF0000) >> 16);
    buff[6] = (uint8_t)((pdu->con_len & 0xFF00) >> 8);
    buff[7] = (uint8_t)(pdu->con_len & 0xFF);
    offset += 8;
    if (0 != pdu->token_ind) {
        buff[offset] = (uint8_t)((pdu->token & 0xFF000000) >> 24);
        buff[offset+1] = (uint8_t)((pdu->token & 0xFF0000) >> 16);
        buff[offset+2] = (uint8_t)((pdu->token & 0xFF00) >> 8);
        buff[offset+3] = (uint8_t)(pdu->token & 0xFF);
        offset += 4;
    }
    if (0 != pdu->hash_ind) {
        buff[offset] = (uint8_t)((pdu->hash & 0xFF000000) >> 24);
        buff[offset+1] = (uint8_t)((pdu->hash & 0xFF0000) >> 16);
        buff[offset+2] = (uint8_t)((pdu->hash & 0xFF00) >> 8);
        buff[offset+3] = (uint8_t)(pdu->hash & 0xFF);
        offset += 4;
    }
    if (0 != pdu->entry_ind) {
        buff[offset] = (uint8_t)((pdu->entry & 0xFF000000) >> 24);
        buff[offset+1] = (uint8_t)((pdu->entry & 0xFF0000) >> 16);
        buff[offset+2] = (uint8_t)((pdu->entry & 0xFF00) >> 8);
        buff[offset+3] = (uint8_t)(pdu->entry & 0xFF);
        offset += 4;
    }
    if (offset + pdu->con_len > buff_len) {
        ESP_LOGE(PROTO_TAG, "buff overflow [%ld]>[%d]", offset+pdu->con_len, buff_len);
        return OSH_ERR_PROTO_BUFF_LEN;
    }
    if (0 < pdu->con_len) memcpy(&buff[offset], pdu->data, pdu->con_len);
    pdu->oct_wr += (offset + pdu->con_len);

    return ESP_OK;
}

#define proto_get_code_class(pdu) \
    ((OSH_CODE_CLASS_ENUM)((((osh_node_proto_pdu_t *)(pdu))->code & 0xE0) >> 5))

#define proto_get_code_code(pdu) \
    ((uint8_t)(((osh_node_proto_pdu_t *)(pdu))->code & 0x1F))

#define proto_make_code(class, code) \
    ((uint8_t)(((((uint8_t)(class)) & 0x07) << 5) | (((uint8_t)(code)) & 0x1F)))

/** -------------------------------
 *            task
 *  -------------------------------
*/
static void hb_timeout_cb(TimerHandle_t timer) {
    // TODO
    // broadcast heartbeat
}

static esp_err_t decode_pdu(void) {
    // init rsp
    proto_init_response(&g_proto.session, &g_proto.response,
                        g_proto.send_buff.base, g_proto.send_buff.size);

    // decode request
    esp_err_t res = proto_decode_pdu(&g_proto.session, &g_proto.request,
                        g_proto.recv_buff.base, g_proto.recv_buff.len);

    if (ESP_OK != res) {
        // bad request
        osh_node_proto_pdu_t *rsp = &g_proto.response;
        osh_node_proto_pdu_t *req = &g_proto.request;
        ESP_LOGE(PROTO_TAG, "bad request.[0x%x]", req->mid);
        proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_BAD_REQUEST);
    }

    return ESP_OK;
}

static void response_remote(int sock, struct sockaddr_in *addr) {
    esp_err_t err = proto_encode_pdu(&g_proto.session, &g_proto.response,
                    g_proto.send_buff.base, g_proto.send_buff.size);
    if (ESP_OK != err) {
        ESP_LOGE(PROTO_TAG, "failed to encode response. err:%d", err);
        return;
    } else {
        // set length to be sent
        g_proto.send_buff.len = g_proto.response.oct_wr - g_proto.response.oct_rd;
    }
    // send to remote
    if (0 > sendto(sock, g_proto.send_buff.base, g_proto.send_buff.len,
                    0, (struct sockaddr *)addr, sizeof(struct sockaddr_in))) {
        ESP_LOGE(PROTO_TAG, "failed to send response to %s", inet_ntoa(addr->sin_addr));
    } else {
        ESP_LOGI(PROTO_TAG, "reponse to %s", inet_ntoa(addr->sin_addr));
    }
}

static esp_err_t handle_mdm_pdu(void) {
    osh_node_proto_pdu_t *req = &g_proto.request;
    osh_node_proto_pdu_t *rsp = &g_proto.response;

    if (1 < req->type) {
        // not a request
        ESP_LOGE(PROTO_TAG, "receive pdu not request %d. [0x%x]",
                (int)req->type, req->mid);
        proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_BAD_REQUEST);
        return OSH_ERR_PROTO_PDU_FMT;
    }
    if (1 == req->entry_ind && !OSH_NODE_ENTRY_IS_MDM(req->entry)) {
        // has entry not belong to MDM
        ESP_LOGE(PROTO_TAG, "invalid MDM entry 0x%lx. [0x%x]",
                req->entry, req->mid);
        proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_FORBIDDEN);
        rsp->con_len = sizeof(req->entry);
        rsp->data = &req->entry;
        return OSH_ERR_PROTO_INVALID_ENTRY;
    }

    if (OSH_CC_SGINAL == req->code_class) {
        // only handle PING in MDM
        if (OSH_SIGNAL_PING == req->code_code) {
            ESP_LOGI(PROTO_TAG, "MDM Ping. [0x%x]", req->mid);
            rsp->version = OSH_NODE_PROTO_VER;
            rsp->type = OSH_RESPONSE_ACK;
            rsp->code_class = OSH_CC_SGINAL;
            rsp->code_code = OSH_SIGNAL_PONG;
            rsp->token_ind = req->token_ind;
            rsp->hash_ind = req->hash_ind;
            rsp->con_type = OSH_CONTENT_OCTETS;
            rsp->con_len = 0;
            return ESP_OK;
        }
    } else if (OSH_CC_METHOD == req->code_class) {
        // method
        if (1 != req->entry_ind) {
            ESP_LOGE(PROTO_TAG, "MDM method without entry. [0x%x]", req->mid);
            proto_response_err_head(req, rsp,
                    OSH_CC_CLIENT_ERR, OSH_CERR_ENTITY_INCOMPLETE);
            return OSH_ERR_PROTO_INVALID_ENTRY;
        }

        // search and call entry callback
        osh_node_proto_entry_t *entry = NULL;
        listFOR_EACH_ENTRY(&g_proto.entry_list, osh_node_proto_entry_t, entry) {
            if (req->entry == entry->entry) {
                osh_node_proto_route_t *route = NULL;
                listFOR_EACH_ENTRY(&entry->route_list, osh_node_proto_route_t, route) {
                    if (req->code_code == route->method) {
                        ESP_LOGI(PROTO_TAG, "MDM matched route. method %d@0x%lx. [0x%x]",
                                req->code_code, req->entry, req->mid);
                        if (NULL != route->route_cb) {
                            return route->route_cb(req->entry, g_proto.node_bb,
                                        &g_proto.session, req, rsp);
                        }
                    }
                }
            }
        }

        // not match
        ESP_LOGW(PROTO_TAG, "MDM not matched route.method %d@0x%lx. [0x%x]",
                req->code_code, req->entry, req->mid);
        proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_NOT_FOUND);
        return OSH_ERR_PROTO_NOT_FOUND;
    }

    // can't handle
    proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_NOT_FOUND);
    return OSH_ERR_PROTO_NOT_FOUND;
}

static esp_err_t handle_app_pdu(void) {
    osh_node_proto_pdu_t *req = &g_proto.request;
    osh_node_proto_pdu_t *rsp = &g_proto.response;

    if (1 < req->type) {
        // not a request
        ESP_LOGE(PROTO_TAG, "receive pdu not request %d. [0x%x]",
                (int)req->type, req->mid);
        proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_BAD_REQUEST);
        return OSH_ERR_PROTO_PDU_FMT;
    }
    if (1 == req->entry_ind && !OSH_NODE_ENTRY_IS_APP(req->entry)) {
        // has entry not belong to APP
        ESP_LOGE(PROTO_TAG, "invalid APP entry 0x%lx. [0x%x]",
                req->entry, req->mid);
        proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_FORBIDDEN);
        rsp->con_len = sizeof(req->entry);
        rsp->data = &req->entry;
        return OSH_ERR_PROTO_INVALID_ENTRY;
    }

    if (OSH_CC_SGINAL == req->code_class) {
        if (OSH_SIGNAL_SHAKEHAND == req->code_code) {
            ESP_LOGI(PROTO_TAG, "APP shakehand. [0x%x]", req->mid);
            // todo exchange the key
            rsp->version = OSH_NODE_PROTO_VER;
            rsp->type = OSH_RESPONSE_ACK;
            rsp->code_class = OSH_CC_SGINAL;
            rsp->code_code = OSH_SIGNAL_PONG;
            rsp->token_ind = req->token_ind;
            rsp->hash_ind = req->hash_ind;
            rsp->con_type = OSH_CONTENT_OCTETS;
            rsp->con_len = 0;
            return ESP_OK;
        } else if (OSH_SIGNAL_UPDATE == req->code_code) {
            ESP_LOGI(PROTO_TAG, "APP update. [0x%x]", req->mid);
            // todo update node
            return ESP_OK;
        }
    } else if (OSH_CC_METHOD == req->code_class) {
        // method
        if (1 != req->entry_ind) {
            ESP_LOGE(PROTO_TAG, "APP method without entry. [0x%x]", req->mid);
            proto_response_err_head(req, rsp,
                    OSH_CC_CLIENT_ERR, OSH_CERR_ENTITY_INCOMPLETE);
            return OSH_ERR_PROTO_INVALID_ENTRY;
        }

        // search and call entry callback
        osh_node_proto_entry_t *entry = NULL;
        listFOR_EACH_ENTRY(&g_proto.entry_list, osh_node_proto_entry_t, entry) {
            if (req->entry == entry->entry) {
                osh_node_proto_route_t *route = NULL;
                listFOR_EACH_ENTRY(&entry->route_list, osh_node_proto_route_t, route) {
                    if (req->code_code == route->method) {
                        ESP_LOGI(PROTO_TAG, "APP matched route. method %d@0x%lx. [0x%x]",
                                req->code_code, req->entry, req->mid);
                        if (NULL != route->route_cb) {
                            return route->route_cb(req->entry, g_proto.node_bb,
                                        &g_proto.session, req, rsp);
                        }
                    }
                }
            }
        }

        // not match
        ESP_LOGW(PROTO_TAG, "APP not matched route.method %d@0x%lx. [0x%x]",
                req->code_code, req->entry, req->mid);
        proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_NOT_FOUND);
        return OSH_ERR_PROTO_NOT_FOUND;
    }

    // can't handle
    proto_response_err_head(req, rsp, OSH_CC_CLIENT_ERR, OSH_CERR_NOT_FOUND);
    return OSH_ERR_PROTO_NOT_FOUND;
}

static void proto_server(void * arg) {
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(g_proto.mdm_sock, &read_fds);
        FD_SET(g_proto.app_sock, &read_fds);

        int max_sd = g_proto.mdm_sock > g_proto.app_sock ? g_proto.mdm_sock : g_proto.app_sock;
        struct timeval timeout = {10, 0}; // 10 seconds timeout

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, &timeout);

        if ((activity < 0) && (errno != EINTR)) {
            ESP_LOGE(PROTO_TAG, "select error: errno %d", errno);
        }

        if (FD_ISSET(g_proto.mdm_sock, &read_fds)) {
            socklen_t socklen = sizeof(struct sockaddr_in);
            int len = recvfrom(g_proto.mdm_sock, g_proto.recv_buff.base,
                        g_proto.recv_buff.size, 0,
                        (struct sockaddr *)&g_proto.session.remote_addr,
                        &socklen);

            if (len < 0) {
                ESP_LOGE(PROTO_TAG, "recvfrom (multicast) failed: errno %d", errno);
            } else {
                ESP_LOGI(PROTO_TAG, "MDM Received %d bytes from %s:",
                         len, inet_ntoa(g_proto.session.remote_addr.sin_addr));
                g_proto.recv_buff.len = len;
                if (ESP_OK == decode_pdu()) {
                    if (ESP_OK != handle_mdm_pdu() ||
                        OSH_REQUEST_CONFIRM == g_proto.request.type) {
                        // response when need confirm
                        response_remote(g_proto.mdm_sock, &g_proto.session.remote_addr);
                    }
                } else {
                    response_remote(g_proto.mdm_sock, &g_proto.session.remote_addr);
                }
            }
        }

        if (FD_ISSET(g_proto.app_sock, &read_fds)) {
            socklen_t socklen = sizeof(struct sockaddr_in);
            int len = recvfrom(g_proto.app_sock, g_proto.recv_buff.base,
                        g_proto.recv_buff.size, 0,
                        (struct sockaddr *)&g_proto.session.remote_addr,
                        &socklen);

            if (len < 0) {
                ESP_LOGE(PROTO_TAG, "recvfrom (unicast) failed: errno %d", errno);
            } else {
                ESP_LOGI(PROTO_TAG, "APP Received %d bytes from %s:",
                        len, inet_ntoa(g_proto.session.remote_addr.sin_addr));
                g_proto.recv_buff.len = len;
                if (ESP_OK == decode_pdu()) {
                    if (ESP_OK != handle_app_pdu() ||
                        OSH_REQUEST_CONFIRM == g_proto.request.type) {
                        // response when need confirm
                        response_remote(g_proto.app_sock, &g_proto.session.remote_addr);
                    }
                } else {
                    // response bad request
                    response_remote(g_proto.app_sock, &g_proto.session.remote_addr);
                }
            }
        }
    }

    vTaskDelete(NULL);
}

/** -------------------------------
 *            functions
 *  -------------------------------
*/

/* init proto */
esp_err_t osh_node_proto_init(osh_node_bb_t *node_bb, void * conf_arg) {
    memset(&g_proto, 0, sizeof(osh_node_proto_t));
    g_proto.report_sock = -1;
    g_proto.mdm_sock = -1;
    g_proto.app_sock = -1;
    g_proto.report_addr.sin_family = AF_INET;
    g_proto.report_addr.sin_port = htons(CONFIG_NODE_PROTO_REPORT_PORT);
    g_proto.report_addr.sin_addr.s_addr = inet_addr(CONFIG_NODE_PROTO_REPORT_ADDR);

    g_proto.node_bb = node_bb;
    g_proto.recv_buff.size = CONFIG_NODE_PROTO_BUFF_SIZE;
    g_proto.recv_buff.base = malloc(CONFIG_NODE_PROTO_BUFF_SIZE);
    if (NULL == g_proto.recv_buff.base) {
        ESP_LOGE(PROTO_TAG, "failedto malloc mem for proto recv buff");
        return ESP_ERR_NO_MEM;
    }
    memset(g_proto.recv_buff.base, 0, CONFIG_NODE_PROTO_BUFF_SIZE);
    g_proto.send_buff.size = CONFIG_NODE_PROTO_BUFF_SIZE;
    g_proto.send_buff.base = malloc(CONFIG_NODE_PROTO_BUFF_SIZE);
    if (NULL == g_proto.send_buff.base) {
        ESP_LOGE(PROTO_TAG, "failedto malloc mem for proto send buff");
        return ESP_ERR_NO_MEM;
    }
    memset(g_proto.send_buff.base, 0, CONFIG_NODE_PROTO_BUFF_SIZE);

    // init entry list
    vListInitialise(&g_proto.entry_list);
    ESP_LOGI(PROTO_TAG, "coap proto init");
    return ESP_OK;
}

/* fini proto */
esp_err_t osh_node_proto_fini(void) {
    if (NULL != g_proto.recv_buff.base) {
        free(g_proto.recv_buff.base);
        g_proto.recv_buff.size = 0;
        g_proto.recv_buff.base = NULL;
    }
    if (NULL != g_proto.send_buff.base) {
        free(g_proto.send_buff.base);
        g_proto.send_buff.size = 0;
        g_proto.send_buff.base = NULL;
    }
    return ESP_OK;
}

/* start proto */
esp_err_t osh_node_proto_start(void *run_arg) {
    // prepare sockets
    if (-1 != g_proto.report_sock) {
        close(g_proto.report_sock);
        g_proto.report_sock = -1;
    }
    if (-1 != g_proto.mdm_sock) {
        close(g_proto.mdm_sock);
        g_proto.mdm_sock = -1;
    }
    if (-1 != g_proto.app_sock) {
        close(g_proto.app_sock);
        g_proto.app_sock = -1;
    }

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int err;

    // report - multicast client
    g_proto.report_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (0 > g_proto.report_sock) {
        ESP_LOGE(PROTO_TAG, "faield to create Report socket, errno:%d", errno);
        goto failed;
    }

    // mdm - multicast server
    g_proto.mdm_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (0 > g_proto.mdm_sock) {
        ESP_LOGE(PROTO_TAG, "faield to create MDM socket, errno:%d", errno);
        goto failed;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFIG_NODE_PROTO_MDM_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(g_proto.mdm_sock, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        ESP_LOGE(PROTO_TAG, "MDM socket unable to bind: errno %d", errno);
        goto failed;
    }
    mreq.imr_multiaddr.s_addr = inet_addr(CONFIG_NODE_PROTO_MDM_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    err = setsockopt(g_proto.mdm_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err < 0) {
        ESP_LOGE(PROTO_TAG, "MDM failed to add multicast membership: errno %d", errno);
        goto failed;
    }

    // app - unicast server
    g_proto.app_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (0 > g_proto.app_sock) {
        ESP_LOGE(PROTO_TAG, "faield to create APP socket, errno:%d", errno);
        goto failed;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFIG_NODE_PROTO_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(g_proto.app_sock, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        ESP_LOGE(PROTO_TAG, "APP socket unable to bind: errno %d", errno);
        goto failed;
    }

    // task
    if (NULL == g_proto.proto_task) {
        // create proto task if not existed
        xTaskCreate(proto_server, "proto", 8*1024, &g_proto, 5, &g_proto.proto_task);
    } else {
        // suspend proto task if existed
        vTaskResume(g_proto.proto_task);
    }

    if (NULL == g_proto.hb_timer) {
        // create timer if not existed
        g_proto.hb_timer = xTimerCreate("hb_timer",
                                    pdMS_TO_TICKS(CONFIG_NODE_PROTO_HB_PERIOD),
                                    pdTRUE, NULL,
                                    hb_timeout_cb);
        if (NULL == g_proto.hb_timer) {
            ESP_LOGE(PROTO_TAG, "Failed to create heartbeat timer");
            return OSH_ERR_PROTO_INNER;
        }
    } else {
        // start timer
        xTimerStart(g_proto.hb_timer, 0);
    }

    return ESP_OK;

failed:
    if (-1 != g_proto.report_sock) {
        close(g_proto.report_sock);
        g_proto.report_sock = -1;
    }
    if (-1 != g_proto.mdm_sock) {
        close(g_proto.mdm_sock);
        g_proto.mdm_sock = -1;
    }
    if (-1 != g_proto.app_sock) {
        close(g_proto.app_sock);
        g_proto.app_sock = -1;
    }

    return OSH_ERR_PROTO_INNER;
}

/* stop proto */
esp_err_t osh_node_proto_stop(void) {
    // clear up buffer
    memset(g_proto.recv_buff.base, 0, g_proto.recv_buff.size);
    g_proto.recv_buff.len = 0;
    memset(g_proto.send_buff.base, 0, g_proto.send_buff.size);
    g_proto.send_buff.len = 0;

    // suspend proto task
    vTaskSuspend(g_proto.proto_task);

    // stop timer
    xTimerStop(g_proto.hb_timer, 0);

    // close sockets
    if (-1 != g_proto.report_sock) {
        close(g_proto.report_sock);
        g_proto.report_sock = -1;
    }
    if (-1 != g_proto.mdm_sock) {
        close(g_proto.mdm_sock);
        g_proto.mdm_sock = -1;
    }
    if (-1 != g_proto.app_sock) {
        close(g_proto.app_sock);
        g_proto.app_sock = -1;
    }
    return ESP_OK;
}


/* register handler */
esp_err_t osh_node_route_register(uint32_t e,
                                  OSH_CODE_METHOD_ENUM method,
                                  osh_node_proto_handler_t handler) {
    esp_err_t res = ESP_FAIL;

    osh_node_proto_entry_t *entry = NULL;
    osh_node_proto_route_t *route = NULL;

    // find the entry
    if (!listLIST_IS_EMPTY(&g_proto.entry_list)) {
        osh_node_proto_entry_t *tmp_entry = NULL;
        listFOR_EACH_ENTRY(&g_proto.entry_list, osh_node_proto_entry_t, tmp_entry) {
            if (e == tmp_entry->entry) {
                // matched
                entry = tmp_entry;
                break;
            }
        }
    }

    if (NULL == entry) {
        // not matched, create a new entry
        entry = (osh_node_proto_entry_t *)malloc(sizeof(osh_node_proto_entry_t));
        if (NULL == entry) {
            ESP_LOGE(PROTO_TAG, "failed to malloc mem for entry 0x%lx", e);
            res = ESP_ERR_NO_MEM;
            goto clearup;
        }
        entry->entry = e;
        // init entry list
        vListInitialise(&entry->route_list);
        // init entry item
        vListInitialiseItem(&entry->entry_item);

        // add entry to list
        listSET_LIST_ITEM_OWNER(&entry->entry_item, entry);
        vListInsert(&g_proto.entry_list, &entry->entry_item);
    }

    // register route into entry
    route = malloc(sizeof(osh_node_proto_route_t));
    if (NULL == route) {
        ESP_LOGE(PROTO_TAG, "failed to malloc for method %d@0x%lx",
                (int)method, e);
        res = ESP_ERR_NO_MEM;
        goto clearup;
    }
    memset(route, 0, sizeof(osh_node_proto_route_t));

    route->method = method;
    vListInitialiseItem(&route->route_item);
    listSET_LIST_ITEM_OWNER(&route->route_item, route);

    // insert to route list
    route->route_item.xItemValue = (int)method;  // using bits as value
    vListInsert(&entry->route_list, &route->route_item);

    ESP_LOGI(PROTO_TAG, "create method method %d@0x%lx", (int)method, e);
    return ESP_OK;

clearup:
    if (NULL != entry)  free(entry);
    if (NULL != route) free(route);
    return res;
}
