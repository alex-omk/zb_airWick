#ifndef TOOLS_H
#define TOOLS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_attr.h"

#define T_STATUS_FAILED "Failed!"
#define T_STATUS_DONE "Done"

#define SYS_STATS_INTERVAL_TASK 60000

#define MINUTES_TO_MS(minutes) ((minutes) * 60UL * 1000UL)

#define HOURS_TO_MS(hours) ((hours) * 60UL * 60UL * 1000UL)

extern uint16_t restart_counter;

int64_t IRAM_ATTR millis();

void setup_NVS();
bool check_NVS_key(const char *nvs_key);
int32_t read_NVS(const char *nvs_key);
bool write_NVS(const char *nvs_key, int value);

void print_chip_info();
void heap_stats();

void sys_stats_task(void *pvParameters);

#endif 