#pragma once

#ifdef __cplusplus
extern "C" {
#endif

esp_zb_attribute_list_t *esp_zb_create_ota_cluster();
esp_err_t zb_ota_upgrade_query_image_resp_handler(esp_zb_zcl_ota_upgrade_query_image_resp_message_t message);
esp_err_t zb_ota_upgrade_status_handler(esp_zb_zcl_ota_upgrade_value_message_t message);

#ifdef __cplusplus
} // extern "C"
#endif