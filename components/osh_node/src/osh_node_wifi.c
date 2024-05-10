/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 22:36:41
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-11 01:00:26
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_wifi.c
 * @Description : WiFi network
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "wifi_provisioning/manager.h"
#include <wifi_provisioning/scheme_softap.h>
#include "esp_netif.h"
#include "qrcode.h"

#include "esp_ping.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "ping/ping_sock.h"

#include "mbedtls/pk.h"
#include "mbedtls/md.h"
#include "mbedtls/error.h"

#include "osh_node_network.h"
#include "osh_node_wifi.h"
#include "osh_node_events.h"
#include "osh_node_fsm.h"
#include "osh_node_proto.h"

const char *WIFI_TAG = "WiFi";

#define NODE_NAME_LEN                      12
#define MAX_PING_TIMEOUT                    5

/* network */
typedef struct
{
    char      node_name[NODE_NAME_LEN];
    uint8_t              timeout_count;
    TimerHandle_t           ping_timer;
    ip_addr_t               gateway_ip;
    void                     *conf_arg;
} osh_node_network;


static osh_node_network g_node_wifi;

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
                osh_node_fsm_invoke_event(OSH_NODE_EVENT_DISCONNECT);
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
        osh_node_fsm_invoke_event(OSH_NODE_EVENT_CONNECT);

        // Get the DNS server IP dynamically
        esp_netif_dns_info_t dns;
        esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dns);
        memcpy(&g_node_wifi.gateway_ip, &dns.ip, sizeof(ip_addr_t));
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

static void ping_on_success(esp_ping_handle_t hdl, void *arg) {
    ESP_LOGI(WIFI_TAG, "ping gateway OK");
    g_node_wifi.timeout_count = 0;  // reset
}

static void ping_on_timeout(esp_ping_handle_t hdl, void *args) {
    ESP_LOGE(WIFI_TAG, "ping gateway timeout");
    if (MAX_PING_TIMEOUT > ++g_node_wifi.timeout_count) {
        // invoke event
        g_node_wifi.timeout_count = 0; // reset
        osh_node_fsm_invoke_event(OSH_NODE_EVENT_DISCONNECT);
        esp_wifi_connect();
    }
}

static void ping_timeout_cb(TimerHandle_t timer) {
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = g_node_wifi.gateway_ip;

    /* set callback functions */
    esp_ping_callbacks_t cbs = {
        .cb_args = NULL,
        .on_ping_success = ping_on_success,
        .on_ping_timeout = ping_on_timeout,
        .on_ping_end = NULL
    };
    esp_ping_handle_t ping;
    esp_ping_new_session(&config, &cbs, &ping);

    // start ping
    esp_ping_start(ping);

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
    // TODO start coap proto

    // change state to OSH_FSM_STATE_IDLE
    osh_node_fsm_set_state(OSH_FSM_STATE_IDLE);

    // start ping timer
    xTimerStart(g_node_wifi.ping_timer, 0);

    return ESP_OK;
}

/**
 * state: OSH_FSM_STATE_IDLE, OSH_FSM_STATE_ONGOING
 * event: OSH_NODE_EVENT_DISCONNECT
 * next state: OSH_FSM_STATE_INIT, wait for connect
*/
static esp_err_t osh_node_wifi_on_disconnect(void *config, void *arg) {
    // TODO stop coap proto

    // change state to OSH_FSM_STATE_INIT
    osh_node_fsm_set_state(OSH_FSM_STATE_INIT);

    // stop ping timer
    xTimerStop(g_node_wifi.ping_timer, 0);

    return ESP_OK;
}

/** -------------------------------
 *            functions
 *  -------------------------------
*/

/* init WiFi */
esp_err_t osh_node_wifi_init(void *conf_arg) {
    memset(&g_node_wifi, 0, sizeof(osh_node_network));

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

    g_node_wifi.conf_arg = conf_arg;

    g_node_wifi.ping_timer = xTimerCreate("ping_timer",
                                    pdMS_TO_TICKS(CONFIG_NODE_WIFI_CHECK_PERIOD),
                                    pdTRUE, NULL,
                                    ping_timeout_cb);
    if (g_node_wifi.ping_timer == NULL) {
        ESP_LOGE(WIFI_TAG, "Failed to create ping timer");
        return OSH_ERR_NET_INNER;
    }

    //  TODO
    /* register event callback to FSM */
    return ESP_OK;
}

/* start WiFi */
esp_err_t osh_node_wifi_start(void *run_arg) {
    // TODO start timer for check WiFi
    osh_node_fsm_invoke_event(OSH_NODE_EVENT_POWERON);
    return ESP_OK;
}

/* reset WiFi */
esp_err_t osh_node_wifi_reset(void) {
    esp_wifi_restore();
    return ESP_OK;
}

/* get node dev name */
const char *osh_node_wifi_get_dev_name(void) {
    return (const char *)g_node_wifi.node_name;
}
