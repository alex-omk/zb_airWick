#include "leds_status.h"

#include "driver/gpio.h"
#include "common.h"
#include "esp_zigbee.h"

static const char *TAG = "ESP_ZB_LED";

TaskHandle_t ledTaskHandle = NULL;

#ifdef WS2812
  #include "led_strip.h"
  static led_strip_handle_t led_strip;
#endif

uint8_t s_led_state = 0;

static void ledON(void){
  #ifdef WS2812
    led_strip_set_pixel(led_strip, 0, 255, 0, 0);
    led_strip_refresh(led_strip);   /* Refresh the strip to send data */
  #else
    gpio_set_level(LED_PIN, LED_ON);
  #endif
}

static void ledOFF(void){
  #ifdef WS2812
    led_strip_clear(led_strip);  /* Set all LED off to clear all pixels */
  #else
    gpio_set_level(LED_PIN, LED_OFF);
  #endif
}

void blink_led(void){
  if (s_led_state) {
    ledON();
  } else {
    ledOFF();
  }
}

void led_task(void *pvParameters){
  ESP_LOGI(TAG, "Start LED Task");
  while (1) {
    if (!ZbGetStatus()){
      s_led_state = !s_led_state;
      blink_led();
    } else{
      ESP_LOGI(TAG, "LED OFF");
      if(s_led_state == 1) ledOFF();
      vTaskDelete(NULL);
    }
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void configure_led(void){
  #ifdef WS2812
    ESP_LOGI(TAG, "Configured to blink addressable LED!");
    led_strip_config_t strip_config = {
      .led_model = LED_MODEL_WS2812,
      .strip_gpio_num = LED_PIN,
      .max_leds = 1, // at least one LED on board
    };
    led_strip_spi_config_t spi_config = {
      .clk_src = SPI_CLK_SRC_DEFAULT,
      .spi_bus = SPI2_HOST,
      .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));

    /*clear pixels */
    led_strip_clear(led_strip);
  #else
    ESP_LOGI (TAG, "Initialize the LED IO");
    gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << LED_PIN),
      .pull_down_en =  GPIO_PULLDOWN_DISABLE,
      .pull_up_en =  GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, LED_OFF);
  #endif
}