#include "zb_util.h"
#include "esp_check.h"
#include "zcl/esp_zigbee_zcl_power_config.h"
#include "common.h"
#include "tools.h"

#ifdef USE_BATTERY_MOD
#include "battery.h"
#endif

static const char *TAG = "ZCL_UTILITY";

esp_zb_attribute_list_t *measurement_attr_cluster() {

  uint16_t undefined_value = 0x0000;
  uint16_t rms_current_phase_a = ELECTRICAL_MEASUREMENT_RMS_CURRENT;
  uint16_t rms_voltage_phase_a = ELECTRICAL_MEASUREMENT_RMS_VOLTAGE;
  uint16_t power_multiplier = (uint16_t)(1);
  uint16_t power_divisor = (uint16_t)(1000);
  // uint16_t e_meas_type = (uint16_t)0x0008;
  uint32_t e_meas_type = 0x00000000;
  e_meas_type |= (1 << 3);

  esp_zb_attribute_list_t *esp_zb_electrical_measurement_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_ELECTRICAL_MEASUREMENT);

  ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_MEASUREMENT_TYPE_ID, &e_meas_type));

  /* Add attribute ActivePower Phase A  */
  ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_ACTIVE_POWER_ID, &undefined_value));
  ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_POWER_DIVISOR_ID, &power_divisor));
  ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_POWER_MULTIPLIER_ID, &power_multiplier));
  /* Add attribute RMSVoltage Phase A */
  ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_RMSVOLTAGE_ID, &rms_voltage_phase_a));
  /* Add attribute RMSVoltage Phase A */
  ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_RMSCURRENT_ID, &rms_current_phase_a));
  // AC_FREQUENCY
  ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_AC_FREQUENCY_ID, &undefined_value));

  #if defined(ENERGY_BL0939)
    uint16_t rms_voltage_phase_b = ELECTRICAL_MEASUREMENT_RMS_VOLTAGE;
    uint16_t rms_current_phase_b = ELECTRICAL_MEASUREMENT_RMS_CURRENT;
    /* Add attribute RMSVoltage Phase B */
    ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_RMSVOLTAGE_PHB_ID, &rms_voltage_phase_b));
    /* Add attribute RMSCurrent Phase B */
    ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_RMSCURRENT_PHB_ID, &rms_current_phase_b));
    /* Add attribute ActivePower Phase B  */
    ESP_ERROR_CHECK(esp_zb_electrical_meas_cluster_add_attr(esp_zb_electrical_measurement_cluster, ESP_ZB_ZCL_ATTR_ELECTRICAL_MEASUREMENT_ACTIVE_POWER_PHB_ID, &undefined_value));
  #endif

  return esp_zb_electrical_measurement_cluster;
}

void set_zcl_string(char *buffer, char *value) {
  buffer[0] = (char)strlen(value);
  memcpy(buffer + 1, value, buffer[0]);
}

esp_err_t esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_ep_list_t *ep_list, uint8_t endpoint_id, zcl_basic_manufacturer_info_t *info) {
  esp_err_t ret = ESP_OK;
  esp_zb_cluster_list_t *cluster_list = NULL;
  esp_zb_attribute_list_t *basic_cluster = NULL;

  cluster_list = esp_zb_ep_list_get_ep(ep_list, endpoint_id);
  ESP_RETURN_ON_FALSE(cluster_list, ESP_ERR_INVALID_ARG, TAG, "Failed to find endpoint id: %d in list: %p", endpoint_id, ep_list);

  basic_cluster = esp_zb_cluster_list_get_cluster(cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  ESP_RETURN_ON_FALSE(basic_cluster, ESP_ERR_INVALID_ARG, TAG, "Failed to find basic cluster in endpoint: %d", endpoint_id);
  ESP_RETURN_ON_FALSE((info && info->manufacturer_name), ESP_ERR_INVALID_ARG, TAG, "Invalid manufacturer name");
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, info->manufacturer_name));
  ESP_RETURN_ON_FALSE((info && info->model_identifier), ESP_ERR_INVALID_ARG, TAG, "Invalid model identifier");
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, info->model_identifier));
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, info->firmware_version));
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_DATE_CODE_ID, info->firmware_date));
  return ret;
}

esp_zb_attribute_list_t *esp_zb_create_basic_cluster(zcl_basic_manufacturer_info_t *info) {

  esp_zb_attribute_list_t *basic_attr_list = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_BASIC);

  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_attr_list, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, info->manufacturer_name));
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_attr_list, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, info->model_identifier));

  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_attr_list, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, info->firmware_version));
  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_attr_list, ESP_ZB_ZCL_ATTR_BASIC_DATE_CODE_ID, info->firmware_date));

  ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_attr_list, ESP_ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID, &info->power_source));

  return basic_attr_list;

  // esp_zb_cluster_list_t *basic_cluster = esp_zb_zcl_cluster_list_create();

  // esp_zb_cluster_list_add_basic_cluster(basic_cluster, basic_attr_list, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

  // return basic_cluster;
}

esp_zb_attribute_list_t *esp_zb_create_identify_cluster() {

  uint16_t identify_id = 0;
  uint16_t identify_time = 0;

  esp_zb_attribute_list_t *identify_attr_list = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY);

  ESP_ERROR_CHECK(esp_zb_identify_cluster_add_attr(identify_attr_list, ESP_ZB_ZCL_CMD_IDENTIFY_IDENTIFY_ID, &identify_id));
  // ESP_ERROR_CHECK(esp_zb_identify_cluster_add_attr(identify_attr_list, ESP_ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID, &identify_time));

  return identify_attr_list;

  // esp_zb_cluster_list_t *identify_cluster = esp_zb_zcl_cluster_list_create();
  // esp_zb_cluster_list_add_identify_cluster(identify_cluster, identify_attr_list, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

  // return identify_cluster;
}

esp_zb_attribute_list_t *esp_zb_create_on_off_cluster() {
  // esp_zb_cluster_list_t *esp_zb_create_on_off_cluster() {

  esp_zb_on_off_cluster_cfg_t on_off_cfg = {
      .on_off = ESP_ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE,
  };

  esp_zb_attribute_list_t *on_off_attr_list = esp_zb_on_off_cluster_create(&on_off_cfg);

  uint8_t default_value = 0xff;
  uint16_t on_of_time = ESP_ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
  uint16_t on_of_wait_time = ESP_ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;

  ESP_ERROR_CHECK(esp_zb_on_off_cluster_add_attr(on_off_attr_list, ESP_ZB_ZCL_ATTR_ON_OFF_START_UP_ON_OFF, &default_value));
  ESP_ERROR_CHECK(esp_zb_on_off_cluster_add_attr(on_off_attr_list, ESP_ZB_ZCL_ATTR_ON_OFF_ON_TIME, &on_of_time));
  ESP_ERROR_CHECK(esp_zb_on_off_cluster_add_attr(on_off_attr_list, ESP_ZB_ZCL_ATTR_ON_OFF_OFF_WAIT_TIME, &on_of_wait_time));

  return on_off_attr_list;

  // esp_zb_cluster_list_t *on_off_cluster = esp_zb_zcl_cluster_list_create();

  // esp_zb_cluster_list_add_on_off_cluster(on_off_cluster, on_off_attr_list, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

  // return on_off_cluster;
}

esp_zb_attribute_list_t *esp_zb_create_analog_value(int val){

  // esp_zb_analog_value_cluster_cfg_t analog_value_cfg ={0};
  esp_zb_analog_value_cluster_cfg_t analog_value_cfg ={
    .out_of_service = true,
    .status_flags = 1,
    .present_value = val,
  };

  esp_zb_attribute_list_t *analog_value_attr_list = esp_zb_analog_value_cluster_create(&analog_value_cfg);

  return analog_value_attr_list;
}

esp_zb_attribute_list_t *esp_zb_create_analog_input(int val){

  // esp_zb_analog_input_cluster_cfg_t analog_input_cfg ={0};
  esp_zb_analog_input_cluster_cfg_t analog_input_cfg ={
    .out_of_service = true,
    .status_flags = 1,
    .present_value = val,
  };

  esp_zb_attribute_list_t *analog_input_attr_list = esp_zb_analog_input_cluster_create(&analog_input_cfg);

  return analog_input_attr_list;
}

esp_zb_attribute_list_t *esp_zb_create_analog_output(int val){

  // esp_zb_analog_output_cluster_cfg_t analog_output_cfg ={0};
  esp_zb_analog_output_cluster_cfg_t analog_output_cfg ={
    .out_of_service = true,
    .status_flags = 1,
    .present_value = val,
  };

  esp_zb_attribute_list_t *analog_output_attr_list = esp_zb_analog_output_cluster_create(&analog_output_cfg);

  return analog_output_attr_list;
}

#ifdef USE_BATTERY_MOD
esp_zb_attribute_list_t *esp_zb_create_power_cfg_cluster(){

  uint8_t batteryVoltage = 0xff;
  uint8_t batterySize = 0x02;

  esp_zb_power_config_cluster_cfg_t power_cfg = {0};

  esp_zb_attribute_list_t *power_attr_list = esp_zb_power_config_cluster_create(&power_cfg);

  ESP_ERROR_CHECK(esp_zb_power_config_cluster_add_attr(power_attr_list, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID, &batteryVoltage));
  ESP_ERROR_CHECK(esp_zb_power_config_cluster_add_attr(power_attr_list, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID, &battery_percentage));
  ESP_ERROR_CHECK(esp_zb_power_config_cluster_add_attr(power_attr_list, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_SIZE_ID, &batterySize));

  return power_attr_list;
}
#endif

esp_zb_attribute_list_t *esp_zb_create_diagnostics_cluster(){

  esp_zb_diagnostics_cluster_cfg_t diagnostics_cfg;
  uint16_t count = 0;
  esp_zb_attribute_list_t *diagnostics_attr_list = esp_zb_diagnostics_cluster_create(&diagnostics_cfg);
  ESP_ERROR_CHECK(esp_zb_diagnostics_cluster_add_attr(diagnostics_attr_list, ESP_ZB_ZCL_ATTR_DIAGNOSTICS_NUMBER_OF_RESETS_ID, &count));

  return diagnostics_attr_list;
}