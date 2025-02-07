# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS ${app_sources}
    PRIV_REQUIRES nvs_flash app_update esp_timer freertos ieee802154 esp_adc
)
