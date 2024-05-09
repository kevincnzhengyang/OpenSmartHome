/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 11:35:44
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-08 19:42:36
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
typedef esp_err_t (* osh_FSM_step_callback)(void *arg);
typedef esp_err_t (* osh_FSM_field_callback)(OSH_FSM_STATES_ENUM current_state,
                                            EventBits_t group_bits, void *arg);

/* state item of FSM */
typedef struct {
    OSH_FSM_STATES_ENUM             state;
    EventBits_t                group_bits;
    List_t                    events_list;
    ListItem_t                 state_item;
} osh_FSM_state_stru;

/* event item of state */
typedef struct {
    ListItem_t                 event_item;
    EventBits_t                      bits;
    osh_FSM_event_callback       callback;
    void                             *arg;
} osh_FSM_event_stru;

/* FSM*/
typedef struct {
    OSH_FSM_STATES_ENUM     current_state;
    List_t                    states_list;
    EventGroupHandle_t        event_group;
    osh_FSM_step_callback   init_callback;      // init FSM
    osh_FSM_step_callback   fini_callback;      // fini FSM
    osh_FSM_field_callback   pre_callback;      // before loop step
    osh_FSM_field_callback  post_callback;      // after loop step
    osh_FSM_step_callback   miss_callback;      // unknown state
    osh_FSM_step_callback  event_callback;      // found event
    void                             *arg;      // user defined data
} osh_FSM_stru;

/* singleton FSM */
extern osh_FSM_stru *g_osh_FSM;

/* check and create if FSM not exists */
esp_err_t osh_fsm_verify(void);

/* get or create if state not exists */
esp_err_t osh_fsm_fetch_state(OSH_FSM_STATES_ENUM state,
                osh_FSM_state_stru **fsm_state);

/* get event */
esp_err_t osh_fsm_get_event(osh_FSM_state_stru *fsm_state,
                EventBits_t bits, osh_FSM_event_stru **fsm_event);

/* create event */
esp_err_t osh_fsm_create_event(osh_FSM_state_stru *fsm_state, EventBits_t bits,
                osh_FSM_event_callback callback, osh_FSM_event_stru **fsm_event);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_FSM_INC */
