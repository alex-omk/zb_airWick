#pragma once

#include "common.h"

#ifdef USE_BATTERY_MOD

extern int adc_raw;
extern int voltage;
extern int64_t last_battery_measurement_time;

void batteryUpdate(void);
void batterySetup(void);
void batteryReadVolts();
void batteryPercentage(void);

// void adc_calibration_deinit(adc_cali_handle_t handle);

// void battery_task(void *pvParameters);

#endif