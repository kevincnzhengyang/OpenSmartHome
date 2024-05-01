/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 11:39:22
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 11:39:23
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_errors.h
 * @Description : errors definition
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_ERRORS_H
#define OSH_NODE_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>

// error base
#define OSH_ERR_NET_BASE            0x100000    /*!< Starting number of network error codes */
#define OSH_ERR_FSM_BASE            0x200000    /*!< Starting number of FSM error codes */
#define OSH_ERR_APP_BASE            0x300000    /*!< Starting number of application error codes */
#define OSH_ERR_OPER_BASE           0x400000    /*!< Starting number of operation error codes */


#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_ERRORS_H */
