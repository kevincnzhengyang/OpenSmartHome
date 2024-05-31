/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-30 13:05:39
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-31 23:19:53
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_fsm.c
 * @Description : FSM
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "osh_node_fsm.h"
#include "osh_node_fsm.inc"

static const char *FSM_TAG = "FSM";

/* singleton FSM */
 osh_node_fsm_t *g_osh_fsm = NULL;

/* check and create if FSM not exists */
esp_err_t osh_fsm_verify(void) {
    if (NULL != g_osh_fsm) return ESP_OK;
    g_osh_fsm = malloc(sizeof( osh_node_fsm_t));
    if (NULL == g_osh_fsm) {
        ESP_LOGE(FSM_TAG, "failed to malloc FSM");
        return ESP_ERR_NO_MEM;
    }
    memset(g_osh_fsm, 0, sizeof( osh_node_fsm_t));
    g_osh_fsm->event_group = xEventGroupCreate();
    if (NULL == g_osh_fsm->event_group) {
        ESP_LOGE(FSM_TAG, "failed to create event group");
        free(g_osh_fsm);
        return OSH_ERR_FSM_INNER;
    }
    g_osh_fsm->current_state = OSH_FSM_STATE_BUTT;

    return ESP_OK;
}

static esp_err_t osh_fsm_create_state(OSH_FSM_STATES_ENUM state,
                osh_node_fsm_state_t **fsm_state) {
    // create
    osh_node_fsm_state_t *tmp_state = malloc(sizeof(osh_node_fsm_state_t));
    if (NULL == tmp_state) {
        ESP_LOGE(FSM_TAG, "failedto malloc FSM state %d", state);
        *fsm_state = NULL;
        return ESP_ERR_NO_MEM;
    }

    // init
    memset(tmp_state, 0, sizeof(osh_node_fsm_state_t));
    vListInitialiseItem(&tmp_state->state_item);
    vListInitialise(&tmp_state->events_list);
    listSET_LIST_ITEM_OWNER(&tmp_state->state_item, tmp_state);

    // insert to list
    tmp_state->state_item.xItemValue = (int)state;  // using state as value
    vListInsert(&g_osh_fsm->states_list, &tmp_state->state_item);

    ESP_LOGI(FSM_TAG, "create state %d", state);
    *fsm_state = tmp_state;
    return ESP_OK;
}

/* get or create if state not exists */
esp_err_t osh_fsm_fetch_state(OSH_FSM_STATES_ENUM state,
                osh_node_fsm_state_t **fsm_state) {
    if (NULL == g_osh_fsm) return OSH_ERR_FSM_NOT_INIT;

    // traverse to find the state
    if (listLIST_IS_EMPTY(&g_osh_fsm->states_list)) {
        // empty list
        return osh_fsm_create_state(state, fsm_state);
    }

    osh_node_fsm_state_t *tmp_state = NULL;
    listFOR_EACH_ENTRY(&g_osh_fsm->states_list, osh_node_fsm_state_t, tmp_state) {
        if (state == tmp_state->state) {
            ESP_LOGI(FSM_TAG, "find state %d", (int)state);
            *fsm_state = tmp_state;
            break;
        }
    }
    if (NULL == tmp_state) {
        ESP_LOGI(FSM_TAG, "can't find state %d", (int)state);
        return osh_fsm_create_state(state, fsm_state);
    }

    return ESP_OK;
}

/* get event */
esp_err_t osh_fsm_get_event(osh_node_fsm_state_t *fsm_state,
                EventBits_t bits, osh_node_fsm_event_t **fsm_event){

    if (NULL == fsm_state) {
        ESP_LOGE(FSM_TAG, "can't fetch event with NULL state");
        return OSH_ERR_FSM_MISS_STATE;
    }

    // traverse to find the event
    if (listLIST_IS_EMPTY(&fsm_state->events_list)) {
        // empty list
        ESP_LOGW(FSM_TAG, "state %d empty events", (int)fsm_state->state);
        return OSH_ERR_FSM_MISS_EVENT;
    }

    osh_node_fsm_event_t *tmp_event = NULL;
    listFOR_EACH_ENTRY(&fsm_state->events_list, osh_node_fsm_event_t, tmp_event) {
        if (bits == tmp_event->bits) {
            ESP_LOGD(FSM_TAG, "find event %d", (int)bits);
            *fsm_event = tmp_event;
            break;
        }
    }
    if (NULL == tmp_event) {
        ESP_LOGW(FSM_TAG, "can't find event %d at state %d",
                (int)bits, (int)fsm_state->state);
        *fsm_event = NULL;
        return OSH_ERR_FSM_MISS_EVENT;
    }

    return ESP_OK;
}

/* create event */
esp_err_t osh_fsm_create_event(osh_node_fsm_state_t *fsm_state, EventBits_t bits,
                osh_node_fsm_event_cb callback, osh_node_fsm_event_t **fsm_event) {
    if (NULL == fsm_state) {
        ESP_LOGE(FSM_TAG, "can't create event with NULL state");
        return OSH_ERR_FSM_MISS_STATE;
    }

    // Orthogonal event bits
    if (0 != (bits & fsm_state->group_bits)) {
        ESP_LOGE(FSM_TAG, "state %d bits %d already set",
                (int)fsm_state->state, (int)bits);
        return OSH_ERR_FSM_DUP_EVENT;
    }

    // create
    osh_node_fsm_event_t *tmp_event = malloc(sizeof(osh_node_fsm_event_t));
    if (NULL == tmp_event) {
        ESP_LOGE(FSM_TAG, "failedto malloc FSM state %d", (int)fsm_state->state);
        *fsm_event = NULL;
        return ESP_ERR_NO_MEM;
    }

    // init
    memset(tmp_event, 0, sizeof(osh_node_fsm_event_t));
    vListInitialiseItem(&tmp_event->event_item);
    listSET_LIST_ITEM_OWNER(&tmp_event->event_item, tmp_event);
    tmp_event->callback = callback;

    // insert to list
    tmp_event->event_item.xItemValue = (int)bits;  // using bits as value
    vListInsert(&fsm_state->events_list, &tmp_event->event_item);

    ESP_LOGI(FSM_TAG, "create event %d for state %d", (int)bits, (int)fsm_state->state);
    return ESP_OK;
}

/* init FSM */
esp_err_t osh_node_fsm_init(osh_node_bb_t *node_bb, void *f_conf_arg)
{
    if (ESP_OK != osh_fsm_verify()) return OSH_ERR_FSM_INNER;

    // init on NOT empty
    if (!listLIST_IS_EMPTY(&g_osh_fsm->states_list)) {
        ESP_LOGE(FSM_TAG, "Init FSM  states");
        return OSH_ERR_FSM_INNER;
    }

    // init state list
    vListInitialise(&g_osh_fsm->states_list);

    g_osh_fsm->node_bb = node_bb;
    g_osh_fsm->conf_arg = f_conf_arg;

    // callback
    if (NULL != g_osh_fsm->init_callback) {
        ESP_LOGI(FSM_TAG, "init callback...");
        return g_osh_fsm->init_callback(f_conf_arg);
    }
    return ESP_OK;
}

/* register a event and corresponding callback to a state */
esp_err_t osh_node_fsm_register_event(const OSH_FSM_STATES_ENUM state, const EventBits_t uxBitsToWaitFor,
                            osh_node_fsm_event_cb handle, void *e_conf_arg)
{
    ESP_LOGI(FSM_TAG, "register event [%d]@[%d]", (int)uxBitsToWaitFor, (int)state);
    if (NULL == g_osh_fsm) {
        ESP_LOGE(FSM_TAG, "failed to step forward with NULL FSM");
        return OSH_ERR_FSM_NOT_INIT;
    }

    // find state
    esp_err_t res = ESP_FAIL;
    osh_node_fsm_state_t *fsm_state = NULL;
    if (ESP_OK != (res = osh_fsm_fetch_state(state, &fsm_state))
        || NULL == fsm_state) return res;

    // find event before create a new one
    osh_node_fsm_event_t *fsm_event = NULL;
    if (ESP_OK == (res = osh_fsm_get_event(fsm_state, uxBitsToWaitFor, &fsm_event))
        && NULL != fsm_event) {
        ESP_LOGE(FSM_TAG, "failed to register event %d state %d",
                (int)uxBitsToWaitFor, (int)state);
        return OSH_ERR_FSM_DUP_EVENT;
    }
    if (ESP_OK != (res = osh_fsm_create_event(fsm_state, uxBitsToWaitFor, handle, &fsm_event))
        || NULL == fsm_event) return res;
    fsm_event->conf_arg = e_conf_arg;
    return ESP_OK;
}

/* run FSM */
esp_err_t osh_node_fsm_loop_step(void *run_arg)
{
    if (NULL == g_osh_fsm) {
        ESP_LOGE(FSM_TAG, "failed to step forward with NULL FSM");
        return OSH_ERR_FSM_NOT_INIT;
    }

    // find current state
    esp_err_t res = ESP_FAIL;
    osh_node_fsm_state_t *fsm_state = NULL;
    if (ESP_OK != (res = osh_fsm_fetch_state(g_osh_fsm->current_state, &fsm_state))
        || NULL == fsm_state) {
        ESP_LOGE(FSM_TAG, "unknow state %d", (int)g_osh_fsm->current_state);

        if (NULL != g_osh_fsm->miss_callback) {
            ESP_LOGI(FSM_TAG, "miss state callback...");
            return g_osh_fsm->miss_callback(g_osh_fsm->conf_arg);
        }

    }
    if (0 == fsm_state->group_bits || listLIST_IS_EMPTY(&fsm_state->events_list)) {
        ESP_LOGE(FSM_TAG, "no event config for state %d", (int)fsm_state->state);
        return OSH_ERR_FSM_INVALID_STATE;
    }

    // dispatch
    EventBits_t bits = xEventGroupWaitBits(g_osh_fsm->event_group, fsm_state->group_bits,
                            pdTRUE, pdFALSE, portMAX_DELAY);
    // pre callback
    if (NULL != g_osh_fsm->pre_callback) {
        ESP_LOGI(FSM_TAG, "pre callback...");
        if (ESP_OK != (res = g_osh_fsm->pre_callback(g_osh_fsm->current_state,
                                                bits, g_osh_fsm->conf_arg))) return res;
    }

    osh_node_fsm_event_t *fsm_event = NULL;
    listFOR_EACH_ENTRY(&fsm_state->events_list, osh_node_fsm_event_t, fsm_event) {
        if (0 != (bits & fsm_event->bits)) {
            ESP_LOGI(FSM_TAG, "matched %d event %d",
                    (int)fsm_state->state, (int)fsm_event->bits);
            if (NULL == fsm_event->callback) {
                ESP_LOGE(FSM_TAG, "state %d invalid event %d",
                    (int)fsm_state->state, (int)fsm_event->bits);
                return OSH_ERR_FSM_INVALID_EVENT;
            }
            ESP_LOGI(FSM_TAG, "state %d event %d callback...",
                    (int)fsm_state->state, (int)fsm_event->bits);
            if (ESP_OK != (res = fsm_event->callback(fsm_event->conf_arg, run_arg))) return res;
            // DO NOT break, tranverse all events
        }
    }

    // post callback
    if (NULL != g_osh_fsm->post_callback) {
        ESP_LOGI(FSM_TAG, "post callback...");
        if (ESP_OK != (res = g_osh_fsm->post_callback(g_osh_fsm->current_state,
                                                bits, g_osh_fsm->conf_arg))) return res;
    }
    return ESP_OK;
}

/* fini FSM */
esp_err_t osh_node_fsm_fini(void)
{
    if (NULL == g_osh_fsm) return ESP_OK;

    esp_err_t res = ESP_FAIL;

    // callback
    if (NULL != g_osh_fsm->fini_callback) {
        ESP_LOGI(FSM_TAG, "fini callback...");
        if (ESP_OK != (res = g_osh_fsm->fini_callback(g_osh_fsm->conf_arg))) return res;
    }

    // clear list
    if (!listLIST_IS_EMPTY(&g_osh_fsm->states_list)) {
        // not empty list
        osh_node_fsm_state_t *fsm_state = NULL;
        listFOR_EACH_ENTRY(&g_osh_fsm->states_list, osh_node_fsm_state_t, fsm_state) {
            if (!listLIST_IS_EMPTY(&fsm_state->events_list)) {
                // not empty list
                osh_node_fsm_event_t *fsm_event = NULL;
                listFOR_EACH_ENTRY(&fsm_state->events_list, osh_node_fsm_event_t, fsm_event) {
                    ESP_LOGI(FSM_TAG, "free state %d event %d",
                            (int)fsm_state->state, (int)fsm_event->bits);
                    uxListRemove(&fsm_event->event_item);   // remove from list
                    free(fsm_event);    // free mem
                }
            }
            ESP_LOGI(FSM_TAG, "free state %d", (int)fsm_state->state);
            uxListRemove(&fsm_state->state_item);   // remove from list
            free(fsm_state);
        }
    }

    ESP_LOGI(FSM_TAG, "free FSM");
    if (NULL != g_osh_fsm->event_group) vEventGroupDelete(g_osh_fsm->event_group);
    free(g_osh_fsm);
    g_osh_fsm = NULL;

    return ESP_OK;
}

/* invoke event */
esp_err_t osh_node_fsm_invoke_event(const EventBits_t uxBitsToSet)
{
    if (NULL == g_osh_fsm) return OSH_ERR_FSM_NOT_INIT;
    xEventGroupSetBits(g_osh_fsm->event_group, uxBitsToSet);
    return ESP_OK;
}

esp_err_t osh_node_fsm_invoke_event_from_ISR(const EventBits_t uxBitsToSet)
{
    if (NULL == g_osh_fsm) return OSH_ERR_FSM_NOT_INIT;
    xEventGroupSetBits(g_osh_fsm->event_group, uxBitsToSet);

    // xHigherPriorityTaskWoken must be initialised to pdFALSE.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Set bit 0 and bit 4 in xEventGroup.
     BaseType_t xResult = xEventGroupSetBitsFromISR(
                                g_osh_fsm->event_group,
                                uxBitsToSet,
                                &xHigherPriorityTaskWoken);

    // Was the message posted successfully?
    if (xResult == pdPASS) {
        // If xHigherPriorityTaskWoken is now set to pdTRUE then a context
        // switch should be requested.  The macro used is port specific and
        // will be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() -
        // refer to the documentation page for the port being used.
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
     }

    return ESP_OK;
}

/* get state */
OSH_FSM_STATES_ENUM osh_node_fsm_get_state(void) {
    return (NULL == g_osh_fsm) ? OSH_FSM_STATE_BUTT : g_osh_fsm->current_state;
}

/* set state */
esp_err_t osh_node_fsm_set_state(OSH_FSM_STATES_ENUM state) {
    if (OSH_FSM_STATE_BUTT <= state) {
        ESP_LOGE(FSM_TAG, "invalid state %d", state);
        return OSH_ERR_FSM_INNER;
    } else {
        g_osh_fsm->current_state = state;
        return ESP_OK;
    }
}
