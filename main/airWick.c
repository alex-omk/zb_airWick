#include "airWick.h"

#include "common.h"
#include "tools.h"
#include "driver/gpio.h"
#include "ha/esp_zigbee_ha_standard.h"

static const char *TAG = "ZB_AirWick";

int32_t spray_counter = 0;

static void airWickWriteCounter (){
  ESP_ERROR_CHECK(write_NVS("spray_counter", spray_counter));
}

static void airWickReadCounter (){
  spray_counter = read_NVS("spray_counter");
}

void airWickSetup(){

  ESP_LOGI (TAG, "Initialize the Motor IO");
  gpio_config_t io_conf = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pull_down_en =  GPIO_PULLDOWN_DISABLE,
    .pull_up_en =  GPIO_PULLDOWN_DISABLE,
    io_conf.pin_bit_mask = (1ULL << MOTOR_UP_PIN )| (1ULL << MOTOR_DOWN_PIN),
  };
  ESP_ERROR_CHECK(gpio_config(&io_conf));

  gpio_set_level(MOTOR_UP_PIN, PIN_LOW);
  gpio_set_level(MOTOR_DOWN_PIN, PIN_LOW);

  airWickReadCounter();
  ESP_LOGW(TAG, "Spray counter = %" PRIu32, spray_counter);
}

void airWickMotorUP(){
  gpio_set_level(MOTOR_UP_PIN, PIN_HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(MOTOR_UP_PIN, PIN_LOW);
}

void airWickMotorDown(){
  gpio_set_level(MOTOR_DOWN_PIN, PIN_HIGH);
  vTaskDelay(pdMS_TO_TICKS(120));
  gpio_set_level(MOTOR_DOWN_PIN, PIN_LOW);
}

void airWickSpray(){
  ESP_LOGW(TAG, "Spray");
  airWickMotorDown();
  vTaskDelay(pdMS_TO_TICKS(5));
  airWickMotorUP();

  spray_counter++;
  airWickWriteCounter();
  ESP_LOGW(TAG, "Spray counter = %" PRIu32, spray_counter);

  float n_count = (float)spray_counter;
  esp_zb_zcl_set_attribute_val(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &n_count, false);
  // esp_zb_zcl_status_t status = esp_zb_zcl_set_attribute_val(HA_ENDPOINT, AIR_WICK_CUSTOM_CLUSTER, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, SPRAY_COUNTER_ATTR_ID, &spray_counter, false);
  // if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
  //   ESP_LOGE(TAG, "Set spray counter attribute value FAIL!");
  // }
}