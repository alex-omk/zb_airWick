#pragma once

#include "common.h"

#ifdef USE_BATTERY_MOD

extern uint8_t battery_percentage;
extern int voltage;
extern int64_t last_battery_measurement_time;

void batteryUpdate(void);
void batterySetup(void);
void batteryReadVolts();
void batteryPercentage(void);

#endif