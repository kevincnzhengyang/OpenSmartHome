/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:13:16
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 00:13:16
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node.h
 * @Description : Open Smart Home Node public header file
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_H
#define OSH_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"

/**
 *  error code
*/
#define OSH_ERR_NODE_NVS_BASE           (OSH_ERR_NODE_BASE + 0x20000)
#define OSH_ERR_NODE_NAME_INVALID       (OSH_ERR_NODE_BASE + 1)

/* balckboard shared among all modules */
typedef struct {
    /**
     * logical name restore from nvs flash after power up
     *
     * R: all modules
     * W: app module
     *
    */
    char                       *node_name;

    /**
     * device name from MAC address
     *
     * R: all modules
     * W: network module
    */
    char                        *dev_name;
} osh_node_bb_t;

/* callback for module init */
typedef esp_err_t (*module_init_cb)(osh_node_bb_t *node_bb, void *conf_arg);

/* callback for module start */
typedef esp_err_t (*module_start_cb)(void *run_arg);

/* structure for module */
typedef struct {
    char                          *name;
    module_init_cb          init_module;
    void                      *conf_arg;
    module_start_cb        start_module;
    void                       *run_arg;
} osh_node_module_t;

/**
 * init OSH Node
 *
 * @proto_buff_size: size for COAP proto buffer
 * @conf_arg: user define argument
*/
esp_err_t osh_node_init(void);

/**
 * init modules
*/
esp_err_t osh_node_modules_init(osh_node_module_t *pmods, size_t num);

/**
 * start modules
*/
esp_err_t osh_node_modules_start(void);

/**
 * update node name
*/
esp_err_t osh_node_node_name_update(const char *name);

/**
 * update device name
*/
esp_err_t osh_node_dev_name_update(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_H */