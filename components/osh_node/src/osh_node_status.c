/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2024-04-29 23:47:47
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2024-05-09 21:04:24
 * @FilePath    : /OpenSmartHome/components/osh_node/src/osh_node_status.c
 * @Description :
 * Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
 */

#include "driver/gpio.h"
#include "iot_button.h"

#include "osh_node_status.h"
#include "osh_node_fsm.h"
#include "osh_node_network.h"

static const uint32_t status_rgb[OSH_FSM_STATE_BUTT + 1] = {
    // OSH_FSM_STATE_INIT, yellow
    (1UL << 16) | (1UL << 8),
    // OSH_FSM_STATE_IDLE, blue
    1UL,
    // OSH_FSM_STATE_ONGOING, green
    (1UL << 8),
    // OSH_FSM_STATE_SAVING,
    (1UL << 8) | 1,
    // OSH_FSM_STATE_UPGRADING
    (1UL << 16) | 1,
    // OSH_FSM_STATE_BUTT, read
    (1UL << 16)
};

// on/off flag
static BaseType_t led_flag = pdFALSE;
// last time
static TickType_t last_time;
// last state
static OSH_FSM_STATES_ENUM last_state = OSH_FSM_STATE_BUTT;

/* init RGB LED for status indicator */
esp_err_t osh_node_status_init(void) {
    gpio_config_t io_config = {
        .intr_type      = GPIO_INTR_DISABLE,
        .mode           = GPIO_MODE_OUTPUT,
        .pin_bit_mask   = (1ULL << CONFIG_STATUS_R_LED_GPIO_NUM) |
                          (1ULL << CONFIG_STATUS_G_LED_GPIO_NUM) |
                          (1ULL << CONFIG_STATUS_B_LED_GPIO_NUM),
        .pull_down_en   = 0,
        .pull_up_en     = 0
    };
    gpio_config(&io_config);
    last_time = xTaskGetTickCount();
    return ESP_OK;
}

/* flush status */
esp_err_t osh_node_status_refresh(void) {
    // get state
    OSH_FSM_STATES_ENUM state = osh_node_fsm_get_state();
    state = (OSH_FSM_STATE_BUTT < state) ? OSH_FSM_STATE_BUTT : state;

    TickType_t this_time = xTaskGetTickCount();

    if (last_state != state) {
        // update
        last_time = this_time;
        last_state = state;
        led_flag = pdTRUE;

    }else if (((this_time - last_time) / (float)configTICK_RATE_HZ) > 2.0f) {
        // reverse and update last time
        led_flag =  !led_flag;
        last_time = this_time;
    }

    if (led_flag) {
        // set RGB
        gpio_set_level(CONFIG_STATUS_R_LED_GPIO_NUM,
                    (status_rgb[state] & 0xFF0000) >> 16);
        gpio_set_level(CONFIG_STATUS_G_LED_GPIO_NUM,
                    (status_rgb[state] & 0x00FF00) >> 8);
        gpio_set_level(CONFIG_STATUS_B_LED_GPIO_NUM,
                    (status_rgb[state] & 0xFF));
    } else {
        // turn off
        gpio_set_level(CONFIG_STATUS_R_LED_GPIO_NUM, 0);
        gpio_set_level(CONFIG_STATUS_G_LED_GPIO_NUM, 0);
        gpio_set_level(CONFIG_STATUS_B_LED_GPIO_NUM, 0);
    }
    return ESP_OK;
}

// callback for reset
static void button_press_reset(void *arg, void *data)
{
    osh_node_network_reset();
    esp_restart();
}

// callback for restart
static void button_press_restart(void *arg, void *data)
{
    esp_restart();
}

/* init GPIO button for reset network config */
esp_err_t osh_node_reset_btn_init(void) {
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = CONFIG_NODE_RESET_GPIO_NUM,
            .active_level = 0,
        },
    };
    button_handle_t btn_handle = iot_button_create(&btn_cfg);
    if (btn_handle) {
        ESP_ERROR_CHECK(iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_UP,
                                               button_press_reset, NULL));
        ESP_ERROR_CHECK(iot_button_register_cb(btn_handle, BUTTON_PRESS_UP,
                                               button_press_restart, NULL));
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}
