#include "main.h"
#include "common.h"
#include "esp_zigbee.h"
#include "leds_status.h"
#include "switch_driver.h"

#include "tools.h"

static const char *TAG = "ESP_ZB_MAIN";

#ifdef USE_BATTERY_MOD
#include "battery.h"
#endif

static switch_func_pair_t button_func_pair[] = {
    {BTN_PIN, BTN_EVT_CONTROL}
};


static void zb_buttons_handler(button_event_t evt) {
  if (evt == BTN_SINGLE_CLICK) {
    ESP_LOGI(__func__, "Single_click");
  }
  if (evt == BTN_LONG_PRESS) {
    ESP_LOGI(__func__, "Long press, leave & reset");
    esp_zb_factory_reset();
  }
}

static esp_err_t esp_zb_power_save_init(void) {
  esp_err_t rc = ESP_OK;
#ifdef CONFIG_PM_ENABLE
  int cur_cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
  esp_pm_config_t pm_config = {
      .max_freq_mhz = cur_cpu_freq_mhz,
      .min_freq_mhz = cur_cpu_freq_mhz,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
      .light_sleep_enable = true
#endif
  };
  rc = esp_pm_configure(&pm_config);
#endif
  return rc;
}



void app_main(void) {

  ESP_LOGW(TAG, "Device model %s, FW verison 0x%x (%d) date: %s", ModuleName, FIRMWARE_VERSION, FIRMWARE_VERSION, FW_BUILD_DATE);

  bool status = switch_driver_init(button_func_pair, PAIR_SIZE(button_func_pair), zb_buttons_handler);

  ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << BTN_PIN, ESP_EXT1_WAKEUP_ANY_LOW));

#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
  rtc_gpio_pulldown_dis(BTN_PIN);
  rtc_gpio_pullup_en(BTN_PIN);
#else
  gpio_pulldown_dis(BTN_PIN);
  gpio_pullup_en(BTN_PIN);
#endif


  setup_NVS();
  print_chip_info();
  configure_led();

  ESP_ERROR_CHECK(esp_zb_power_save_init());
  ZbSetup();

  // xTaskCreate(led_task, "Led_status_task", 4096, NULL, 3, NULL);

  // #ifndef USE_SLEEP_MODE
  //   xTaskCreate(sys_stats_task, "Sys_stats_task", 4096, NULL, 3, NULL);
  // #endif
}