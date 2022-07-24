/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ds18b20.h"
#include "driver/gpio.h"


#define TEMP_BUS 3
#define TEMP_LB 15
#define TEMP_UB 40

DeviceAddress tempSensors[2];

static TaskHandle_t water_handle = NULL;
static TaskHandle_t temperature_handle = NULL;

int pulso=0;
int pump_state=0;

static void gerar_pulso(void* arg) {
    while(1) {
        pulso = !pulso;
//        printf("PULSO %d!\n", pulso);
        gpio_set_level(GPIO_NUM_2, pulso);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void detect_water_level(void *param) {
    while(1) {
        printf("Pino 4(SENSOR) = %d\n", gpio_get_level(GPIO_NUM_4));
        printf("Pino 5(BOMBA) = %d\n", pump_state);
        if(!gpio_get_level(GPIO_NUM_4) && !pump_state){
            printf("Nivel de água Baixo.\n");
            pump_state = 1;
            gpio_set_level(GPIO_NUM_5, pump_state);
            printf("Bomba de água ligada.\n");
        } else if(gpio_get_level(GPIO_NUM_4) && pump_state) {
            pump_state = 0;
            gpio_set_level(GPIO_NUM_5, pump_state);
            printf("Bomba de água desligada.\n");
        }
        vTaskDelay(2500 / portTICK_PERIOD_MS); }
}

void detect_temperature(void *param){
    while(1){
        float cTemp = ds18b20_get_temp();
        printf("Temperature: %0.1f C\n", cTemp);

        if (cTemp >= TEMP_LB && cTemp <= TEMP_UB){
            printf("Temperatura de água normal.\n");
            gpio_set_level(GPIO_NUM_7, 0);
            gpio_set_level(GPIO_NUM_8, 0);
        }else if(cTemp < TEMP_LB){
            printf("Temperatura de água baixa.\n");
            gpio_set_level(GPIO_NUM_7, 1);
            gpio_set_level(GPIO_NUM_8, 0);
        }else{
            printf("Temperatura de água alta.\n");
            gpio_set_level(GPIO_NUM_7, 0);
            gpio_set_level(GPIO_NUM_8, 1);
        }

        vTaskDelay(2500/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{

    gpio_config_t io_conf = {};

    // Pulso teste
    io_conf.intr_type = GPIO_MODE_OUTPUT;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << 2;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    // SAIDA P/AGUA, BOMBA/LED
    io_conf.pin_bit_mask = 1ULL << 5;
    gpio_config(&io_conf);

    // SAIDA P/Checagem TEMP ABAIXO, LED
    io_conf.pin_bit_mask = 1ULL << 7;
    gpio_config(&io_conf);

    // SAIDA P/Checagem TEMP ACIMA, LED
    io_conf.pin_bit_mask = 1ULL << 8;
    gpio_config(&io_conf);

    // SENSOR DE NIVEL DE AGUA
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << 4;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_set_level(GPIO_NUM_4, 0);
    gpio_set_level(GPIO_NUM_5, 0);
    gpio_set_level(GPIO_NUM_7, 0);
    gpio_set_level(GPIO_NUM_8, 0);

    // CONFIGS DO SENSOR DE TEMPERATURA
    ds18b20_init(TEMP_BUS);
    ds18b20_setResolution(tempSensors, 2, 10);

    xTaskCreate(gerar_pulso, "pulso", 2048, NULL, 1, NULL);
    xTaskCreate(detect_water_level, "Detect water level", 2048, NULL, 1, &water_handle);
    xTaskCreate(detect_temperature, "Detect temperature", 2048, NULL, 1, &temperature_handle);

}
