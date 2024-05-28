/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:10:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-28 23:14:17
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "osh_node.h"
#include "osh_node.inc"

#include "nvs_flash.h"

static const char *NODE_TAG = "NODE";

static osh_node_t g_node;


static esp_err_t write_string_to_nvs(const char* key, const char* value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("osh_node_bb", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NODE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return OSH_ERR_NODE_NVS_BASE + err;
    }

    err = nvs_set_str(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(NODE_TAG, "Failed to write string to NVS (%s)", esp_err_to_name(err));
        goto failed;
    } else {
        ESP_LOGI(NODE_TAG, "Successfully written string to NVS");
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NODE_TAG, "Failed to commit to NVS (%s)", esp_err_to_name(err));
        goto failed;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
failed:
    nvs_close(nvs_handle);
    return OSH_ERR_NODE_NVS_BASE + err;
}

static esp_err_t read_string_from_nvs(const char* key, char **pvalue) {
    nvs_handle_t nvs_handle;
    char *value = NULL;

    esp_err_t err = nvs_open("osh_node_bb", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NODE_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return OSH_ERR_NODE_NVS_BASE + err;
    }

    size_t required_size = 0;  // Required size variable
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (ESP_OK != err) {
        if (ESP_ERR_NVS_NOT_FOUND == err) {
            ESP_LOGW(NODE_TAG, "Failed to find [%s] from NVS", key);
            *pvalue = NULL;
            nvs_close(nvs_handle);
            return ESP_OK;  // not a error
        } else {
            ESP_LOGE(NODE_TAG, "Failed to get string size from NVS (%s)", esp_err_to_name(err));
            goto failed;
        }
    }

    value = (char *)malloc(required_size);
    if (value == NULL) {
        ESP_LOGE(NODE_TAG, "Failed to allocate memory for string [%s]", key);
        nvs_close(nvs_handle);
        err = ESP_ERR_NO_MEM;
        goto failed;
    }

    err = nvs_get_str(nvs_handle, key, value, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(NODE_TAG, "Failed to get [%s] from NVS (%s)", key, esp_err_to_name(err));
        goto failed;
    } else {
        ESP_LOGI(NODE_TAG, "Read [%s] from NVS: %s", key, value);
    }

    *pvalue = value;
    nvs_close(nvs_handle);
    return ESP_OK;

failed:
    if (NULL != value) free(value);
    *pvalue = NULL;
    nvs_close(nvs_handle);
    return OSH_ERR_NODE_NVS_BASE + err;
}

/**
 * Init OSH Node
 *
 * @proto_buff_size: size for COAP proto buffer
 * @conf_arg: user define argument
*/
esp_err_t osh_node_init(void) {
    memset(&g_node, 0, sizeof(osh_node_t));
    vListInitialise(&g_node.module_list);

    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
            * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* restore node name from nvs flash */
    ESP_ERROR_CHECK(read_string_from_nvs("node_name", &g_node.node_bb.node_name));

    /* restore device name from nvs flash */
    ESP_ERROR_CHECK(read_string_from_nvs("dev_name", &g_node.node_bb.dev_name));

    return ESP_OK;
}


/**
 * start running of OSH Node
*/
esp_err_t osh_node_start(void) {
    /* init modules one by one */
    // TODO

    /* Init Status LED */
    // ESP_ERROR_CHECK(osh_node_status_init());

    /* Init Reset BUtton */
    // ESP_ERROR_CHECK(osh_node_reset_btn_init());

    /* Init Network */
    // ESP_ERROR_CHECK(osh_node_wifi_init(proto_buff_size, conf_arg));


    // start status
    // ESP_ERROR_CHECK(osh_node_status_start());

    // start WiFi
    // ESP_ERROR_CHECK(osh_node_network_start(run_arg));

    /* start modules one by one */
    // TODO

    // move to wifi start

    return ESP_OK;
}

/**
 * register module
*/
esp_err_t osh_node_module_register(const char *name,
                    module_init_cb init_module, void *conf_arg,
                    module_start_cb start_module, void *run_arg) {
    osh_node_module_t *mod = (osh_node_module_t *)malloc(sizeof(osh_node_module_t));
    if (NULL == mod) {
        ESP_LOGE(NODE_TAG, "failed to malloc mem for module %s", name);
        return ESP_ERR_NO_MEM;
    }

    mod->name = strdup(name);
    mod->init_cb = init_module;
    mod->conf_arg = conf_arg;
    mod->start_cb = start_module;
    mod->run_arg = run_arg;
    vListInitialiseItem(&mod->mod_item);

    // add modeule to list
    listSET_LIST_ITEM_OWNER(&mod->mod_item, mod);
    vListInsert(&g_node.module_list, &mod->mod_item);
    ESP_LOGI(NODE_TAG, "register module %s", name);
    return ESP_OK;
}

/**
 * update node name
*/
esp_err_t osh_node_node_name_update(const char *name) {
    if (NULL == name) return OSH_ERR_NODE_NAME_INVALID;

    if (NULL != g_node.node_bb.node_name) {
        // free if already set
        free(g_node.node_bb.node_name);
        g_node.node_bb.node_name = NULL;
    }

    // set and save
    g_node.node_bb.node_name = strdup(name);

    return write_string_to_nvs("node_name", g_node.node_bb.node_name);
}

/**
 * update device name
*/
esp_err_t osh_node_dev_name_update(const char *name) {
    if (NULL == name) return OSH_ERR_NODE_NAME_INVALID;

    if (NULL != g_node.node_bb.dev_name) {
        // free if already set
        free(g_node.node_bb.dev_name);
        g_node.node_bb.dev_name = NULL;
    }

    // set and save
    g_node.node_bb.dev_name = strdup(name);

    return write_string_to_nvs("dev_name", g_node.node_bb.dev_name);
}
