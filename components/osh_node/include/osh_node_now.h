/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-05-28 09:44:10
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-28 09:44:10
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_now.h
 * @Description : ESP-NOW protocol
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_NOW_H
#define OSH_NODE_NOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"

#include "esp_now.h"

/* init ESP-NOW proto */
esp_err_t osh_node_now_init(void *conf_arg);

/* start ESP-NOW proto */
esp_err_t osh_node_now_start(void *run_arg);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_NOW_H */