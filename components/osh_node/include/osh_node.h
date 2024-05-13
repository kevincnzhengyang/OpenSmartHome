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
 * Init OSH Node
 *
 * @proto_buff_size: size for COAP proto buffer
 * @conf_arg: user define argument
*/
esp_err_t osh_node_init(size_t proto_buff_size, void *conf_arg);


/**
 * start running of OSH Node
*/
esp_err_t osh_node_start(void *run_arg);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_H */