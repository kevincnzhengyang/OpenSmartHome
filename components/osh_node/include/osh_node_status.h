/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:12:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 00:12:56
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_status.h
 * @Description : status RGB LED and reset button
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_STATUS_H
#define OSH_NODE_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"


/* init RGB LED for status indicator */
esp_err_t osh_node_status_init(void);

/* start status task */
esp_err_t osh_node_status_start(void);

/* init GPIO button for reset network config */
esp_err_t osh_node_reset_btn_init(void);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_STATUS_H */