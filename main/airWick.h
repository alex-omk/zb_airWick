#pragma once

#include <inttypes.h>
#include <sdkconfig.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


extern int32_t spray_counter;
extern int16_t spray_interval;
extern int64_t last_spray_time;

extern bool autoSpray;

void airWickClearCounter();

void airWickSetup();

void airWickMotorUP();

void airWickMotorDown();

void airWickSpray();

void airWickWriteInterval();