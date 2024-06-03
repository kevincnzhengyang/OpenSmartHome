/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-06-02 20:04:36
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-06-03 22:57:32
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_proto.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include <string.h>
#include <sys/socket.h>

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
static uint16_t proto_address_get_port(const osh_node_proto_addr *addr) {
    if (NULL == addr) return 0;
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
    struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)addr;
    switch (addr->ss_family) {
    case AF_INET:
        return ntohs(addr_in->sin_port);
    case AF_INET6:
        return ntohs(addr_in6->sin6_port);
    default: /* undefined */
        return 0;
    }
    return 0;
}

static void proto_address_set_port(osh_node_proto_addr *addr, uint16_t port) {
    if (NULL == addr) return;
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
    struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)addr;
    switch (addr->ss_family) {
    case AF_INET:
        addr_in->sin_port = htons(port);
        break;
    case AF_INET6:
        addr_in6->sin6_port = htons(port);
        break;
    default: /* undefined */
        ;
  }
}

static esp_err_t proto_address_equals(const osh_node_proto_addr *a, const osh_node_proto_addr *b) {
    if (NULL == a || NULL == b) return ESP_FAIL;
    if (a->s2_len != b->s2_len || a->ss_family != b->ss_family) return ESP_FAIL;

    /* need to compare only relevant parts of sockaddr_in6 */
    switch (a->ss_family) {
    case AF_INET:
        return ((struct sockaddr_in *)a)->sin_port == ((struct sockaddr_in *)b)->sin_port &&
            0 == memcmp(&(((struct sockaddr_in *)a)->sin_addr),
                    &(((struct sockaddr_in *)b)->sin_addr),
                    sizeof(struct in_addr));
    case AF_INET6:
        return ((struct sockaddr_in6 *)a)->sin6_port == ((struct sockaddr_in6 *)b)->sin6_port &&
            0 == memcmp(&(((struct sockaddr_in6 *)a)->sin6_addr),
                    &(((struct sockaddr_in6 *)b)->sin6_addr),
                    sizeof(struct in6_addr));
    default: /* fall through and signal error */
        ;
    }
    return 0;
}

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
    pdu->code = buff[1];
    pdu->mid = (uint16_t)((buff[2] << 8) | buff[3]);
    pdu->con_type = (OSH_CONTENT_TYPE_ENUM)buff[4];
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

    if (offset > buff_len) {
        ESP_LOGE(PROTO_TAG, "decode overflow [%d]>[%d]", offset, buff_len);
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

    // init
    pdu->octets = buff;
    pdu->octets_size = buff_len;
    pdu->oct_rd = pdu->oct_wr = buff;
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
    buff[1] = pdu->code;
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
    memcpy(&buff[offset], pdu->content, pdu->con_len);
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

static void proto_server(void * arg) {
//     coap_context_t *ctx = NULL;
//     int have_ep = 0;
//     uint16_t u_s_port = atoi(CONFIG_NODE_COAP_PORT);
// #ifdef CONFIG_NODE_COAPS_PORT
//     uint16_t s_port = atoi(CONFIG_NODE_COAPS_PORT);
// #else /* ! CONFIG_NODE_COAPS_PORT */
//     uint16_t s_port = 0;
// #endif /* ! CONFIG_NODE_COAPS_PORT */

//     uint32_t scheme_hint_bits;

//     /* Initialize libcoap library */
//     coap_startup();

//     // snprintf((char *)g_proto.buff, g_proto.buff_size, INITIAL_DATA);
//     // g_proto.buff_len = strlen((char *)g_proto.buff);
//     coap_set_log_handler(coap_log_handler);
//     coap_set_log_level(CONFIG_COAP_LOG_DEFAULT_LEVEL);

//     while (1) {
//         unsigned wait_ms;
//         coap_addr_info_t *info = NULL;
//         coap_addr_info_t *info_list = NULL;

//         ctx = coap_new_context(NULL);
//         if (!ctx) {
//             ESP_LOGE(PROTO_TAG, "coap_new_context() failed");
//             goto clean_up;
//         }
//         coap_context_set_block_mode(ctx,
//                                     COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
//         coap_context_set_max_idle_sessions(ctx, 20);

// #ifdef CONFIG_NODE_COAP_MBEDTLS_PSK
//         /* Need PSK setup before we set up endpoints */
//         coap_context_set_psk(ctx, "CoAP",
//                              (const uint8_t *)CONFIG_NODE_COAP_PSK_KEY,
//                              sizeof(CONFIG_NODE_COAP_PSK_KEY) - 1);
// #endif /* CONFIG_NODE_COAP_MBEDTLS_PSK */

// #ifdef CONFIG_NODE_COAP_MBEDTLS_PKI
//         /* Need PKI setup before we set up endpoints */
//         unsigned int ca_pem_bytes = ca_pem_end - ca_pem_start;
//         unsigned int server_crt_bytes = server_crt_end - server_crt_start;
//         unsigned int server_key_bytes = server_key_end - server_key_start;
//         coap_dtls_pki_t dtls_pki;

//         memset (&dtls_pki, 0, sizeof(dtls_pki));
//         dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
//         if (ca_pem_bytes) {
//             /*
//              * Add in additional certificate checking.
//              * This list of enabled can be tuned for the specific
//              * requirements - see 'man coap_encryption'.
//              *
//              * Note: A list of root ca file can be setup separately using
//              * coap_context_set_pki_root_cas(), but the below is used to
//              * define what checking actually takes place.
//              */
//             dtls_pki.verify_peer_cert        = 1;
//             dtls_pki.check_common_ca         = 1;
//             dtls_pki.allow_self_signed       = 1;
//             dtls_pki.allow_expired_certs     = 1;
//             dtls_pki.cert_chain_validation   = 1;
//             dtls_pki.cert_chain_verify_depth = 2;
//             dtls_pki.check_cert_revocation   = 1;
//             dtls_pki.allow_no_crl            = 1;
//             dtls_pki.allow_expired_crl       = 1;
//             dtls_pki.allow_bad_md_hash       = 1;
//             dtls_pki.allow_short_rsa_length  = 1;
//             dtls_pki.validate_cn_call_back   = verify_cn_callback;
//             dtls_pki.cn_call_back_arg        = NULL;
//             dtls_pki.validate_sni_call_back  = NULL;
//             dtls_pki.sni_call_back_arg       = NULL;
//         }
//         dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM_BUF;
//         dtls_pki.pki_key.key.pem_buf.public_cert = server_crt_start;
//         dtls_pki.pki_key.key.pem_buf.public_cert_len = server_crt_bytes;
//         dtls_pki.pki_key.key.pem_buf.private_key = server_key_start;
//         dtls_pki.pki_key.key.pem_buf.private_key_len = server_key_bytes;
//         dtls_pki.pki_key.key.pem_buf.ca_cert = ca_pem_start;
//         dtls_pki.pki_key.key.pem_buf.ca_cert_len = ca_pem_bytes;

//         coap_context_set_pki(ctx, &dtls_pki);
// #endif /* CONFIG_NODE_COAP_MBEDTLS_PKI */

//         /* set up the CoAP server socket(s) */
//         scheme_hint_bits =
//             coap_get_available_scheme_hint_bits(
// #if defined(CONFIG_NODE_COAP_MBEDTLS_PSK) || defined(CONFIG_NODE_COAP_MBEDTLS_PKI)
//                 1,
// #else /* ! CONFIG_NODE_COAP_MBEDTLS_PSK) && ! CONFIG_NODE_COAP_MBEDTLS_PKI */
//                 0,
// #endif /* ! CONFIG_NODE_COAP_MBEDTLS_PSK) && ! CONFIG_NODE_COAP_MBEDTLS_PKI */
//                 0,
//                 0);

//         info_list = coap_resolve_address_info(coap_make_str_const("0.0.0.0"), u_s_port, s_port,
//                                               0, 0, 0,
//                                               scheme_hint_bits,
//                                               COAP_RESOLVE_TYPE_LOCAL);
//         if (info_list == NULL) {
//             ESP_LOGE(PROTO_TAG, "coap_resolve_address_info() failed");
//             goto clean_up;
//         }

//         for (info = info_list; info != NULL; info = info->next) {
//             coap_endpoint_t *ep;

//             ep = coap_new_endpoint(ctx, &info->addr, info->proto);
//             if (!ep) {
//                 ESP_LOGW(PROTO_TAG, "cannot create endpoint for proto %u", info->proto);
//             } else {
//                 have_ep = 1;
//             }
//         }
//         coap_free_address_info(info_list);
//         if (!have_ep) {
//             ESP_LOGE(PROTO_TAG, "No endpoints available");
//             goto clean_up;
//         }

//         // register all routes
//         if (!listLIST_IS_EMPTY(&g_proto.entry_list)) {
//             // have entry
//             osh_node_proto_entry_t *entry = NULL;
//             listFOR_EACH_ENTRY(&g_proto.entry_list, osh_node_proto_entry_t, entry) {
//                 if (!listLIST_IS_EMPTY(&entry->route_list)) {
//                     // have routes, create resource and register coap handlers
//                             coap_resource_t *resource = coap_resource_init(coap_make_str_const(entry->uri), 0);
//                             if (!resource) {
//                                 ESP_LOGE(PROTO_TAG, "coap_resource_init() failed");
//                                 goto clean_up;
//                             }
//                             osh_node_proto_route_t *route = NULL;
//                             listFOR_EACH_ENTRY(&entry->route_list, osh_node_proto_route_t, route) {
//                                 coap_register_handler(resource, route->method, route->route_cb);
//                                 ESP_LOGI(PROTO_TAG, "register coap %s: %d", entry->uri, (int)route->method);
//                             }

//                             /* We possibly want to Observe the GETs */
//                             coap_resource_set_get_observable(resource, 1);

//                             coap_add_resource(ctx, resource);
//                 }
//             }
//         }

//         // multicast
//         esp_netif_t *netif = NULL;
//         for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
//             char buf[8];
//             netif = esp_netif_next_unsafe(netif);
//             esp_netif_get_netif_impl_name(netif, buf);
//             coap_join_mcast_group_intf(ctx, CONFIG_NODE_COAP_MULTICAST_ADDR, buf);
//         }


//         wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

//         while (1) {
//             int result = coap_io_process(ctx, wait_ms);
//             if (result < 0) {
//                 break;
//             } else if (result && (unsigned)result < wait_ms) {
//                 /* decrement if there is a result wait time returned */
//                 wait_ms -= result;
//             }
//             if (result) {
//                 /* result must have been >= wait_ms, so reset wait_ms */
//                 wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
//             }
//         }
//     }
// clean_up:
//     coap_free_context(ctx);
//     coap_cleanup();

    vTaskDelete(NULL);
}

/** -------------------------------
 *            functions
 *  -------------------------------
*/

/* init proto */
esp_err_t osh_node_proto_init(osh_node_bb_t *node_bb, void * conf_arg) {
    memset(&g_proto, 0, sizeof(osh_node_proto_t));
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
    return ESP_OK;
}


/* register handler */
esp_err_t osh_node_route_register(uint32_t entry,
                                  OSH_CODE_METHOD_ENUM method,
                                  osh_node_proto_handler_t handler) {
    // esp_err_t res = ESP_FAIL;

    // osh_node_proto_entry_t *entry = NULL;
    // osh_node_proto_route_t *route = NULL;

    // // find the uri
    // if (!listLIST_IS_EMPTY(&g_proto.entry_list)) {
    //     osh_node_proto_entry_t *tmp_entry = NULL;
    //     listFOR_EACH_ENTRY(&g_proto.entry_list, osh_node_proto_entry_t, tmp_entry) {
    //         if (0 == strncmp(uri, tmp_entry->uri, strlen(tmp_entry->uri))) {
    //             // matched
    //             entry = tmp_entry;
    //             break;
    //         }
    //     }
    // }

    // if (NULL == entry) {
    //     // not matched, create a new entry
    //     entry = (osh_node_proto_entry_t *)malloc(sizeof(osh_node_proto_entry_t));
    //     if (NULL == entry) {
    //         ESP_LOGE(PROTO_TAG, "failed to malloc mem for entry %s", uri);
    //         res = ESP_ERR_NO_MEM;
    //         goto clearup;
    //     }
    //     entry->uri = strdup(uri);
    //     if (NULL == entry->uri) {
    //         ESP_LOGE(PROTO_TAG, "failed to malloc mem for uri %s", uri);
    //         res = ESP_ERR_NO_MEM;
    //         goto clearup;
    //     }
    //     // init entry list
    //     vListInitialise(&entry->route_list);
    //     // init entry item
    //     vListInitialiseItem(&entry->entry_item);

    //     // add entry to list
    //     listSET_LIST_ITEM_OWNER(&entry->entry_item, entry);
    //     vListInsert(&g_proto.entry_list, &entry->entry_item);
    // }

    // // register route into entry
    // route = malloc(sizeof(osh_node_proto_route_t));
    // if (NULL == route) {
    //     ESP_LOGE(PROTO_TAG, "failed to malloc for route %s, method %d", uri, (int)method);
    //     res = ESP_ERR_NO_MEM;
    //     goto clearup;
    // }
    // memset(route, 0, sizeof(osh_node_proto_route_t));

    // route->method = method;
    // vListInitialiseItem(&route->route_item);
    // listSET_LIST_ITEM_OWNER(&route->route_item, route);

    // // insert to route list
    // route->route_item.xItemValue = (int)method;  // using bits as value
    // vListInsert(&entry->route_list, &route->route_item);

    // ESP_LOGI(PROTO_TAG, "create route %s, method %d", uri, (int)method);
    return ESP_OK;

// clearup:
//     if (NULL != entry) {
//         if (NULL != entry->uri) free(entry->uri);
//         free(entry);
//     }
//     if (NULL != route) free(route);
//     return res;
}
