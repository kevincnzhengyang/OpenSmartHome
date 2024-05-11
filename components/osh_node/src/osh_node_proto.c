/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-08 19:20:33
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-11 22:28:36
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_proto.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include <string.h>
#include <sys/socket.h>

#include "esp_wifi.h"

#include "osh_node_proto.h"
#include "osh_node_proto.inc"

static const char *PROTO_TAG = "COAP";

static osh_node_proto_t g_proto;

#ifdef CONFIG_NODE_COAP_MBEDTLS_PKI
/* CA cert, taken from coap_ca.pem
   Server cert, taken from coap_server.crt
   Server key, taken from coap_server.key

   The PEM, CRT and KEY file are examples taken from
   https://github.com/eclipse/californium/tree/master/demo-certs/src/main/resources
   as the Certificate test (by default) for the coap_client is against the
   californium server.

   To embed it in the app binary, the PEM, CRT and KEY file is named
   in the CMakeLists.txt EMBED_TXTFILES definition.
 */
extern uint8_t ca_pem_start[] asm("_binary_coap_ca_pem_start");
extern uint8_t ca_pem_end[]   asm("_binary_coap_ca_pem_end");
extern uint8_t server_crt_start[] asm("_binary_coap_server_crt_start");
extern uint8_t server_crt_end[]   asm("_binary_coap_server_crt_end");
extern uint8_t server_key_start[] asm("_binary_coap_server_key_start");
extern uint8_t server_key_end[]   asm("_binary_coap_server_key_end");
#endif /* CONFIG_NODE_COAP_MBEDTLS_PKI */

// #define INITIAL_DATA "Hello World!"

#ifdef CONFIG_NODE_COAP_MBEDTLS_PKI
static int verify_cn_callback(const char *cn,
                   const uint8_t *asn1_public_cert,
                   size_t asn1_length,
                   coap_session_t *session,
                   unsigned depth,
                   int validated,
                   void *arg
                  ) {
    coap_log_info("CN '%s' presented by server (%s)\n",
                  cn, depth ? "CA" : "Certificate");
    return 1;
}
#endif /* CONFIG_NODE_COAP_MBEDTLS_PKI */

static void coap_log_handler (coap_log_t level, const char *message) {
    uint32_t esp_level = ESP_LOG_INFO;
    const char *cp = strchr(message, '\n');

    while (cp) {
        ESP_LOG_LEVEL(esp_level, PROTO_TAG, "%.*s", (int)(cp - message), message);
        message = cp + 1;
        cp = strchr(message, '\n');
    }
    if (message[0] != '\000') {
        ESP_LOG_LEVEL(esp_level, PROTO_TAG, "%s", message);
    }
}

static void coap_server(void * arg) {
    coap_context_t *ctx = NULL;
    int have_ep = 0;
    uint16_t u_s_port = atoi(CONFIG_NODE_COAP_PORT);
#ifdef CONFIG_NODE_COAPS_PORT
    uint16_t s_port = atoi(CONFIG_NODE_COAPS_PORT);
#else /* ! CONFIG_NODE_COAPS_PORT */
    uint16_t s_port = 0;
#endif /* ! CONFIG_NODE_COAPS_PORT */

    uint32_t scheme_hint_bits;

    /* Initialize libcoap library */
    coap_startup();

    // snprintf((char *)g_proto.buff, g_proto.buff_size, INITIAL_DATA);
    // g_proto.buff_len = strlen((char *)g_proto.buff);
    coap_set_log_handler(coap_log_handler);
    coap_set_log_level(CONFIG_COAP_LOG_DEFAULT_LEVEL);

    while (1) {
        unsigned wait_ms;
        coap_addr_info_t *info = NULL;
        coap_addr_info_t *info_list = NULL;

        ctx = coap_new_context(NULL);
        if (!ctx) {
            ESP_LOGE(PROTO_TAG, "coap_new_context() failed");
            goto clean_up;
        }
        coap_context_set_block_mode(ctx,
                                    COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
        coap_context_set_max_idle_sessions(ctx, 20);

#ifdef CONFIG_NODE_COAP_MBEDTLS_PSK
        /* Need PSK setup before we set up endpoints */
        coap_context_set_psk(ctx, "CoAP",
                             (const uint8_t *)CONFIG_NODE_COAP_PSK_KEY,
                             sizeof(CONFIG_NODE_COAP_PSK_KEY) - 1);
#endif /* CONFIG_NODE_COAP_MBEDTLS_PSK */

#ifdef CONFIG_NODE_COAP_MBEDTLS_PKI
        /* Need PKI setup before we set up endpoints */
        unsigned int ca_pem_bytes = ca_pem_end - ca_pem_start;
        unsigned int server_crt_bytes = server_crt_end - server_crt_start;
        unsigned int server_key_bytes = server_key_end - server_key_start;
        coap_dtls_pki_t dtls_pki;

        memset (&dtls_pki, 0, sizeof(dtls_pki));
        dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
        if (ca_pem_bytes) {
            /*
             * Add in additional certificate checking.
             * This list of enabled can be tuned for the specific
             * requirements - see 'man coap_encryption'.
             *
             * Note: A list of root ca file can be setup separately using
             * coap_context_set_pki_root_cas(), but the below is used to
             * define what checking actually takes place.
             */
            dtls_pki.verify_peer_cert        = 1;
            dtls_pki.check_common_ca         = 1;
            dtls_pki.allow_self_signed       = 1;
            dtls_pki.allow_expired_certs     = 1;
            dtls_pki.cert_chain_validation   = 1;
            dtls_pki.cert_chain_verify_depth = 2;
            dtls_pki.check_cert_revocation   = 1;
            dtls_pki.allow_no_crl            = 1;
            dtls_pki.allow_expired_crl       = 1;
            dtls_pki.allow_bad_md_hash       = 1;
            dtls_pki.allow_short_rsa_length  = 1;
            dtls_pki.validate_cn_call_back   = verify_cn_callback;
            dtls_pki.cn_call_back_arg        = NULL;
            dtls_pki.validate_sni_call_back  = NULL;
            dtls_pki.sni_call_back_arg       = NULL;
        }
        dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM_BUF;
        dtls_pki.pki_key.key.pem_buf.public_cert = server_crt_start;
        dtls_pki.pki_key.key.pem_buf.public_cert_len = server_crt_bytes;
        dtls_pki.pki_key.key.pem_buf.private_key = server_key_start;
        dtls_pki.pki_key.key.pem_buf.private_key_len = server_key_bytes;
        dtls_pki.pki_key.key.pem_buf.ca_cert = ca_pem_start;
        dtls_pki.pki_key.key.pem_buf.ca_cert_len = ca_pem_bytes;

        coap_context_set_pki(ctx, &dtls_pki);
#endif /* CONFIG_NODE_COAP_MBEDTLS_PKI */

        /* set up the CoAP server socket(s) */
        scheme_hint_bits =
            coap_get_available_scheme_hint_bits(
#if defined(CONFIG_NODE_COAP_MBEDTLS_PSK) || defined(CONFIG_NODE_COAP_MBEDTLS_PKI)
                1,
#else /* ! CONFIG_NODE_COAP_MBEDTLS_PSK) && ! CONFIG_NODE_COAP_MBEDTLS_PKI */
                0,
#endif /* ! CONFIG_NODE_COAP_MBEDTLS_PSK) && ! CONFIG_NODE_COAP_MBEDTLS_PKI */
                0,
                0);

        info_list = coap_resolve_address_info(coap_make_str_const("0.0.0.0"), u_s_port, s_port,
                                              0, 0, 0,
                                              scheme_hint_bits,
                                              COAP_RESOLVE_TYPE_LOCAL);
        if (info_list == NULL) {
            ESP_LOGE(PROTO_TAG, "coap_resolve_address_info() failed");
            goto clean_up;
        }

        for (info = info_list; info != NULL; info = info->next) {
            coap_endpoint_t *ep;

            ep = coap_new_endpoint(ctx, &info->addr, info->proto);
            if (!ep) {
                ESP_LOGW(PROTO_TAG, "cannot create endpoint for proto %u", info->proto);
            } else {
                have_ep = 1;
            }
        }
        coap_free_address_info(info_list);
        if (!have_ep) {
            ESP_LOGE(PROTO_TAG, "No endpoints available");
            goto clean_up;
        }

        // register all routes
        if (!listLIST_IS_EMPTY(&g_proto.entry_list)) {
            // have entry
            osh_node_proto_entry_t *entry = NULL;
            listFOR_EACH_ENTRY(&g_proto.entry_list, osh_node_proto_entry_t, entry) {
                if (!listLIST_IS_EMPTY(&entry->route_list)) {
                    // have routes, create resource and register coap handlers
                            coap_resource_t *resource = coap_resource_init(coap_make_str_const(entry->uri), 0);
                            if (!resource) {
                                ESP_LOGE(PROTO_TAG, "coap_resource_init() failed");
                                goto clean_up;
                            }
                            osh_node_proto_route_t *route = NULL;
                            listFOR_EACH_ENTRY(&entry->route_list, osh_node_proto_route_t, route) {
                                coap_register_handler(resource, route->method, route->route_cb);
                                ESP_LOGI(PROTO_TAG, "register coap %s: %d", entry->uri, (int)route->method);
                            }

                            /* We possibly want to Observe the GETs */
                            coap_resource_set_get_observable(resource, 1);

                            coap_add_resource(ctx, resource);
                }
            }
        }

        // multicast
        esp_netif_t *netif = NULL;
        for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
            char buf[8];
            netif = esp_netif_next_unsafe(netif);
            esp_netif_get_netif_impl_name(netif, buf);
            coap_join_mcast_group_intf(ctx, CONFIG_NODE_COAP_MULTICAST_ADDR, buf);
        }


        wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

        while (1) {
            int result = coap_io_process(ctx, wait_ms);
            if (result < 0) {
                break;
            } else if (result && (unsigned)result < wait_ms) {
                /* decrement if there is a result wait time returned */
                wait_ms -= result;
            }
            if (result) {
                /* result must have been >= wait_ms, so reset wait_ms */
                wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
            }
        }
    }
clean_up:
    coap_free_context(ctx);
    coap_cleanup();

    vTaskDelete(NULL);
}

/** -------------------------------
 *            functions
 *  -------------------------------
*/

/* init proto */
esp_err_t osh_node_proto_init(size_t buff_size, void * conf_arg) {
    memset(&g_proto, 0, sizeof(osh_node_proto_t));
    g_proto.buff = malloc(buff_size);
    if (NULL == g_proto.buff) {
        ESP_LOGE(PROTO_TAG, "failedto malloc mem for proto");
        return ESP_ERR_NO_MEM;
    }
    memset(g_proto.buff, 0, buff_size);

    // init entry list
    vListInitialise(&g_proto.entry_list);
    g_proto.buff_size = buff_size;

    return ESP_OK;
}

/* fini proto */
esp_err_t osh_node_proto_fini(void) {
    if (NULL != g_proto.buff) {
        free(g_proto.buff);
        g_proto.buff_size = 0;
        g_proto.buff = NULL;
    }
    return ESP_OK;
}

/* start proto */
esp_err_t osh_node_proto_start(void) {
    if (NULL == g_proto.coap_task) {
        // create coap task if not existed
        xTaskCreate(coap_server, "coap", 8*1024, &g_proto, 5, &g_proto.coap_task);
    } else {
        // suspend coap task if existed
        vTaskResume(g_proto.coap_task);
    }

    return ESP_OK;
}

/* stop proto */
esp_err_t osh_node_proto_stop(void) {
    // clear up buffer
    memset(g_proto.buff, 0, g_proto.buff_size);
    g_proto.buff_len = 0;

    // resume coap task
    vTaskSuspend(g_proto.coap_task);
    return ESP_OK;
}


/* register handler */
esp_err_t osh_node_proto_register_request(const char* uri,
                                        coap_request_t method,
                                          coap_route_cb handler) {
    esp_err_t res = ESP_FAIL;

    osh_node_proto_entry_t *entry = NULL;
    osh_node_proto_route_t *route = NULL;

    // find the uri
    if (!listLIST_IS_EMPTY(&g_proto.entry_list)) {
        osh_node_proto_entry_t *tmp_entry = NULL;
        listFOR_EACH_ENTRY(&g_proto.entry_list, osh_node_proto_entry_t, tmp_entry) {
            if (0 == strncmp(uri, tmp_entry->uri, strlen(tmp_entry->uri))) {
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
            ESP_LOGE(PROTO_TAG, "failed to malloc mem for entry %s", uri);
            res = ESP_ERR_NO_MEM;
            goto clearup;
        }
        entry->uri = strdup(uri);
        if (NULL == entry->uri) {
            ESP_LOGE(PROTO_TAG, "failed to malloc mem for uri %s", uri);
            res = ESP_ERR_NO_MEM;
            goto clearup;
        }
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
        ESP_LOGE(PROTO_TAG, "failed to malloc for route %s, method %d", uri, (int)method);
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

    ESP_LOGI(PROTO_TAG, "create route %s, method %d", uri, (int)method);
    return ESP_OK;

clearup:
    if (NULL != entry) {
        if (NULL != entry->uri) free(entry->uri);
        free(entry);
    }
    if (NULL != route) free(route);
    return res;
}
