/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>


#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueButId;

SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_y;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        int btn_id = gpio;
        xQueueSendFromISR(xQueueButId, &btn_id, 0);
    } 
}

void btn_task(void* p) {
    int btn_id;
    while (true) {
        xQueueReceive(xQueueButId, &btn_id, portMAX_DELAY);

        if (btn_id == BTN_PIN_R) {
            xSemaphoreGive(xSemaphore_r);
        } else if (btn_id == BTN_PIN_Y) {
            xSemaphoreGive(xSemaphore_y);
        }
    }
}

void led_r_task(void *p){
    bool blinking = false;
    while (true) {
        if (!blinking) {
            xSemaphoreTake(xSemaphore_r, portMAX_DELAY);
            blinking = true;
        } else {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));

            if (xSemaphoreTake(xSemaphore_r, 0)){
                blinking = false;
                gpio_put(LED_PIN_R, 0);
            }
        }
    }
}

void led_y_task(void *p){
    bool blinking = false;
    while (true) {
        if (!blinking) {
            xSemaphoreTake(xSemaphore_y, portMAX_DELAY);
            blinking = true;
        } else {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));

            if (xSemaphoreTake(xSemaphore_y, 0)){
                blinking = false;
                gpio_put(LED_PIN_Y, 0);
            }
        }
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN_R);gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_init(LED_PIN_Y);gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_Y); gpio_set_dir(BTN_PIN_Y, GPIO_IN); gpio_pull_up(BTN_PIN_Y);                                                            
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true); 

    xQueueButId = xQueueCreate(32, sizeof(int));
    xTaskCreate(btn_task, "BTN_Task 1", 256, NULL, 1, NULL);
    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_y = xSemaphoreCreateBinary();

    xTaskCreate(led_y_task, "LED_Y_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_r_task, "LED_R_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}