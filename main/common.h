#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include <inttypes.h>
#include <sdkconfig.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_PM_ENABLE
#include "esp_pm.h"
#include "esp_private/esp_clk.h"
#include "esp_sleep.h"
#endif

static const char *ModuleName = {"Air Wick"};

#define HA_ENDPOINT 1


#define MANUFACTURER_NAME "OMK"
#define MODEL_NAME "esp32AirWick"
#define OTA_UPGRADE_MANUFACTURER 2810 /* The attribute indicates the file version of the downloaded image on the device*/
#define OTA_UPGRADE_IMAGE_TYPE 4113

#define FIRMWARE_VERSION 0x0000006C
#define FW_BUILD_DATE "20250210"

#define AIR_WICK_CUSTOM_CLUSTER 0xFFF2
#define SPRAY_COUNTER_ATTR_ID 0x00

#define USE_BATTERY_MOD
#define READ_BATT_INTERVAL 60 //min

#define MOTOR_UP_PIN 11
#define MOTOR_DOWN_PIN 12

#define PIN_HIGH  1
#define PIN_LOW   0

#define BTN_PIN 9

// #define WS2812

#ifndef WS2812
#define LED_PIN 13
#else
#define LED_PIN 8
#endif

#define LED_ON 1
#define LED_OFF 0



#ifdef __cplusplus
} // extern "C"
#endif
