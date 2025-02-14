#include "common.h"
#include "esp_timer.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"
#include <esp_ota_ops.h>
#include <string.h>
#include <zlib.h>

static const char *TAG = "ESP_ZB_OTA";

#define OTA_UPGRADE_HW_VERSION    0x0101   /* The parameter indicates the version of hardware */
#define OTA_UPGRADE_MAX_DATA_SIZE 64

#define OTA_UPGRADE_QUERY_INTERVAL (12 * 60) // ESP_ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF;

bool zlib_init = false;
static const esp_partition_t *s_ota_partition = NULL;
static esp_ota_handle_t s_ota_handle = 0;
z_stream zlib_stream;

uint8_t ota_header[6];
size_t ota_header_len = 0;
bool ota_upgrade_subelement = false;
size_t ota_data_len = 0;
uint64_t ota_last_receive_us = 0;
size_t ota_receive_not_logged = 0;


esp_zb_attribute_list_t *esp_zb_create_ota_cluster() {

  esp_zb_ota_cluster_cfg_t ota_cluster_cfg = {
      .ota_upgrade_file_version = FIRMWARE_VERSION,         /*!<  The attribute indicates the file version of the runningfirmware image on the device */
      .ota_upgrade_downloaded_file_ver = FIRMWARE_VERSION,  /*!<  The attribute indicates the file version of the downloaded image on the device*/
      .ota_upgrade_manufacturer = OTA_UPGRADE_MANUFACTURER, /*!<  The attribute indicates the value for the manufacturer of the device */
      .ota_upgrade_image_type = OTA_UPGRADE_IMAGE_TYPE,     /*!<  The attribute indicates the the image type of the file that the client is currently downloading */
  };

  esp_zb_attribute_list_t *esp_zb_ota_cluster = esp_zb_ota_cluster_create(&ota_cluster_cfg);

  esp_zb_zcl_ota_upgrade_client_variable_t ota_client_parameter_cfg = {
      .timer_query = OTA_UPGRADE_QUERY_INTERVAL,  /* time interval for query next image request command */
      .hw_version = OTA_UPGRADE_HW_VERSION,       /* version of hardware */
      .max_data_size = OTA_UPGRADE_MAX_DATA_SIZE, /* maximum data size of query block image */
  };

  ESP_ERROR_CHECK(esp_zb_ota_cluster_add_attr(esp_zb_ota_cluster, ESP_ZB_ZCL_ATTR_OTA_UPGRADE_CLIENT_DATA_ID, (void *)&ota_client_parameter_cfg));

  // esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
  // ESP_ERROR_CHECK(esp_zb_cluster_list_add_ota_cluster(cluster_list, esp_zb_ota_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));

  return esp_zb_ota_cluster;
}

esp_err_t zb_ota_upgrade_query_image_resp_handler(esp_zb_zcl_ota_upgrade_query_image_resp_message_t message) {
  esp_err_t ret = ESP_OK;
  if (message.info.status == ESP_ZB_ZCL_STATUS_SUCCESS) {
    ESP_LOGI(TAG, "Queried OTA image from address: 0x%04hx, endpoint: %d", message.server_addr.u.short_addr, message.server_endpoint);
    ESP_LOGI(TAG, "Image version: 0x%lx, manufacturer code: 0x%x, image size: %ld", message.file_version, message.manufacturer_code, message.image_size);
  }
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Approving OTA image upgrade");
  } else {
    ESP_LOGI(TAG, "Rejecting OTA image upgrade, status: %s", esp_err_to_name(ret));
  }
  return ret;
}

void ota_reset() {
  if (s_ota_partition) {
    esp_ota_abort(s_ota_handle);
    s_ota_partition = NULL;
  }

  if (zlib_init) {
    inflateEnd(&zlib_stream);
    zlib_init = false;
  }
}

bool ota_start() {
  zlib_init = false;
  s_ota_partition = NULL;
  s_ota_handle = 0;

  memset(&zlib_stream, 0, sizeof(zlib_stream));
  int ret = inflateInit(&zlib_stream);
  if (ret == Z_OK) {
    zlib_init = true;
  } else {
    ESP_LOGE(__func__, "zlib init failed: %d", ret);
    return false;
  }

  s_ota_partition = esp_ota_get_next_update_partition(NULL);
  if (!s_ota_partition) {
    ESP_LOGE(__func__, "No next OTA partition");
    return false;
  }

  esp_err_t err =
      esp_ota_begin(s_ota_partition, OTA_WITH_SEQUENTIAL_WRITES, &s_ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error starting OTA: %d", err);
    s_ota_partition = NULL;
    return false;
  }

  return true;
}

bool ota_write(const uint8_t *data, size_t size, bool flush) {
  uint8_t buf[256];

  if (!s_ota_partition) {
    return false;
  }

  zlib_stream.avail_in = size;
  zlib_stream.next_in = data;

  do {
    zlib_stream.avail_out = sizeof(buf);
    zlib_stream.next_out = buf;

    int ret = inflate(&zlib_stream, flush ? Z_FINISH : Z_NO_FLUSH);
    if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR ||
        ret == Z_MEM_ERROR) {
      ESP_LOGE(__func__, "zlib error: %d", ret);
      esp_ota_abort(s_ota_handle);
      s_ota_partition = NULL;
      return false;
    }

    size_t available = sizeof(buf) - zlib_stream.avail_out;
    if (available > 0) {
      esp_err_t err = esp_ota_write(s_ota_handle, buf, available);
      if (err != ESP_OK) {
        ESP_LOGE(__func__, "Error writing OTA: %d", err);
        esp_ota_abort(s_ota_handle);
        s_ota_partition = NULL;
        return false;
      }
    }
  } while (zlib_stream.avail_in > 0 || zlib_stream.avail_out == 0);

  return true;
}

bool ota_finish() {
  if (!s_ota_partition) {
    ESP_LOGE(__func__, "OTA not running");
    return false;
  }

  if (!ota_write(NULL, 0, true)) {
    return false;
  }

  esp_err_t err = esp_ota_end(s_ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error ending OTA: %d", err);
    s_ota_partition = NULL;
    return false;
  }

  inflateEnd(&zlib_stream);
  zlib_init = false;

  err = esp_ota_set_boot_partition(s_ota_partition);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error setting boot partition: %d", err);
    s_ota_partition = NULL;
    return false;
  }

  s_ota_partition = NULL;
  return true;
}

esp_err_t zb_ota_upgrade_status_handler(esp_zb_zcl_ota_upgrade_value_message_t message) {
  esp_err_t ret = ESP_OK;
  static uint32_t offset = 0;
  static uint32_t total_size = 0;

  if (message.info.status == ESP_ZB_ZCL_STATUS_SUCCESS) {
    if (message.upgrade_status != ESP_ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE) {
      if (ota_receive_not_logged) {
        ESP_LOGD(__func__, "OTA (%zu receive data messages suppressed)", ota_receive_not_logged);
        ota_receive_not_logged = 0;
      }
      ota_last_receive_us = 0;
    }

    switch (message.upgrade_status) {
    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_START:
      ESP_LOGI(__func__, "-- OTA upgrade start");
      ota_reset();
      ota_header_len = 0;
      ota_upgrade_subelement = false;
      ota_data_len = 0;
      if (!ota_start()) {
        ota_reset();
        ret = ESP_FAIL;
      }
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
      const uint8_t *payload = message.payload;
      size_t payload_size = message.payload_size;
      total_size = message.ota_header.image_size;
      offset += message.payload_size;
      // ESP_LOGI(__func__, "-- OTA Client receives data: progress [%ld/%ld]", offset, total_size);

      // Read and process the first sub-element, ignoring everything else
      while (ota_header_len < 6 && payload_size > 0) {
        ota_header[ota_header_len++] = payload[0];
        payload++;
        payload_size--;
      }

      if (!ota_upgrade_subelement && ota_header_len == 6) {
        if (ota_header[0] == 0 && ota_header[1] == 0) {
          ota_upgrade_subelement = true;
          ota_data_len = (((int)ota_header[5] & 0xFF) << 24) |
                         (((int)ota_header[4] & 0xFF) << 16) |
                         (((int)ota_header[3] & 0xFF) << 8) |
                         ((int)ota_header[2] & 0xFF);
          ESP_LOGD(__func__, "OTA sub-element size %zu", ota_data_len);
        } else {
          ESP_LOGE(__func__, "OTA sub-element type %02x%02x not supported", ota_header[0], ota_header[1]);
          ota_reset();
          ret = ESP_FAIL;
        }
      }

      if (ota_data_len) {
        if (payload_size > ota_data_len)
          payload_size = ota_data_len;
        ota_data_len -= payload_size;

        if (ota_write(payload, payload_size, false)) {
          uint64_t now_us = esp_timer_get_time();
          if (!ota_last_receive_us ||
              now_us - ota_last_receive_us >= 30 * 1000 * 1000) {
            ESP_LOGD(__func__, "OTA receive data (%zu messages suppressed)", ota_receive_not_logged);
            ota_last_receive_us = now_us;
            ota_receive_not_logged = 0;
          } else {
            ota_receive_not_logged++;
          }
        } else {
          ota_reset();
          ret = ESP_FAIL;
        }
      }
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_APPLY:
      ESP_LOGI(__func__, "OTA apply");
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
      ESP_LOGI(__func__, "OTA data complete");
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
      ESP_LOGI(__func__, "OTA finished");
      ESP_LOGI(__func__, "-- OTA Information: version: 0x%lx, manufacturer code: 0x%x, image type: 0x%x, total size: %ld bytes,",
          message.ota_header.file_version, message.ota_header.manufacturer_code, message.ota_header.image_type,
          message.ota_header.image_size);
      bool ok = ota_finish();
      ota_reset();
      if (ok)
        esp_restart();
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_ABORT:
      ESP_LOGI(__func__, "OTA aborted");
      ota_reset();
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_OK:
      ESP_LOGI(__func__, "OTA data ok");
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_ERROR:
      ESP_LOGI(__func__, "OTA data error");
      ota_reset();
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_NORMAL:
      ESP_LOGI(__func__, "OTA image accepted");
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_BUSY:
      ESP_LOGI(__func__, "OTA busy");
      ota_reset();
      break;

    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_SERVER_NOT_FOUND:
      ESP_LOGI(__func__, "OTA server not found");
      ota_reset();
      break;

    default:
      ESP_LOGI(__func__, "OTA status: %d", message.upgrade_status);
      break;
    }
  }

  return ret;
}