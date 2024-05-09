/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 22:36:41
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-08 22:11:40
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_wifi.c
 * @Description : WiFi network
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "wifi_provisioning/manager.h"
#include <wifi_provisioning/scheme_softap.h>
#include "esp_netif.h"
#include "qrcode.h"
#include "mdns.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

#include "mbedtls/pk.h"
#include "mbedtls/md.h"
#include "mbedtls/error.h"

#include "osh_node_wifi.h"
#include "osh_node_events.h"
#include "osh_node_fsm.h"

const char *WIFI_TAG = "WiFi";

typedef struct {
    struct sockaddr_storage source_addr;
    socklen_t                   socklen;
    int                            sock;
} wifi_conn_stru;

static osh_node_network g_node_wifi;
static wifi_conn_stru  g_wifi_conn;
static osh_node_network g_network;

/** --------------------------
 *        WiFi Provision
 *  --------------------------
*/
#define EXAMPLE_PROV_SEC2_USERNAME          "wifiprov"
#define EXAMPLE_PROV_SEC2_PWD               "abcd1234"

/* This salt,verifier has been generated for username = "wifiprov" and password = "abcd1234"
 * IMPORTANT NOTE: For production cases, this must be unique to every device
 * and should come from device manufacturing partition.*/
static const char sec2_salt[] = {
    0x03, 0x6e, 0xe0, 0xc7, 0xbc, 0xb9, 0xed, 0xa8, 0x4c, 0x9e, 0xac, 0x97, 0xd9, 0x3d, 0xec, 0xf4
};

static const char sec2_verifier[] = {
    0x7c, 0x7c, 0x85, 0x47, 0x65, 0x08, 0x94, 0x6d, 0xd6, 0x36, 0xaf, 0x37, 0xd7, 0xe8, 0x91, 0x43,
    0x78, 0xcf, 0xfd, 0x61, 0x6c, 0x59, 0xd2, 0xf8, 0x39, 0x08, 0x12, 0x72, 0x38, 0xde, 0x9e, 0x24,
    0xa4, 0x70, 0x26, 0x1c, 0xdf, 0xa9, 0x03, 0xc2, 0xb2, 0x70, 0xe7, 0xb1, 0x32, 0x24, 0xda, 0x11,
    0x1d, 0x97, 0x18, 0xdc, 0x60, 0x72, 0x08, 0xcc, 0x9a, 0xc9, 0x0c, 0x48, 0x27, 0xe2, 0xae, 0x89,
    0xaa, 0x16, 0x25, 0xb8, 0x04, 0xd2, 0x1a, 0x9b, 0x3a, 0x8f, 0x37, 0xf6, 0xe4, 0x3a, 0x71, 0x2e,
    0xe1, 0x27, 0x86, 0x6e, 0xad, 0xce, 0x28, 0xff, 0x54, 0x46, 0x60, 0x1f, 0xb9, 0x96, 0x87, 0xdc,
    0x57, 0x40, 0xa7, 0xd4, 0x6c, 0xc9, 0x77, 0x54, 0xdc, 0x16, 0x82, 0xf0, 0xed, 0x35, 0x6a, 0xc4,
    0x70, 0xad, 0x3d, 0x90, 0xb5, 0x81, 0x94, 0x70, 0xd7, 0xbc, 0x65, 0xb2, 0xd5, 0x18, 0xe0, 0x2e,
    0xc3, 0xa5, 0xf9, 0x68, 0xdd, 0x64, 0x7b, 0xb8, 0xb7, 0x3c, 0x9c, 0xfc, 0x00, 0xd8, 0x71, 0x7e,
    0xb7, 0x9a, 0x7c, 0xb1, 0xb7, 0xc2, 0xc3, 0x18, 0x34, 0x29, 0x32, 0x43, 0x3e, 0x00, 0x99, 0xe9,
    0x82, 0x94, 0xe3, 0xd8, 0x2a, 0xb0, 0x96, 0x29, 0xb7, 0xdf, 0x0e, 0x5f, 0x08, 0x33, 0x40, 0x76,
    0x52, 0x91, 0x32, 0x00, 0x9f, 0x97, 0x2c, 0x89, 0x6c, 0x39, 0x1e, 0xc8, 0x28, 0x05, 0x44, 0x17,
    0x3f, 0x68, 0x02, 0x8a, 0x9f, 0x44, 0x61, 0xd1, 0xf5, 0xa1, 0x7e, 0x5a, 0x70, 0xd2, 0xc7, 0x23,
    0x81, 0xcb, 0x38, 0x68, 0xe4, 0x2c, 0x20, 0xbc, 0x40, 0x57, 0x76, 0x17, 0xbd, 0x08, 0xb8, 0x96,
    0xbc, 0x26, 0xeb, 0x32, 0x46, 0x69, 0x35, 0x05, 0x8c, 0x15, 0x70, 0xd9, 0x1b, 0xe9, 0xbe, 0xcc,
    0xa9, 0x38, 0xa6, 0x67, 0xf0, 0xad, 0x50, 0x13, 0x19, 0x72, 0x64, 0xbf, 0x52, 0xc2, 0x34, 0xe2,
    0x1b, 0x11, 0x79, 0x74, 0x72, 0xbd, 0x34, 0x5b, 0xb1, 0xe2, 0xfd, 0x66, 0x73, 0xfe, 0x71, 0x64,
    0x74, 0xd0, 0x4e, 0xbc, 0x51, 0x24, 0x19, 0x40, 0x87, 0x0e, 0x92, 0x40, 0xe6, 0x21, 0xe7, 0x2d,
    0x4e, 0x37, 0x76, 0x2f, 0x2e, 0xe2, 0x68, 0xc7, 0x89, 0xe8, 0x32, 0x13, 0x42, 0x06, 0x84, 0x84,
    0x53, 0x4a, 0xb3, 0x0c, 0x1b, 0x4c, 0x8d, 0x1c, 0x51, 0x97, 0x19, 0xab, 0xae, 0x77, 0xff, 0xdb,
    0xec, 0xf0, 0x10, 0x95, 0x34, 0x33, 0x6b, 0xcb, 0x3e, 0x84, 0x0f, 0xb9, 0xd8, 0x5f, 0xb8, 0xa0,
    0xb8, 0x55, 0x53, 0x3e, 0x70, 0xf7, 0x18, 0xf5, 0xce, 0x7b, 0x4e, 0xbf, 0x27, 0xce, 0xce, 0xa8,
    0xb3, 0xbe, 0x40, 0xc5, 0xc5, 0x32, 0x29, 0x3e, 0x71, 0x64, 0x9e, 0xde, 0x8c, 0xf6, 0x75, 0xa1,
    0xe6, 0xf6, 0x53, 0xc8, 0x31, 0xa8, 0x78, 0xde, 0x50, 0x40, 0xf7, 0x62, 0xde, 0x36, 0xb2, 0xba
};

static esp_err_t example_get_sec2_salt(const char **salt, uint16_t *salt_len) {
    ESP_LOGI(WIFI_TAG, "Development mode: using hard coded salt");
    *salt = sec2_salt;
    *salt_len = sizeof(sec2_salt);
    return ESP_OK;
}

static esp_err_t example_get_sec2_verifier(const char **verifier, uint16_t *verifier_len) {
    ESP_LOGI(WIFI_TAG, "Development mode: using hard coded verifier");
    *verifier = sec2_verifier;
    *verifier_len = sizeof(sec2_verifier);
    return ESP_OK;
}

#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_SOFTAP   "softap"
#define QRCODE_BASE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"


/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    static int retries;

    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(WIFI_TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(WIFI_TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(WIFI_TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");

                retries++;
                if (retries >= CONFIG_NODE_WIFI_PROV_RETRIES) {
                    ESP_LOGI(WIFI_TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
                    wifi_prov_mgr_reset_sm_state_on_failure();
                    retries = 0;
                }
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(WIFI_TAG, "Provisioning successful");
                retries = 0;
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(WIFI_TAG, "Disconnected. Connecting to the AP again...");
                osh_invoke_event(OSH_NODE_EVENT_DISCONNECT);
                esp_wifi_connect();
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(WIFI_TAG, "SoftAP transport: Connected!");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(WIFI_TAG, "SoftAP transport: Disconnected!");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        // change state and invoke OSH_NODE_EVENT_CONNECT
        osh_invoke_event(OSH_NODE_EVENT_CONNECT);

    } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id) {
            case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
                ESP_LOGI(WIFI_TAG, "Secured session established!");
                break;
            case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
                ESP_LOGE(WIFI_TAG, "Received invalid security parameters for establishing secure session!");
                break;
            case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
                ESP_LOGE(WIFI_TAG, "Received incorrect username and/or PoP for establishing secure session!");
                break;
            default:
                break;
        }
    }
}

static void wifi_init_sta(void) {
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_prov_print_qr(const char *name, const char *username,
                               const char *pop, const char *transport) {
    if (!name || !transport) {
        ESP_LOGW(WIFI_TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
    if (pop) {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"username\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, username, pop, transport);
    } else {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, transport);
    }
    ESP_LOGI(WIFI_TAG, "Scan this QR code from the provisioning application for Provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);
    ESP_LOGI(WIFI_TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}

/** --------------------------
 *          mDNS
 *  --------------------------
*/
/* start mDNS */
static esp_err_t start_mdns(void) {
    // init mDNS
    esp_err_t err = mdns_init();
    if (err) {
        return err;
    }

    // set hostname
    mdns_hostname_set(g_node_wifi.node_name);
    // set default instance
    mdns_instance_name_set("OpenSmartHome");
    // add service
    mdns_service_add(NULL, "_osh_node", "_udp",
            CONFIG_NODE_UDP_PORT, NULL, 0);

    return ESP_OK;
}

/* stop mDNS */
static esp_err_t stop_mdns(void) {
    mdns_free();
    return ESP_OK;
}

/** --------------------------
 *           UDP Server
 *  --------------------------
*/
static void udp_server_svc(void *arg)
{
    struct sockaddr_in6 dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(CONFIG_NODE_UDP_PORT);

    if (NULL != g_node_wifi.proto && NULL != g_node_wifi.proto->init) {
        // init proto
        if (ESP_OK != g_node_wifi.proto->init(arg)) {
            ESP_LOGE(WIFI_TAG, "UDP Server failed to init proto");
            vTaskDelete(NULL);
            return;
        }
    }

    while (1) {
        // reset proto when server reset
        if (NULL != g_node_wifi.proto && NULL != g_node_wifi.proto->reset) g_node_wifi.proto->reset();
        int sock = -1;
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

        if (0 > sock) {
            ESP_LOGE(WIFI_TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        ESP_LOGI(WIFI_TAG, "Socket created");
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);


        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(WIFI_TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(WIFI_TAG, "Socket bound, port %d", CONFIG_NODE_UDP_PORT);



        while (1) {
            ESP_LOGI(WIFI_TAG, "Waiting for data");
            int len = recvfrom(sock, g_node_wifi.rx_buff, g_node_wifi.rx_size, 0,
                                    (struct sockaddr *)&g_wifi_conn.source_addr,
                                    &g_wifi_conn.socklen);

            if (len < 0) {
                if (EAGAIN == errno) {
                    // TODO count and power save
                    // count
                    ESP_LOGD(WIFI_TAG, "socket timeout");
                    continue;
                } else {
                    ESP_LOGE(WIFI_TAG, "recvfrom failed: errno %d", errno);
                    break;
                }
            } else {
                g_wifi_conn.sock = sock;    // save sock for response
                g_node_wifi.rx_len = len;
                ESP_LOGI(WIFI_TAG, "Received: len %d from " IPSTR, len,
                            IP2STR((esp_ip4_addr_t *)g_wifi_conn.source_addr.s2_data1));

                if (NULL != g_node_wifi.proto && NULL != g_node_wifi.proto->decode) {
                    if (ESP_OK != g_node_wifi.proto->decode(
                                    g_node_wifi.rx_buff,
                                    g_node_wifi.rx_len,
                                    g_node_wifi.arg)) {
                        ESP_LOGE(WIFI_TAG, "failed to process, reset UDP server");
                        break;
                    }
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(WIFI_TAG, "Shutting down socket and restarting...");
            g_wifi_conn.sock = -1;
            g_node_wifi.rx_len = 0;
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

/* start UDP Server */
static esp_err_t start_udp_server(void *arg) {
    xTaskCreate(udp_server_svc, "udp_server",
        CONFIG_NODE_UDP_STACK_SIZE, arg, 5, &g_node_wifi.server_task);
    return ESP_OK;
}

/* stop UDP Server */
static esp_err_t stop_udp_server(void *arg) {
    // fini proto
    if (NULL != g_node_wifi.proto && NULL != g_node_wifi.proto->fini) g_node_wifi.proto->fini();

    vTaskDelete(g_node_wifi.server_task);
    return ESP_OK;
}

/* send msg */
static esp_err_t send_udp_response(void *arg) {
    if (NULL == g_node_wifi.proto || NULL == g_node_wifi.proto->encode) return ESP_OK;

    // encode resposne
    esp_err_t ret = g_node_wifi.proto->encode(g_node_wifi.proto->msg,
                            g_node_wifi.tx_buff, g_node_wifi.tx_size,
                            &g_node_wifi.tx_len);
    if (ESP_OK != ret) return ret;

    // udp send to client
    if (-1 == g_wifi_conn.sock) {
        ESP_LOGE(WIFI_TAG, "lost connet to client");
        osh_invoke_event(OSH_NODE_EVENT_DISCONNECT);
        return OSH_ERR_NET_LINK;
    }
    int err = sendto(g_wifi_conn.sock, g_node_wifi.tx_buff, g_node_wifi.tx_len, 0,
        (struct sockaddr *)&g_wifi_conn.source_addr, sizeof(g_wifi_conn.source_addr));
    if (0 > err) {
        ESP_LOGE(WIFI_TAG, "failed to send response. errno: %d", err);
        return OSH_ERR_NET_SEND;
    }
    return ESP_OK;
}

/** -------------------------------
 *            functions
 *  -------------------------------
*/

/* init node WiFi */
static esp_err_t osh_node_init_wifi(size_t rx_buff_size, size_t tx_buff_size,
                osh_node_proto *proto, void *arg) {
    memset(&g_wifi_conn, 0, sizeof(g_wifi_conn));
    /* init buffer */
    g_node_wifi.rx_buff = malloc(rx_buff_size);
    if (NULL == g_node_wifi.rx_buff) {
        ESP_LOGE(WIFI_TAG, "failed to malloc mem for wifi rx_buffer");
        return ESP_ERR_NO_MEM;
    }
    memset(g_node_wifi.rx_buff, 0, rx_buff_size);
    g_node_wifi.rx_size = rx_buff_size;
    g_node_wifi.rx_len = 0;

    g_node_wifi.tx_buff = malloc(tx_buff_size);
    if (NULL == g_node_wifi.tx_buff) {
        ESP_LOGE(WIFI_TAG, "failed to malloc mem for wifi tx_buffer");
        return ESP_ERR_NO_MEM;
    }
    memset(g_node_wifi.tx_buff, 0, tx_buff_size);
    g_node_wifi.tx_size = tx_buff_size;
    g_node_wifi.tx_len = 0;

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* set Node name */
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(g_node_wifi.node_name, NODE_NAME_LEN, "OSH_%02X%02X%02X",
             eth_mac[3], eth_mac[4], eth_mac[5]);

    g_wifi_conn.sock = -1;
    g_node_wifi.arg = arg;
    g_node_wifi.proto = proto;
    return ESP_OK;
}

/* reset node wifi */
static esp_err_t osh_node_reset_wifi(void) {
    memset(g_node_wifi.rx_buff, 0, g_node_wifi.rx_size);
    g_node_wifi.rx_len = 0;
    memset(g_node_wifi.tx_buff, 0, g_node_wifi.tx_size);
    g_node_wifi.tx_len = 0;

    esp_wifi_restore();
    return ESP_OK;
}

/* get node name */
static const char* osh_node_get_name_wifi(void) {
    return (const char*)g_node_wifi.node_name;
}

/** -------------------------------
 *         events
 *  -------------------------------
*/

/**
 * state: OSH_FSM_STATE_INIT
 * event: OSH_NODE_EVENT_POWERON
 * next state: OSH_FSM_STATE_INIT, wait for network provision and connect
*/
static esp_err_t osh_node_wifi_init_poweron(void *config, void *arg) {

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(
                        WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                        &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
                        PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID,
                        &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
                        WIFI_EVENT, ESP_EVENT_ANY_ID,
                        &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
                        IP_EVENT, IP_EVENT_STA_GOT_IP,
                        &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t wifi_config = {
        /* What is the Provisioning Scheme that we want ?
         * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
        .scheme = wifi_prov_scheme_softap,

        /* Any default scheme specific event handler that you would
         * like to choose. Since our example application requires
         * neither BT nor BLE, we can choose to release the associated
         * memory once provisioning is complete, or not needed
         * (in case when device is already provisioned). Choosing
         * appropriate scheme specific event handler allows the manager
         * to take care of this automatically. This can be set to
         * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(wifi_config));

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    /* If device is not yet provisioned start provisioning service */
    if (!provisioned) {
        ESP_LOGI(WIFI_TAG, "Starting provisioning");

        /* What is the Device Service Name that we want
         * This translates to :
         *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
         *     - device name when scheme is wifi_prov_scheme_ble
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_2;
        /* The username must be the same one, which has been used in the generation of salt and verifier */

        /* This pop field represents the password that will be used to generate salt and verifier.
         * The field is present here in order to generate the QR code containing password.
         * In production this password field shall not be stored on the device */
        const char *username  = EXAMPLE_PROV_SEC2_USERNAME;
        const char *pop = EXAMPLE_PROV_SEC2_PWD;

        /* This is the structure for passing security parameters
         * for the protocomm security 2.
         * If dynamically allocated, sec2_params pointer and its content
         * must be valid till WIFI_PROV_END event is triggered.
         */
        wifi_prov_security2_params_t sec2_params = {};

        ESP_ERROR_CHECK(example_get_sec2_salt(
                            &sec2_params.salt, &sec2_params.salt_len));
        ESP_ERROR_CHECK(example_get_sec2_verifier(
                            &sec2_params.verifier, &sec2_params.verifier_len));

        wifi_prov_security2_params_t *sec_params = &sec2_params;

        /* What is the service key (could be NULL)
         * This translates to :
         *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
         *          (Minimum expected length: 8, maximum 64 for WPA2-PSK)
         *     - simply ignored when scheme is wifi_prov_scheme_ble
         */
        const char *service_key = NULL;

        /* Do not stop and de-init provisioning even after success,
         * so that we can restart it later. */

        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
                            security, (const void *) sec_params,
                            g_node_wifi.node_name, service_key));

        /* Uncomment the following to wait for the provisioning to finish and then release
         * the resources of the manager. Since in this case de-initialization is triggered
         * by the default event loop handler, we don't need to call the following */
        // wifi_prov_mgr_wait();
        // wifi_prov_mgr_deinit();
        /* Print QR code for provisioning */
        wifi_prov_print_qr(g_node_wifi.node_name, username, pop, PROV_TRANSPORT_SOFTAP);
    } else {
        ESP_LOGI(WIFI_TAG, "Already provisioned, starting Wi-Fi STA");

        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit();

        /* Start Wi-Fi station */
        wifi_init_sta();
    }
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_INIT
 * event: OSH_NODE_EVENT_CONNECT
 * next state: OSH_FSM_STATE_IDLE, wait for request
*/
static esp_err_t osh_node_wifi_init_connect(void *config, void *arg) {
    esp_err_t err = ESP_FAIL;

    // start UDP server to recv message
    if (ESP_OK != (err = start_udp_server(arg))) return err;

    // start mDNS
     if (ESP_OK != (err = start_mdns())) return err;

    // change state to OSH_FSM_STATE_IDLE
    osh_fsm_set_state(OSH_FSM_STATE_IDLE);

    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_IDLE, OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_DISCONNECT
 * next state: OSH_FSM_STATE_INIT, wait for connect
*/
static esp_err_t osh_node_wifi_on_disconnect(void *config, void *arg) {
    // stop mDNS
    stop_mdns();

    // stop UDP server
    stop_udp_server(NULL);

    // change state to OSH_FSM_STATE_INIT
    osh_fsm_set_state(OSH_FSM_STATE_INIT);
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_IDLE
 * event: OSH_NODE_EVENT_T_SAVE
 * next state: OSH_FSM_STATE_SAVING, wait for wakeup
*/
static esp_err_t osh_node_wifi_idle_t_save(void *config, void *arg) {
    //  TODO
    // power save

    // change state to OSH_FSM_STATE_SAVING
    osh_fsm_set_state(OSH_FSM_STATE_SAVING);
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_IDLE, OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_REQUEST
 * next state: OSH_FSM_STATE_ONGOING, wait for request or invoke
*/
static esp_err_t osh_node_wifi_on_request(void *config, void *arg) {
    if (NULL == g_node_wifi.proto) return ESP_OK;

    // call msg handle callback

    // change state to OSH_FSM_STATE_ONGOING
    if (OSH_FSM_STATE_ONGOING != osh_fsm_get_state()) osh_fsm_set_state(OSH_FSM_STATE_ONGOING);
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_IDLE, OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_UPDATE
 * next state: OSH_FSM_STATE_UPGRADING, wait for update
*/
static esp_err_t osh_node_wifi_on_update(void *config, void *arg) {

    // change state to OSH_FSM_STATE_UPGRADING
    osh_fsm_set_state(OSH_FSM_STATE_UPGRADING);
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_T_IDLE
 * next state: OSH_FSM_STATE_IDLE, wait for request or invoke
*/
static esp_err_t osh_node_wifi_ongoing_t_idle(void *config, void *arg) {

    // change state to OSH_FSM_STATE_IDLE
    osh_fsm_set_state(OSH_FSM_STATE_IDLE);
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_DOWNLOAD
 * next state: OSH_FSM_STATE_UPGRADING, wait for ota completed or failed
*/
static esp_err_t osh_node_wifi_upgrading_download(void *config, void *arg) {
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_OTA_COMPLETE
 * next state: OSH_FSM_STATE_INIT, restart
*/
static esp_err_t osh_node_wifi_upgrading_complete(void *config, void *arg) {

    // change state to OSH_FSM_STATE_ONGOING
    osh_fsm_set_state(OSH_FSM_STATE_INIT);
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_UPGRADING
 * event: OSH_NODE_EVENT_OTA_ROLLBACK
 * next state: OSH_FSM_STATE_INIT, restart
*/
static esp_err_t osh_node_wifi_upgrading_rollback(void *config, void *arg) {

    // change state to OSH_FSM_STATE_ONGOING
    osh_fsm_set_state(OSH_FSM_STATE_INIT);
    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_SAVING
 * event: OSH_NODE_EVENT_T_WAKEUP
 * next state: OSH_FSM_STATE_IDLE, wait for request or invoke
*/
static esp_err_t osh_node_wifi_saving_t_wakeup(void *config, void *arg) {

    // change state to OSH_FSM_STATE_IDLE
    osh_fsm_set_state(OSH_FSM_STATE_IDLE);
    return ESP_OK;
}

osh_node_network *osh_node_network_wifi_create(void) {
    osh_node_network *net = &g_network;
    memset(net, 0, sizeof(osh_node_network));
    net->init = osh_node_init_wifi;
    net->reset = osh_node_reset_wifi;
    net->get_name = osh_node_get_name_wifi;
    net->reponse = send_udp_response;
    net->init_poweron = osh_node_wifi_init_poweron;
    net->init_connect = osh_node_wifi_init_connect;
    net->idle_disconnect = osh_node_wifi_on_disconnect;
    net->idle_t_save = osh_node_wifi_idle_t_save;
    net->idle_request = osh_node_wifi_on_request;
    net->idle_update = osh_node_wifi_on_update;
    net->ongoing_t_idle = osh_node_wifi_ongoing_t_idle;
    net->ongoing_request = osh_node_wifi_on_request;
    net->ongoing_disconnect = osh_node_wifi_on_disconnect;
    net->ongoing_update = osh_node_wifi_on_update;
    net->upgrading_download = osh_node_wifi_upgrading_download;
    net->upgrading_complete = osh_node_wifi_upgrading_complete;
    net->upgrading_rollback = osh_node_wifi_upgrading_rollback;
    net->saving_t_wakeup = osh_node_wifi_saving_t_wakeup;
    return net;
}

osh_node_network *osh_node_network_wifi_get(void) {
    return &g_network;
}
