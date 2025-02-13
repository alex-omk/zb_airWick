#include "airWick.h"

#include "common.h"
#include "tools.h"
#include "driver/gpio.h"
#include "zb_util.h"

static const char *TAG = "ZB_AirWick";

int32_t spray_counter = 0;
int32_t spray_interval = 0;

int64_t last_spray_time = 0;

bool autoSpray = true;


static void airWickWriteCounter (){
  ESP_ERROR_CHECK(write_NVS("spray_counter", spray_counter));
}

static void airWickReadCounter (){
  spray_counter = read_NVS("spray_counter");
}

void airWickWriteInterval(){
  ESP_ERROR_CHECK(write_NVS("spray_interval", spray_interval));
}

void airWickClearCounter(){
  spray_counter = 0;
  ESP_ERROR_CHECK(write_NVS("spray_counter", spray_counter));
}

static void airWickReadInterval(){
  if (check_NVS_key("spray_interval")){
    spray_interval = read_NVS("spray_interval");
    if (spray_interval == 0){
      autoSpray = false;
      ESP_LOGW(TAG, "Auto spray disabled");
    }
  } else {
    ESP_LOGW(TAG, "Set default spray interval: %d minutes", DEFAULT_SPRAY_INTERVAL);
    spray_interval = DEFAULT_SPRAY_INTERVAL;
  }
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

  airWickReadInterval();
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
  update_attribute_value(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &n_count, "spray counter");
  
}