#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_check.h"
#include "esp_err.h"
#include "esp_zigbee_core.h"

/*! Maximum length of ManufacturerName string field */
#define ESP_ZB_ZCL_CLUSTER_ID_BASIC_MANUFACTURER_NAME_MAX_LEN 32

/*! Maximum length of ModelIdentifier string field */
#define ESP_ZB_ZCL_CLUSTER_ID_BASIC_MODEL_IDENTIFIER_MAX_LEN 32

#define ESP_ZB_POWER_SOURCE_DC 3


/** optional basic manufacturer information */
typedef struct zcl_basic_manufacturer_info_s {
  char *manufacturer_name;
  char *model_identifier;
  char *firmware_version;
  char *firmware_date;
  uint8_t power_source;
} zcl_basic_manufacturer_info_t;

esp_zb_attribute_list_t *esp_zb_create_basic_cluster(zcl_basic_manufacturer_info_t *info);

esp_zb_attribute_list_t *esp_zb_create_on_off_cluster();

esp_zb_attribute_list_t *esp_zb_create_identify_cluster();

esp_zb_attribute_list_t *measurement_attr_cluster();

esp_zb_attribute_list_t *esp_zb_create_power_cfg_cluster();

esp_zb_attribute_list_t *esp_zb_create_analog_value(int val);

esp_zb_attribute_list_t *esp_zb_create_analog_input(int val);

esp_zb_attribute_list_t *esp_zb_create_analog_output(int val);

esp_zb_attribute_list_t *esp_zb_create_diagnostics_cluster();

void update_attribute_value(uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id, void *value, const char *attr_name);

void set_zcl_string(char *buffer, char *value);

/**
 * @brief Adds manufacturer information to the ZCL basic cluster of endpoint
 *
 * @param[in] ep_list The pointer to the endpoint list with @p endpoint_id
 * @param[in] endpoint_id The endpoint identifier indicating where the ZCL basic cluster resides
 * @param[in] info The pointer to the basic manufacturer information
 * @return
 *      - ESP_OK: On success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 */
esp_err_t esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_ep_list_t *ep_list, uint8_t endpoint_id, zcl_basic_manufacturer_info_t *info);

#ifdef __cplusplus
} // extern "C"
#endif