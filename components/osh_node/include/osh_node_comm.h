/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 10:40:47
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 10:40:48
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_comm.h
 * @Description : common definitions
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_COMM_H
#define OSH_NODE_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <freertos/list.h>

#include "esp_log.h"


/* macro for traverse the list of FreeRTOS */
#define listFOR_EACH_ENTRY(plist, type, owner) \
    for(ListItem_t const *pos = listGET_HEAD_ENTRY(plist);    \
        owner = (type *)listGET_LIST_ITEM_OWNER(pos), pos != listGET_END_MARKER(plist); \
        pos = listGET_NEXT(pos))

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_COMM_H */

