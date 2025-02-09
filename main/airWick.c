#include "airWick.h"

#include "common.h"
#include "driver/gpio.h"

static const char *TAG = "ZB_AirWick";

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
}

void airWickMotorUP(){
  gpio_set_level(MOTOR_UP_PIN, PIN_HIGH);
  vTaskDelay(pdMS_TO_TICKS(80));
  gpio_set_level(MOTOR_UP_PIN, PIN_LOW);
}

void airWickMotorDown(){
  gpio_set_level(MOTOR_DOWN_PIN, PIN_HIGH);
  vTaskDelay(pdMS_TO_TICKS(150));
  gpio_set_level(MOTOR_DOWN_PIN, PIN_LOW);
}

void airWickSpray(){
  ESP_LOGW(TAG, "Spray");
  airWickMotorDown();
  vTaskDelay(pdMS_TO_TICKS(50));
  airWickMotorUP();
}