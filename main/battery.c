#include "common.h"

#ifdef USE_BATTERY_MOD

#include "battery.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "zcl/esp_zigbee_zcl_power_config.h"
#include "tools.h"
#include "zb_util.h"

static const char *TAG = "ESP_ZB_BATT";

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/soc_caps.h"

#define ADC_CHANNEL ADC_CHANNEL_0
#define ADC_ATTEN ADC_ATTEN_DB_12

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void adc_calibration_deinit(adc_cali_handle_t handle);

adc_cali_handle_t adc1_cali_chan0_handle = NULL;

adc_oneshot_unit_handle_t adc1_handle;

int adc_raw;
float battery_voltage;
int battery_millivolts;
uint8_t battery_percentage = 0xff;

int64_t last_battery_measurement_time = 0;

bool do_calibration1_chan0;


uint8_t test_percentage = 200;
uint8_t test_voltage = 255;

void batteryUpdate(void) {
  batterySetup();
  batteryReadVolts();
  batteryPercentage();
}

void batterySetup(void) {
  ESP_LOGI(TAG, "Setup ADC");

  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

  do_calibration1_chan0 = adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL, ADC_ATTEN, &adc1_cali_chan0_handle);
}

void batteryReadVolts() {
  int adc = 0, sample = 0, count = 10;
  
  ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw)); //one blank measurement for stabilization
  
  while (sample < count) {
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw));
    // ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC_CHANNEL, adc_raw);
    adc += adc_raw;
    sample++;
  }

  int adc_avg = adc / count;

  ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC_CHANNEL, adc_avg);

  if (do_calibration1_chan0) {
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_avg, &battery_millivolts));
    // ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw, &battery_millivolts));
    
    ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADC_CHANNEL, battery_millivolts);
    battery_millivolts = battery_millivolts * 2; // divider
    battery_voltage = battery_millivolts / 1000.0;

    last_battery_measurement_time = millis();
    ESP_LOGW(TAG, "LAST READ BATT %d millis", (int)last_battery_measurement_time);
  }

  ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
  if (do_calibration1_chan0) {
    adc_calibration_deinit(adc1_cali_chan0_handle);
  }
}

void batteryPercentage(void) {

  // 0% 3.2v
  // 100% 4.2v

  if (battery_voltage < 3.2) {
    battery_percentage = 0;
  } else if (battery_voltage > 4.1) {
    battery_percentage = 100;
  } else {
    battery_percentage = (uint8_t)(((battery_voltage - 3.2f) / (4.2f - 3.2f)) * 100);
    // percentage = (uint8_t)((battery_voltage - 3.2) * 100 / 2.5);
  }

  ESP_LOGI(TAG, "vIN: %.3f, percentage: %d", battery_voltage, battery_percentage);
  battery_percentage = battery_percentage * 2; // zigbee scale
  uint8_t r_state = 0;
  test_percentage = test_percentage - 1;
  test_voltage = test_voltage - 1;
  
  // esp_zb_zcl_status_t status = esp_zb_zcl_set_attribute_val(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID, &battery_percentage, false);
  // esp_zb_zcl_status_t status2 = esp_zb_zcl_set_attribute_val(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID, &battery_voltage, false);
  
  esp_zb_zcl_status_t status = esp_zb_zcl_set_attribute_val(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID, &test_percentage, false);
  // esp_zb_zcl_status_t status2 = esp_zb_zcl_set_attribute_val(HA_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID, &test_voltage, false);
  if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    ESP_LOGE(TAG, "Set battery percentage attribute value FAIL!");
  }
}

static void adc_calibration_deinit(adc_cali_handle_t handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
  ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
  ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {

  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  *out_handle = handle;
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Calibration Success");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else {
    ESP_LOGE(TAG, "Invalid arg or no memory");
  }

  return calibrated;
}

#endif