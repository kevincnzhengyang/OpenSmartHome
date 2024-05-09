/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 11:16:44
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-04-30 11:16:46
 * @FilePath    : /OpenSmartHome/components/osh_node/include/osh_node_fsm.h
 * @Description : node FSM machine
 * @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */
#ifndef OSH_NODE_FSM_H
#define OSH_NODE_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"

/* errors */
#define OSH_ERR_FSM_NOT_INIT            (OSH_ERR_FSM_BASE +     1)
#define OSH_ERR_FSM_UNKNOW_STATE        (OSH_ERR_FSM_BASE +     2)
#define OSH_ERR_FSM_MISS_STATE          (OSH_ERR_FSM_BASE +     3)
#define OSH_ERR_FSM_INNER               (OSH_ERR_FSM_BASE +     4)
#define OSH_ERR_FSM_DUP_EVENT           (OSH_ERR_FSM_BASE +     5)
#define OSH_ERR_FSM_MISS_EVENT          (OSH_ERR_FSM_BASE +     6)
#define OSH_ERR_FSM_INVALID_STATE       (OSH_ERR_FSM_BASE +     7)
#define OSH_ERR_FSM_INVALID_EVENT       (OSH_ERR_FSM_BASE +     8)

/* Finite State Machine States Enumerate */
typedef enum {
    OSH_FSM_STATE_INIT          = 0,        // init state
    OSH_FSM_STATE_IDLE          = 1,        // idle
    OSH_FSM_STATE_ONGOING       = 2,        // ongoing
    OSH_FSM_STATE_SAVING        = 3,        // saving
    OSH_FSM_STATE_UPGRADING     = 4,        // upgrading
    OSH_FSM_STATE_BUTT
} OSH_FSM_STATES_ENUM;

/* Callback for FSM */
typedef esp_err_t (* osh_node_fsm_event_cb)(void *e_conf_arg, void *run_arg);

/* init FSM */
esp_err_t osh_node_fsm_init(void *f_conf_arg);

/* register a event and corresponding callback to a state */
esp_err_t osh_node_fsm_register_event(const OSH_FSM_STATES_ENUM state, const EventBits_t uxBitsToWaitFor,
                                     osh_node_fsm_event_cb handle, void *e_conf_arg);

/* run FSM */
esp_err_t osh_node_fsm_loop_step(void *run_arg);

/* fini FSM */
esp_err_t osh_node_fsm_fini(void);

/* invoke event */
esp_err_t osh_node_fsm_invoke_event(const EventBits_t uxBitsToSet);
esp_err_t osh_node_fsm_invoke_event_from_ISR(const EventBits_t uxBitsToSet);

/* get state */
OSH_FSM_STATES_ENUM osh_node_fsm_get_state(void);

/* set state */
esp_err_t osh_node_fsm_set_state(OSH_FSM_STATES_ENUM state);

#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_FSM_H */
