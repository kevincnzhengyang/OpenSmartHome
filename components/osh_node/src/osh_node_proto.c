/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-08 19:20:33
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-09 23:11:46
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

#define INITIAL_DATA "Hello World!"

/* default request handler */
static void dft_request_handler(coap_resource_t *resource,
                                coap_session_t *session,
                                const coap_pdu_t *request,
                                const coap_string_t *query,
                                coap_pdu_t *response) {
    // TODO dispatch request to route callback
    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
    coap_add_data_large_response(resource, session, request, response,
                                 query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
                                 g_proto.buff_len,
                                 g_proto.buff,
                                 NULL, NULL);
}

// -------------------------------------
/*
 * The resource handler
 */
static void
hnd_espressif_get(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
    coap_add_data_large_response(resource, session, request, response,
                                 query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
                                 g_proto.buff_len,
                                 g_proto.buff,
                                 NULL, NULL);
}

static void
hnd_espressif_put(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    size_t size;
    size_t offset;
    size_t total;
    const unsigned char *data;

    coap_resource_notify_observers(resource, NULL);

    if (strcmp ((char *)g_proto.buff, INITIAL_DATA) == 0) {
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_CREATED);
    } else {
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
    }

    /* coap_get_data_large() sets size to 0 on error */
    (void)coap_get_data_large(request, &size, &data, &offset, &total);

    if (size == 0) {      /* re-init */
        snprintf((char *)g_proto.buff, g_proto.buff_size, INITIAL_DATA);
        g_proto.buff_len = strlen((char *)g_proto.buff);
    } else {
        g_proto.buff_len = size > g_proto.buff_size ? g_proto.buff_size : size;
        memcpy(g_proto.buff, data, g_proto.buff_len);
    }
}

static void
hnd_espressif_delete(coap_resource_t *resource,
                     coap_session_t *session,
                     const coap_pdu_t *request,
                     const coap_string_t *query,
                     coap_pdu_t *response)
{
    coap_resource_notify_observers(resource, NULL);
    snprintf((char *)g_proto.buff, g_proto.buff_size, INITIAL_DATA);
    g_proto.buff_len = strlen((char *)g_proto.buff);
    coap_pdu_set_code(response, COAP_RESPONSE_CODE_DELETED);
}
// ---------------------------------------


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

    snprintf((char *)g_proto.buff, g_proto.buff_size, INITIAL_DATA);
    g_proto.buff_len = strlen((char *)g_proto.buff);
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

        // TODO register all route
        coap_resource_t *resource = NULL;
        resource = coap_resource_init(coap_make_str_const("Espressif"), 0);
        if (!resource) {
            ESP_LOGE(PROTO_TAG, "coap_resource_init() failed");
            goto clean_up;
        }

        coap_register_handler(resource, COAP_REQUEST_GET, hnd_espressif_get);
        coap_register_handler(resource, COAP_REQUEST_PUT, hnd_espressif_put);
        coap_register_handler(resource, COAP_REQUEST_DELETE, hnd_espressif_delete);
        /* We possibly want to Observe the GETs */
        coap_resource_set_get_observable(resource, 1);
        coap_add_resource(ctx, resource);

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
    xTaskCreate(coap_server, "coap", 8*1024, &g_proto, 5, &g_proto.coap_task);
    return ESP_OK;
}

/* stop proto */
esp_err_t osh_node_proto_stop(void) {
    vTaskDelete(g_proto.coap_task);
    memset(g_proto.buff, 0, g_proto.buff_size);
    g_proto.buff_len = 0;
    return ESP_OK;
}


/* register handler */
esp_err_t osh_node_proto_register_request(coap_request_t method,
                                          const char* uri,
                                          coap_route_cb handler) {
    // register route into list
    return ESP_OK;
}
