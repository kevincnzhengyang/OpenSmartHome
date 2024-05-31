/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:10:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-31 21:53:18
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

    esp_err_t err = nvs_open("osh_node_bb", NVS_READWRITE, &nvs_handle);
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
 * init modules
*/
esp_err_t osh_node_modules_init(osh_node_module_t *pmods, size_t num) {
    if (0 == num) return ESP_OK;

    g_node.modules = pmods;
    g_node.mod_num = num;

    /* init modules */
    for (int i = 0; i < num; i++) {
        osh_node_module_t *mod = &g_node.modules[i];
        if (NULL != mod && NULL != mod->init_module) {
            ESP_LOGI(NODE_TAG, "init module %s ...", mod->name);
            ESP_ERROR_CHECK(mod->init_module(&g_node.node_bb, mod->conf_arg));
        }
    }
    return ESP_OK;
}

/**
 * start running of OSH Node
*/
esp_err_t osh_node_modules_start(void) {
    /* init modules one by one */
    if (0 == g_node.mod_num)  return ESP_OK;

    /* start modules */
    for (int i = 0; i < g_node.mod_num; i++) {
        osh_node_module_t *mod = &g_node.modules[i];
        if (NULL != mod && NULL != mod->start_module) {
            ESP_LOGI(NODE_TAG, "start module %s ...", mod->name);
            ESP_ERROR_CHECK(mod->start_module(mod->run_arg));
        }
    }
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
