
/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 00:13:16
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 00:13:16
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node.h
 * @Description : Open Smark Home Node private header file
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_INC
#define OSH_NODE_INC

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node.h"

#include <freertos/list.h>


/* OSH module */
typedef struct {
    char                            *name;
    module_init_cb                init_cb;
    void                        *conf_arg;
    module_start_cb              start_cb;
    void                         *run_arg;
    ListItem_t                   mod_item;
} osh_node_module_t;


/* OSH node */
typedef struct osh_node_stru {
    osh_node_bb_t                 node_bb;
    List_t                    module_list;
} osh_node_t;


#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_INC */
