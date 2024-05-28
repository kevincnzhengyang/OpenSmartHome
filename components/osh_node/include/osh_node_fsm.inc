/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 11:35:44
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-28 22:21:22
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_fsm.inc
 * @Description :  node FSM private header file
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#ifndef OSH_NODE_FSM_INC
#define OSH_NODE_FSM_INC

#ifdef __cplusplus
extern "C" {
#endif

#include <freertos/list.h>

#include "osh_node_fsm.h"
#include "osh_node_errors.h"

/* callback for FSM */
typedef esp_err_t (* osh_node_fsm_step_cb)(void *conf_arg);

typedef esp_err_t (* osh_node_fsm_field_cb)(OSH_FSM_STATES_ENUM current_state,
                                            EventBits_t group_bits,
                                            void *conf_arg);

/* state item of FSM */
typedef struct {
    OSH_FSM_STATES_ENUM             state;
    EventBits_t                group_bits;
    List_t                    events_list;
    ListItem_t                 state_item;
} osh_node_fsm_state_t;

/* event item of state */
typedef struct {
    ListItem_t                 event_item;
    EventBits_t                      bits;
    osh_node_fsm_event_cb        callback;
    void                        *conf_arg;
} osh_node_fsm_event_t;

/* FSM*/
typedef struct {
    osh_node_bb_t                *node_bb;
    OSH_FSM_STATES_ENUM     current_state;
    List_t                    states_list;
    EventGroupHandle_t        event_group;
    osh_node_fsm_step_cb    init_callback;      // init FSM
    osh_node_fsm_step_cb    fini_callback;      // fini FSM
    osh_node_fsm_field_cb    pre_callback;      // before loop step
    osh_node_fsm_field_cb   post_callback;      // after loop step
    osh_node_fsm_step_cb    miss_callback;      // unknown state
    void                        *conf_arg;      // user defined data
} osh_node_fsm_t;

/* singleton FSM */
// extern  osh_node_fsm_t *g_osh_fsm;

/* check and create if FSM not exists */
esp_err_t osh_fsm_verify(void);

/* get or create if state not exists */
esp_err_t osh_fsm_fetch_state(OSH_FSM_STATES_ENUM state,
                osh_node_fsm_state_t **fsm_state);

/* get event */
esp_err_t osh_fsm_get_event(osh_node_fsm_state_t *fsm_state,
                EventBits_t bits, osh_node_fsm_event_t **fsm_event);

/* create event */
esp_err_t osh_fsm_create_event(osh_node_fsm_state_t *fsm_state, EventBits_t bits,
                osh_node_fsm_event_cb callback, osh_node_fsm_event_t **fsm_event);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_FSM_INC */
