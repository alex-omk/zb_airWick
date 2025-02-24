
#include "tools.h"
#include "freertos/FreeRTOS.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "string.h"
#include <time.h>
#include "common.h"

static const char *TAG = "ESP_ZB_TOOLS";


uint16_t restart_counter = 0; // value will default to 0, if not set yet in NVS

int64_t IRAM_ATTR millis(){
  return (esp_timer_get_time() / 1000ULL);
}



void setup_NVS() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  nvs_handle_t my_handle;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  ESP_LOGI(__func__, "Opening Non-Volatile Storage (NVS) handle... %s", (err != ESP_OK) ? T_STATUS_FAILED : T_STATUS_DONE);
  if (err == ESP_OK) {

    // Read
    
    err = nvs_get_u16(my_handle, "restart_counter", &restart_counter);
    ESP_LOGI(__func__, "Reading restart counter from NVS ... %s", (err != ESP_OK) ? T_STATUS_FAILED : T_STATUS_DONE);
    switch (err) {
    case ESP_OK:
      ESP_LOGI(__func__, "Restart counter = %" PRIu16, restart_counter);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGI(__func__, "The value is not initialized yet!");
      break;
    default:
      ESP_LOGI(__func__, "Error (%s) reading!", esp_err_to_name(err));
    }

    // Write
    restart_counter++;
    err = nvs_set_u16(my_handle, "restart_counter", restart_counter);
    ESP_LOGI(__func__, "Updating restart counter in NVS ... %s", (err != ESP_OK) ? T_STATUS_FAILED : T_STATUS_DONE);

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    ESP_LOGI(__func__, "Committing updates in NVS ... %s", (err != ESP_OK) ? T_STATUS_FAILED : T_STATUS_DONE);

    // Close
    nvs_close(my_handle);
  }
}

bool check_NVS_key(const char *nvs_key){
  nvs_handle_t my_handle;
  int32_t value = 0;
  esp_err_t err;

  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  // if(err == ESP_OK){ESP_LOGW(TAG, "NVS_OPEN ... OK");}
  err = nvs_get_i32(my_handle, nvs_key, &value);
  nvs_close(my_handle);

  if(err == ESP_ERR_NVS_NOT_FOUND){ESP_LOGE(__func__, "The value %s is not initialized yet!", nvs_key);}
  if(err == ESP_OK){
    return true;
  }
  
  return false;
  // nvs_handle_t my_handle;
  // int32_t value = 0;
  // esp_err_t err;

  // err = nvs_open("storage", NVS_READWRITE, &my_handle);
  // err = nvs_find_key(&my_handle, nvs_key, NVS_TYPE_ANY);
  // if(err == ESP_OK){
  //   return true;
  // }
  
  // return false;
}

int32_t read_NVS(const char *nvs_key) {
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

  // Read

  int32_t value = 0;
  err = nvs_get_i32(my_handle, nvs_key, &value);
  switch (err) {
  case ESP_OK:
    ESP_LOGI(__func__, "%s is %ld ", nvs_key, value);
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGE(__func__, "The value is not initialized yet!");

    // char *substring = "_led_mode";
    // if (strstr(nvs_key, substring) != NULL) {
    //   value = 1;
    // }
    err = nvs_set_i32(my_handle, nvs_key, value);
    ESP_LOGW(__func__, "Updating %s in NVS ... %s", nvs_key, (err != ESP_OK) ? T_STATUS_FAILED : T_STATUS_DONE);
    break;
  default:
    ESP_LOGE(__func__, "Error (%s) reading!", esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  if (err != ESP_OK) {
    return false;
  }
  return value;
}

bool write_NVS(const char *nvs_key, int value) {
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  err = nvs_set_i32(my_handle, nvs_key, value);
  ESP_LOGI(__func__, "Write value... %s", (err != ESP_OK) ? T_STATUS_FAILED : T_STATUS_DONE);

  // Commit written value.
  // After setting any values, nvs_commit() must be called to ensure changes are written
  // to flash storage. Implementations may write to storage at other times,
  // but this is not guaranteed.
  err = nvs_commit(my_handle);
  ESP_LOGI(__func__, "Commit updates... %s", (err != ESP_OK) ? T_STATUS_FAILED : T_STATUS_DONE);

  // Close
  nvs_close(my_handle);

  if (err != ESP_OK) {
    return ESP_FAIL;
  }
  return ESP_OK;
}

void print_chip_info() {
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  
  esp_chip_info(&chip_info);

  ESP_LOGW(__func__, "This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  ESP_LOGW(__func__, "Silicon revision v%d.%d", major_rev, minor_rev);

  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
    ESP_LOGE(__func__, "Get flash size failed");
    return;
  }

  ESP_LOGW(__func__, "%" PRIu32 "MB %s flash",
           flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  ESP_LOGW(__func__, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
}

void heap_stats() {
  multi_heap_info_t heap_info;
  heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);

  size_t total_heap_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  size_t free_heap_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  size_t minimum_free_size = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);

  float frag_heap_percentage = 0.0;
  if (free_heap_size > 0) {
    frag_heap_percentage = (1.0 - ((float)largest_free_block / (float)free_heap_size)) * 100.0;
  }

  float free_heap_percentage = ((float)free_heap_size / (float)total_heap_size) * 100.0;

  ESP_LOGI(__func__, "total: %d, free: %d, largest free block: %d, minimum free size: %d",
           total_heap_size,
           free_heap_size,
           largest_free_block,
           minimum_free_size);

  ESP_LOGW(__func__, "free: %.2f%% (%d bytes), fragmentation: %.2f%%",
           free_heap_percentage,
           free_heap_size,
           frag_heap_percentage);
}

void sys_stats_task(void *pvParameters) {
  while (1) {
    heap_stats();
    vTaskDelay(pdMS_TO_TICKS(SYS_STATS_INTERVAL_TASK));
  }
}