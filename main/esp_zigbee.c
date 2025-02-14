#include "esp_zigbee.h"

#include "ha/esp_zigbee_ha_standard.h"
#include "zcl/esp_zigbee_zcl_command.h"
#include "zcl/esp_zigbee_zcl_power_config.h"
#include "zb_ota.h"
#include "zb_util.h"

#include "common.h"
#include "tools.h"
#include "leds_status.h"
#include "airWick.h"

#ifdef USE_BATTERY_MOD
#include "battery.h"
#endif

static const char *TAG = "ESP_ZB";

bool zb_connected = false;

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask) {
  ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {

  uint32_t *p_sg_p = signal_struct->p_app_signal;
  esp_err_t err_status = signal_struct->esp_err_status;
  esp_zb_app_signal_type_t sig_type = *p_sg_p;
  esp_zb_zdo_signal_leave_params_t *leave_params = NULL;

  switch (sig_type) {
  case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
    ESP_LOGI(TAG, "Initialize Zigbee stack");
    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
    break;
  case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
  case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
    if (err_status == ESP_OK) {
      // ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
      ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
      if (esp_zb_bdb_is_factory_new()) {
        ESP_LOGI(TAG, "Start network steering");
        zb_connected = false;

        if (ledTaskHandle != NULL){
          vTaskDelete(ledTaskHandle);
          ledTaskHandle = NULL;
        }
        xTaskCreate(led_task, "led_task", 4096, NULL, 3, &ledTaskHandle);

        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
      } else {
        ESP_LOGI(TAG, "Device rebooted");
        zb_connected = true;
      }
    } else {
      /* commissioning failed */
      ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s), restart device", esp_err_to_name(err_status));
      // esp_restart();
    }
    break;
  case ESP_ZB_BDB_SIGNAL_STEERING:
    if (err_status == ESP_OK) {
      zb_connected = true;
      // if (ledTaskHandle != NULL){
      //   vTaskDelete(ledTaskHandle);
      //   ledTaskHandle = NULL;
      // }
      esp_zb_ieee_addr_t extended_pan_id;
      esp_zb_get_extended_pan_id(extended_pan_id);
      ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
               extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
               extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
               esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
    } else {
      ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
      esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
      zb_connected = false;
      if (ledTaskHandle == NULL){
        xTaskCreate(led_task, "led_task", 4096, NULL, 3, &ledTaskHandle);
      }
    }
    break;
  case ESP_ZB_ZDO_SIGNAL_LEAVE:
    leave_params = (esp_zb_zdo_signal_leave_params_t *)esp_zb_app_signal_get_params(p_sg_p);
    if (leave_params->leave_type == ESP_ZB_NWK_LEAVE_TYPE_RESET) {
      ESP_LOGI(TAG, "Reset device");
      esp_zb_factory_reset();
    }
    break;
  case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
    ESP_LOGI(TAG, "Zigbee can sleep");
#ifdef USE_BATTERY_MOD
    // if( (millis() - last_battery_measurement_time) > 30000){
    if( (millis() - last_battery_measurement_time) > MINUTES_TO_MS(READ_BATT_INTERVAL)){
      batteryUpdate();
    }
    if(autoSpray && (millis() - last_spray_time) > MINUTES_TO_MS(spray_interval)){
      last_spray_time = millis();
      airWickSpray();
    }
#endif
    esp_zb_sleep_now();
    break;
  default:
    ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type, esp_err_to_name(err_status));
    break;
  }
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message) {

  esp_err_t ret = ESP_OK;
  bool state = 0;

  ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
  ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                      message->info.status);
  ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
           message->attribute.id, message->attribute.data.size);

  if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
    if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
      state = *(bool *)message->attribute.data.value;
    }
  }

  if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ANALOG_OUTPUT && message->attribute.id == ESP_ZB_ZCL_ATTR_ANALOG_OUTPUT_PRESENT_VALUE_ID){
    int new_interval = *(float *)message->attribute.data.value;
    
    ESP_LOGW(TAG,"NEW spray interval %d", new_interval);
    if (new_interval == 0){
      autoSpray = false;
    }
    spray_interval = new_interval;
    airWickWriteInterval();
    // esp_zb_zcl_set_attribute_val(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_OUTPUT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_ANALOG_OUTPUT_PRESENT_VALUE_ID, &new_val, false);
  }

  return ret;
}

static esp_err_t zb_privileged_cmd_handler(const esp_zb_zcl_privilege_command_message_t *message) {
  // Check if cluster and command correspond to "ON_OFF TOGGLE" command
  if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF && message->info.command.id == ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID) {
    ESP_LOGI(TAG, "Spray command received");
    airWickSpray();
  } else {
    ESP_LOGE(TAG, "bad command in zb_privileged_cmd_handler");
  }
  return ESP_OK;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message) {

  esp_err_t ret = ESP_OK;
  switch (callback_id) {
  case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
    ESP_LOGW(TAG, "Receive Zigbee attr action(0x%x) callback", callback_id);
    ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
    break;
  case ESP_ZB_CORE_OTA_UPGRADE_VALUE_CB_ID:
    ret = zb_ota_upgrade_status_handler(*(esp_zb_zcl_ota_upgrade_value_message_t *)message);
    break;
  case ESP_ZB_CORE_OTA_UPGRADE_QUERY_IMAGE_RESP_CB_ID:
    ret = zb_ota_upgrade_query_image_resp_handler(*(esp_zb_zcl_ota_upgrade_query_image_resp_message_t *)message);
    break;
  case ESP_ZB_CORE_CMD_PRIVILEGE_COMMAND_REQ_CB_ID:
    ESP_LOGW(TAG, "Receive Zigbee PRIVILEGE_COMMAND_REQ (0x%x) callback", ESP_ZB_CORE_CMD_PRIVILEGE_COMMAND_REQ_CB_ID);
    ret = zb_privileged_cmd_handler(message);
    break;
  case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID:
    ESP_LOGW(TAG, "Receive Zigbee default response(0x%x) callback", callback_id);
    break;
  case ESP_ZB_CORE_BASIC_RESET_TO_FACTORY_RESET_CB_ID:
    ESP_LOGW(TAG, "Receive Zigbee Reset comand");
    airWickClearCounter();
    float n_count = (float)spray_counter;
    update_attribute_value(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &n_count, "spray counter");
    break;
  case ESP_ZB_CORE_CMD_WRITE_ATTR_RESP_CB_ID:
    ESP_LOGW(TAG, "Receive Zigbee WRITE_ATTR_RESP_CB_ID");
    break;
  default:
    ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
    break;
  }
  return ret;
}

static void esp_zb_task(void *pvParameters) {
  static char manufacturer[16], model[16], firmware_version[16], firmware_date[16];
  /* initialize Zigbee stack */
  esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();

  esp_zb_sleep_enable(true);

  esp_zb_init(&zb_nwk_cfg);

  esp_zb_sleep_set_threshold(200);
  
  set_zcl_string(manufacturer, MANUFACTURER_NAME);
  set_zcl_string(model, MODEL_NAME);
  set_zcl_string(firmware_date, FW_BUILD_DATE);

  char fw_version[10];
  sprintf(fw_version, "%d", FIRMWARE_VERSION);
  set_zcl_string(firmware_version, fw_version);

  zcl_basic_manufacturer_info_t info = {
      .manufacturer_name = manufacturer,
      .model_identifier = model,
      .firmware_version = firmware_version,
      .firmware_date = firmware_date,
      .power_source = ESP_ZB_POWER_SOURCE_DC,
  };

  esp_zb_endpoint_config_t endpoint_config = {
      .endpoint = HA_ENDPOINT,
      .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
      .app_device_id = ESP_ZB_HA_ON_OFF_LIGHT_DEVICE_ID,
      .app_device_version = 0,
  };

  esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

  ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, esp_zb_create_basic_cluster(&info), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_create_identify_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_ota_cluster(esp_zb_cluster_list, esp_zb_create_ota_cluster(), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list, esp_zb_create_on_off_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

  // ESP_ERROR_CHECK(esp_zb_cluster_list_add_analog_value_cluster(esp_zb_cluster_list,e sp_zb_create_analog_value(3254), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  
  /* this cluster is used to send the value of the number of sprays. */
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_analog_input_cluster(esp_zb_cluster_list, esp_zb_create_analog_input(spray_counter), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  
  /* This cluster is used to adjust the spray interval. */
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_analog_output_cluster(esp_zb_cluster_list, esp_zb_create_analog_output(spray_interval), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
  
  // ESP_ERROR_CHECK(esp_zb_cluster_list_add_diagnostics_cluster(esp_zb_cluster_list, esp_zb_create_diagnostics_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));


  /* Separate endpoint and cluster for sending battery voltage */
  esp_zb_cluster_list_t *esp_zb_cluster_list2 = esp_zb_zcl_cluster_list_create();
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_analog_input_cluster(esp_zb_cluster_list2, esp_zb_create_analog_input(6565), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

#ifdef USE_BATTERY_MOD
  ESP_ERROR_CHECK(esp_zb_cluster_list_add_power_config_cluster(esp_zb_cluster_list, esp_zb_create_power_cfg_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
#endif

  ESP_ERROR_CHECK(esp_zb_zcl_add_privilege_command(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID));


  esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();                  // Create endpoint list
  esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list, endpoint_config); // Added cluster list to ep_list

  /* EP 10    */
  endpoint_config.endpoint = HA_ENDPOINT + 10;
  esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list2, endpoint_config);


  esp_zb_device_register(esp_zb_ep_list);
  esp_zb_core_action_handler_register(zb_action_handler);
  esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);

  ESP_ERROR_CHECK(esp_zb_start(false));
  esp_zb_stack_main_loop();

}

bool ZbGetStatus(void) {
  return zb_connected;
}

void ZbSetup(void) {

  esp_zb_platform_config_t config = {
      .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
      .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
  };

  
  ESP_ERROR_CHECK(esp_zb_platform_config(&config));

  esp_zb_set_default_long_poll_interval(7500);

  uint32_t interval_poll = esp_zb_get_default_long_poll_interval();

  ESP_LOGW(TAG, "Long_poll_interval: %d", (int)interval_poll);


  xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);

}