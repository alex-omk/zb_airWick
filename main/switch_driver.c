/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Zigbee switch driver example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "switch_driver.h"
#include "esp_timer.h"

/**
 * @brief:
 * This example code shows how to configure light switch with attribute as well as button switch handler.
 *
 * @note:
   Currently only support toggle switch functionality is available
 *
 * @note:
 * For other possible switch functions (on/off,level up/down,step up/down). User need to implement and create them by themselves
 */

static QueueHandle_t gpio_evt_queue = NULL;
/* button function pair, should be defined in switch example source file */
static switch_func_pair_t *switch_func_pair;
/* call back function pointer */
static esp_switch_callback_t func_ptr;
/* which button is pressed */
static uint8_t switch_num;
static const char *TAG = "ESP_ZB_SWITCH";

static void switch_driver_gpios_intr_enabled(bool enabled);

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    switch_driver_gpios_intr_enabled(false);
    xQueueSendFromISR(gpio_evt_queue, (switch_func_pair_t *)arg, NULL);
}

/**
 * @brief Enable GPIO (switches refer to) isr
 *
 * @param enabled      enable isr if true.
 */
static void switch_driver_gpios_intr_enabled(bool enabled)
{
    for (int i = 0; i < switch_num; ++i) {
        if (enabled) {
            gpio_intr_enable((switch_func_pair + i)->pin);
        } else {
            gpio_intr_disable((switch_func_pair + i)->pin);
        }
    }
}

/**
 * @brief Tasks for checking the button event and debounce the switch state
 *
 * @param arg      Unused value.
 */
static void switch_driver_button_detected(void *arg)
{
    gpio_num_t io_num = GPIO_NUM_NC;
    switch_func_pair_t button_func_pair;
    static switch_state_t switch_state = SWITCH_IDLE;
    bool evt_flag = false;
    int64_t start_evt_time = 0;


    for (;;) {
        /* check if there is any queue received, if yes read out the button_func_pair */
        if (xQueueReceive(gpio_evt_queue, &button_func_pair, portMAX_DELAY)) {
            io_num =  button_func_pair.pin;
            switch_driver_gpios_intr_enabled(false);
            evt_flag = true;
            start_evt_time = esp_timer_get_time();
            ESP_LOGW(TAG, "EVT START TIME!!!!");
        }
        while (evt_flag) {
            bool value = gpio_get_level(io_num);
            switch (switch_state) {
            case SWITCH_IDLE:
                switch_state = (value == GPIO_INPUT_LEVEL_ON) ? SWITCH_PRESS_DETECTED : SWITCH_IDLE;
                ESP_LOGW(TAG, "THIS IDLE");
                break;
            case SWITCH_PRESS_DETECTED:
                switch_state = (value == GPIO_INPUT_LEVEL_ON) ? SWITCH_PRESS_DETECTED : SWITCH_RELEASE_DETECTED;
                // ESP_LOGW(TAG, "THIS PRESS_DETECTED");
                break;
            case SWITCH_RELEASE_DETECTED:
                if (((esp_timer_get_time() - start_evt_time) / 1000ULL) > 1000){
                    (*func_ptr)(BTN_LONG_PRESS);
                } else {
                    (*func_ptr)(BTN_SINGLE_CLICK);
                }
                // ESP_LOGW(TAG, "THIS SWITCH_RELEASE_DETECTED, go to CB");
                switch_state = SWITCH_IDLE;
                start_evt_time = 0;
                
                break;
            default:
                break;
            }
            if (switch_state == SWITCH_IDLE) {
                switch_driver_gpios_intr_enabled(true);
                evt_flag = false;
                break;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

/**
 * @brief init GPIO configuration as well as isr
 *
 * @param button_func_pair      pointer of the button pair.
 * @param button_num            number of button pair.
 */
static bool switch_driver_gpio_init(switch_func_pair_t *button_func_pair, uint8_t button_num)
{
    gpio_config_t io_conf = {};
    switch_func_pair = button_func_pair;
    switch_num = button_num;
    uint64_t pin_bit_mask = 0;

    /* set up button func pair pin mask */
    for (int i = 0; i < button_num; ++i) {
        pin_bit_mask |= (1ULL << (button_func_pair + i)->pin);
    }
    /* interrupt of falling edge */
    io_conf.intr_type = GPIO_INTR_LOW_LEVEL;
    io_conf.pin_bit_mask = pin_bit_mask;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    /* configure GPIO with the given settings */
    gpio_config(&io_conf);
    /* create a queue to handle gpio event from isr */
    gpio_evt_queue = xQueueCreate(10, sizeof(switch_func_pair_t));
    if ( gpio_evt_queue == 0) {
        ESP_LOGE(TAG, "Queue was not created and must not be used");
        return false;
    }
    /* start gpio task */
    xTaskCreate(switch_driver_button_detected, "button_detected", 4096, NULL, 10, NULL);
    /* install gpio isr service */
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    for (int i = 0; i < button_num; ++i) {
        gpio_isr_handler_add((button_func_pair + i)->pin, gpio_isr_handler, (void *) (button_func_pair + i));
    }
    return true;
}

bool switch_driver_init(switch_func_pair_t *button_func_pair, uint8_t button_num, esp_switch_callback_t cb)
{
    if (!switch_driver_gpio_init(button_func_pair, button_num)) {
        return false;
    }
    func_ptr = cb;
    return true;
}
